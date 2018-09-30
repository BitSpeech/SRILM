/*
 * NgramStats.cc --
 *	N-gram counting
 *
 */

#ifndef _NgramStats_cc_
#define _NgramStats_cc_

#ifndef lint
static char NgramStats_Copyright[] = "Copyright (c) 1995-2006 SRI International.  All Rights Reserved.";
static char NgramStats_RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/NgramStats.cc,v 1.32 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;
#include <string.h>

const unsigned maxLineLength = 10000;

#include "NgramStats.h"

#include "Trie.cc"
#include "LHash.cc"
#include "Array.cc"

#define INSTANTIATE_NGRAMCOUNTS(CountT) \
	INSTANTIATE_TRIE(VocabIndex,CountT); \
	template class NgramCounts<CountT>

template <class CountT>
void
NgramCounts<CountT>::dump()
{
    cerr << "order = " << order << endl;
    counts.dump();
}

template <class CountT>
void
NgramCounts<CountT>::memStats(MemStats &stats)
{
    stats.total += sizeof(*this) - sizeof(counts) - sizeof(vocab);
    vocab.memStats(stats);
    counts.memStats(stats);
}

template <class CountT>
NgramCounts<CountT>::NgramCounts(Vocab &vocab, unsigned int maxOrder)
    : LMStats(vocab), order(maxOrder)
{
}

template <class CountT>
unsigned int
NgramCounts<CountT>::countSentence(const VocabString *words, CountT factor)
{
    static VocabIndex wids[maxWordsPerLine + 3];
    unsigned int howmany;

    if (openVocab) {
	howmany = vocab.addWords(words, wids + 1, maxWordsPerLine + 1);
    } else {
	howmany = vocab.getIndices(words, wids + 1, maxWordsPerLine + 1,
					    vocab.unkIndex());
    }

    /*
     * Check for buffer overflow
     */
    if (howmany == maxWordsPerLine + 1) {
	return 0;
    }

    /*
     * update OOV count
     */
    if (!openVocab) {
	for (unsigned i = 1; i <= howmany; i++) {
	    if (wids[i] == vocab.unkIndex()) {
		stats.numOOVs ++;
	    }
	}
    }

    /*
     * Insert begin/end sentence tokens if necessary
     */
    VocabIndex *start;
    
    if (wids[1] == vocab.ssIndex()) {
	start = wids + 1;
    } else {
	wids[0] = vocab.ssIndex();
	start = wids;
    }
    if (wids[howmany] != vocab.seIndex()) {
	wids[howmany + 1] = vocab.seIndex();
	wids[howmany + 2] = Vocab_None;
    }

    return countSentence(start, factor);
}

/*
 * Incrememt counts indexed by words, starting at node.
 */
template <class CountT>
void
NgramCounts<CountT>::incrementCounts(const VocabIndex *words,
					unsigned minOrder, CountT factor)
{
    NgramNode *node = &counts;

    for (int i = 0; i < order; i++) {
	VocabIndex wid = words[i];

	/*
	 * check of end-of-sentence
	 */
        if (wid == Vocab_None) {
	    break;
	} else {
	    node = node->insertTrie(wid);
	    if (i + 1 >= minOrder) {
		node->value() += factor;
	    }
	}
    }
}

template <class CountT>
unsigned int
NgramCounts<CountT>::countSentence(const VocabIndex *words, CountT factor)
{
    unsigned int start;

    for (start = 0; words[start] != Vocab_None; start++) {
        incrementCounts(words + start, 1, factor);
    }

    /*
     * keep track of word and sentence counts
     */
    stats.numWords += start;
    if (words[0] == vocab.ssIndex()) {
	stats.numWords --;
    }
    if (start > 0 && words[start-1] == vocab.seIndex()) {
	stats.numWords --;
    }

    stats.numSentences ++;

    return start;
}

/*
 * Type-dependent count <--> string conversions
 */
#ifdef INSTANTIATE_TEMPLATES
static
#endif
char ctsBuffer[100];

template <class CountT>
static inline const char *
countToString(CountT count)
{
    sprintf(ctsBuffer, "%lg", (double)count);
    return ctsBuffer;
}

static inline const char *
countToString(unsigned count)
{
    sprintf(ctsBuffer, "%u", count);
    return ctsBuffer;
}

static inline const char *
countToString(int count)
{
    sprintf(ctsBuffer, "%d", count);
    return ctsBuffer;
}

template <class CountT>
static inline Boolean
stringToCount(const char *str, CountT &count)
{
    double x;
    if (sscanf(str, "%lf", &x) == 1) {
	count = x;
	return true;
    } else {
	return false;
    }
}
static inline Boolean
stringToCount(const char *str, unsigned int &count)
{
    /*
     * scanf("%u") doesn't check for a positive sign, so we have to ourselves.
     */
    return (*str != '-' && sscanf(str, "%u", &count) == 1);
}

static inline Boolean
stringToCount(const char *str, unsigned short &count)
{
    /*
     * scanf("%u") doesn't check for a positive sign, so we have to ourselves.
     */
    return (*str != '-' && sscanf(str, "%hu", &count) == 1);
}

static inline Boolean
stringToCount(const char *str, XCount &count)
{
    unsigned x;
    if (stringToCount(str, x)) {
    	count = x;
	return true;
    } else {
    	return false;
    }
}

static inline Boolean
stringToCount(const char *str, int &count)
{
    return (sscanf(str, "%d", &count) == 1);
}

template <class CountT>
unsigned int
NgramCounts<CountT>::parseNgram(char *line,
		      VocabString *words,
		      unsigned int max,
		      CountT &count)
{
    unsigned howmany = Vocab::parseWords(line, words, max);

    if (howmany == max) {
	return 0;
    }

    /*
     * Parse the last word as a count
     */
    if (!stringToCount(words[howmany - 1], count)) {
	return 0;
    }

    howmany --;
    words[howmany] = 0;

    return howmany;
}

template <class CountT>
unsigned int
NgramCounts<CountT>::readNgram(File &file,
		      VocabString *words,
		      unsigned int max,
		      CountT &count)
{
    char *line;

    /*
     * Read next ngram count from file, skipping blank lines
     */
    line = file.getline();
    if (line == 0) {
	return 0;
    }

    unsigned howmany = parseNgram(line, words, max, count);

    if (howmany == 0) {
	file.position() << "malformed N-gram count or more than " << max - 1 << " words per line\n";
	return 0;
    }

    return howmany;
}

template <class CountT>
Boolean
NgramCounts<CountT>::read(File &file, unsigned int order)
{
    VocabString words[maxNgramOrder + 1];
    VocabIndex wids[maxNgramOrder + 1];
    CountT count;
    unsigned int howmany;

    while (howmany = readNgram(file, words, maxNgramOrder + 1, count)) {
	/*
	 * Skip this entry if the length of the ngram exceeds our 
	 * maximum order
	 */
	if (howmany > order) {
	    continue;
	}
	/* 
	 * Map words to indices
	 */
	if (openVocab) {
	    vocab.addWords(words, wids, maxNgramOrder);
	} else {
	    vocab.getIndices(words, wids, maxNgramOrder, vocab.unkIndex());
	}

	/*
	 *  Update the count
	 */
	*counts.insert(wids) += count;
    }
    /*
     * XXX: always return true for now, should return false if there was
     * a format error in the input file.
     */
    return true;
}

/*
 * Helper function to record a set of meta-counts at a given prefix into
 * the ngram tree
 */
template <class CountT>
void
NgramCounts<CountT>::addCounts(const VocabIndex *prefix,
				const LHash<VocabIndex, CountT> &set)
{
    NgramNode *node = counts.insertTrie(prefix);

    LHashIter<VocabIndex, CountT> setIter(set);
    VocabIndex word;
    CountT *count;
    while (count = setIter.next(word)) {
	*node->insert(word) += *count;
    }
}

/*
 * Read a counts file discarding counts below given minimum counts
 * Assumes that counts are merged, i.e., ngram order is generated by a
 * pre-order traversal of the ngram tree.
 */
template <class CountT>
Boolean
NgramCounts<CountT>::readMinCounts(File &file, unsigned order,
						unsigned *minCounts)
{
    VocabString words[maxNgramOrder + 1];
    VocabIndex prefix[maxNgramOrder + 1];

    /*
     * Data for tracking deletable prefixes
     */
    LHash<VocabIndex, CountT> *metaCounts;
    
    metaCounts = new LHash<VocabIndex, CountT>[order];
    assert(metaCounts != 0);

    makeArray(Boolean, haveCounts, order);
    makeArray(VocabIndex *, lastPrefix, order);
    for (unsigned i = 0; i < order; i ++) {
	lastPrefix[i] = new VocabIndex[order + 1];
	assert(lastPrefix[i] != 0);
	lastPrefix[i][0] = Vocab_None;
	haveCounts[i] = false;
    }

    Vocab::compareVocab = 0;		// do comparison by word index

    CountT count;
    unsigned int howmany;

    while (howmany = readNgram(file, words, maxNgramOrder + 1, count)) {
	/*
	 * Skip this entry if the length of the ngram exceeds our 
	 * maximum order
	 */
	if (howmany > order) {
	    continue;
	}

	VocabIndex metaTag = Vocab_None;
	Boolean haveRealCount = false;

	/*
	 * Discard counts below mincount threshold
	 */
	if (count < minCounts[howmany - 1]) {
	    if ((unsigned)count == 0) {
		continue;
	    } else {
		/*
		 * See if metatag is defined, and if so, keep going
		 */
	    	metaTag = vocab.metaTagOfType((unsigned)count);

		if (metaTag == Vocab_None) {
		    continue;
		}
	    }
	}

	/* 
	 * Map words to indices (includes both N-gram prefix and last word)
	 */
	if (openVocab) {
	    vocab.addWords(words, prefix, maxNgramOrder);
	} else {
	    vocab.getIndices(words, prefix, maxNgramOrder, vocab.unkIndex());
	}

	/* 
	 * Truncate Ngram for prefix comparison
	 */
	VocabIndex lastWord = prefix[howmany - 1];
	prefix[howmany - 1] = Vocab_None;

	/*
	 * If current ngram prefix differs from previous one assume that 
	 * we are done with all ngrams of that prefix (this assumption is 
	 * true for counts in prefix order).
	 * In this case, output saved meta-counts for the previous prefix,
	 * but only if there also were some real counts -- this saves a lot
	 * of space in the counts tree.
	 */
	if (Vocab::compare(prefix, lastPrefix[howmany-1]) != 0) {
	    if (haveCounts[howmany-1]) {
		addCounts(lastPrefix[howmany-1], metaCounts[howmany-1]);
		haveCounts[howmany-1] = false;
	    }
	    metaCounts[howmany-1].clear();
	    Vocab::copy(lastPrefix[howmany-1], prefix);
	}

	/*
	 *  Update the count (either meta or real)
	 */
	if (metaTag != Vocab_None) {
	    *metaCounts[howmany-1].insert(metaTag) += 1;
	} else {
	    *insertCount(prefix, lastWord) += count;
	    haveCounts[howmany-1] = true;
	}
    }

    /*
     * Record final saved meta-counts and deallocate prefix buffers
     */
    for (unsigned i = order; i > 0; i --) {
	if (haveCounts[i-1]) {
	    addCounts(lastPrefix[i-1], metaCounts[i-1]);
	}
	delete [] lastPrefix[i-1];
    }

    delete [] metaCounts;

    /*
     * XXX: always return true for now, should return false if there was
     * a format error in the input file.
     */
    return true;
}


template <class CountT>
unsigned int
NgramCounts<CountT>::writeNgram(File &file,
		       const VocabString *words,
		       CountT count)
{
    unsigned int i;

    if (words[0]) {
	fprintf(file, "%s", words[0]);
	for (i = 1; words[i]; i++) {
	    fprintf(file, " %s", words[i]);
	}
    }
    fprintf(file, "\t%s\n", countToString(count));

    return i;
}

/*
 * For reasons of efficiency the write() method doesn't use
 * writeNgram()  (yet).  Instead, it fills up a string buffer 
 * as it descends the tree recursively.  this avoid having to
 * lookup shared prefix words and buffer the corresponding strings
 * repeatedly.
 */
template <class CountT>
void
NgramCounts<CountT>::writeNode(
    NgramNode *node,		/* the trie node we're at */
    File &file,			/* output file */
    char *buffer,		/* output buffer */
    char *bptr,			/* pointer into output buffer */
    unsigned int level,		/* current trie level */
    unsigned int order,		/* target trie level */
    Boolean sorted)		/* produce sorted output */
{
    NgramNode *child;
    VocabIndex wid;

    TrieIter<VocabIndex,CountT> iter(*node, sorted ? vocab.compareIndex() : 0);

    /*
     * Iterate over the child nodes at the current level,
     * appending their word strings to the buffer
     */
    while (!file.error() && (child = iter.next(wid))) {
	VocabString word = vocab.getWord(wid);

	if (word == 0) {
	   cerr << "undefined word index " << wid << "\n";
	   continue;
	}

	unsigned wordLen = strlen(word);

	if (bptr + wordLen + 1 > buffer + maxLineLength) {
	   *bptr = '0';
	   cerr << "ngram ["<< buffer << word
		<< "] exceeds write buffer\n";
	   continue;
	}
        
	strcpy(bptr, word);

	/*
	 * If this is the final level, print out the ngram and the count.
	 * Otherwise set up another level of recursion.
	 */
	if (order == 0 || level == order) {
	   fprintf(file, "%s\t%s\n", buffer, countToString(child->value()));
	} 
	
	if (order == 0 || level < order) {
	   *(bptr + wordLen) = ' ';
	   writeNode(child, file, buffer, bptr + wordLen + 1, level + 1,
			order, sorted);
	}
    }
}

template <class CountT>
void
NgramCounts<CountT>::write(File &file, unsigned int order, Boolean sorted)
{
    static char buffer[maxLineLength];
    writeNode(&counts, file, buffer, buffer, 1, order, sorted);
}

template <class CountT>
CountT
NgramCounts<CountT>::sumNode(NgramNode *node, unsigned int level, unsigned int order)
{
    /*
     * For leaf nodes, or nodes beyond the maximum level we are summing,
     * return their count, leaving it unchanged.
     * For nodes closer to the root, replace their counts with the
     * the sum of the counts of all the children.
     */
    if (level > order || node->numEntries() == 0) {
	return node->value();
    } else {
	NgramNode *child;
	TrieIter<VocabIndex,CountT> iter(*node);
	VocabIndex wid;

	CountT sum = 0;

	while (child = iter.next(wid)) {
	    sum += sumNode(child, level + 1, order);
	}

	node->value() = sum;

	return sum;
    }
}

template <class CountT>
CountT
NgramCounts<CountT>::sumCounts(unsigned int order)
{
    return sumNode(&counts, 1, order);
}

/*
 * Prune ngram counts
 */
template <class CountT>
unsigned
NgramCounts<CountT>::pruneCounts(CountT minCount)
{
    unsigned npruned = 0;
    makeArray(VocabIndex, ngram, order + 1);

    for (unsigned i = 1; i <= order; i++) {
	CountT *count;
	NgramCountsIter<CountT> countIter(*this, ngram, i);

	/*
	 * This enumerates all ngrams
	 */
	while (count = countIter.next()) {
	    if (*count < minCount) {
		removeCount(ngram);
		npruned ++;
	    }
	}
    }
    return npruned;
}

#endif /* _NgramStats_cc_ */
