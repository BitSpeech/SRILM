/*
 * Vocab.cc --
 *	The vocabulary class implementation.
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/Vocab.cc,v 1.34 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "File.h"
#include "Vocab.h"

#include "LHash.cc"
#include "Array.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(VocabString,VocabIndex);
INSTANTIATE_ARRAY(VocabString);
#endif

Vocab::Vocab(VocabIndex start, VocabIndex end)
    : byIndex(start), nextIndex(start), maxIndex(end), _metaTag(0)
{
    /*
     * Vocab_None is both the non-index value and the end-token
     * for key sequences used with Tries.  Both need to be
     * the same value so the user or the LM library doesn't have
     * to deal with Map_noKey() and friends.
     */
    assert(Map_noKeyP(Vocab_None));

    /*
     * Make the last Vocab created be the default one used in
     * ostream output
     */
    outputVocab = this;

    /*
     * default is a closed-vocabulary model
     */
    _unkIsWord = false;

    /*
     * do not map word strings to lowercase by defaults
     */
    _toLower = false;

    /*
     * set some special vocabulary tokens to their defaults
     */
    _unkIndex = addWord(Vocab_Unknown);
    _ssIndex = addWord(Vocab_SentStart);
    _seIndex = addWord(Vocab_SentEnd);
    _pauseIndex = addWord(Vocab_Pause);

    /*
     * declare some known non-events
     */
    addNonEvent(_ssIndex);
    addNonEvent(_pauseIndex);
}

// Compute memory usage
void
Vocab::memStats(MemStats &stats) const
{
    stats.total += sizeof(*this) - sizeof(byName) - sizeof(byIndex);
    byName.memStats(stats);
    byIndex.memStats(stats);
}

// map word string to lowercase
// returns a static buffer
static VocabString
mapToLower(VocabString name)
{
    static char lower[maxWordLength + 1];

    unsigned  i;
    for (i = 0; name[i] != 0 && i < maxWordLength; i++) {
	lower[i] = tolower((unsigned char)name[i]);
    }
    lower[i] = '\0';

    assert(i < maxWordLength);

    return lower;
}

// Add word to vocabulary
VocabIndex
Vocab::addWord(VocabString name)
{
    if (_toLower) {
	name = mapToLower(name);
    }

    Boolean found;
    VocabIndex *indexPtr = byName.insert(name, found);

    if (found) {
	return *indexPtr;
    } else {
	if (nextIndex == maxIndex) {
	    return Vocab_None;
	} else {
	    *indexPtr = nextIndex;
	    byIndex[nextIndex] = byName.getInternalKey(name);

	    /*
	     * Check for metatags, and intern them into our metatag type map
	     */
	    if (_metaTag != 0) {
		unsigned metaTagLength = strlen(_metaTag);

		if (strncmp(name, _metaTag, metaTagLength) == 0) {
		    int type = -1;
		    if (name[metaTagLength] == '\0') {
			type = 0;
		    } else {
			sscanf(&name[metaTagLength], "%u", &type);
		    }
		    if (type >= 0) {
			*metaTagMap.insert(nextIndex) = type;
		    }
		}
	    }

	    return nextIndex++;
	}
    } 
}

// declare word to be a non-event
VocabIndex
Vocab::addNonEvent(VocabIndex word)
{
    /* 
     * First make sure the word is already defined
     */
    if (getWord(word) == 0) {
	return Vocab_None;
    } else {
	*nonEventMap.insert(word) = 1;
	return word;
    }
}

// declare a set of non-events
Boolean
Vocab::addNonEvents(Vocab &nonevents)
{
    VocabIter viter(nonevents);
    Boolean ok = true;

    VocabString name;
    while (name = viter.next()) {
	if (addNonEvent(name) == Vocab_None) {
	    ok = false;
	}
    }

    return ok;
}

// remove word as a non-event
Boolean
Vocab::removeNonEvent(VocabIndex word)
{
    /* 
     * First make sure the word is already defined
     */
    if (getWord(word) == 0) {
	return false;
    } else {
	return nonEventMap.remove(word) != 0;
    }
}

// Get a word's index by its name
VocabIndex
Vocab::getIndex(VocabString name, VocabIndex unkIndex)
{
    if (_toLower) {
	name = mapToLower(name);
    }

    VocabIndex *indexPtr = byName.find(name);

    /*
     * If word is a metatag and not already interned, do it now
     */
    if (indexPtr == 0 &&
	_metaTag != 0 &&
	strncmp(name, _metaTag, strlen(_metaTag)) == 0)
    {
	return addWord(name);
    } else {
	return indexPtr ? *indexPtr : unkIndex;
    }
}

// Get the index of a metatag
VocabIndex
Vocab::metaTagOfType(unsigned type)
{
    if (_metaTag == 0) {
	return Vocab_None;
    } else {
	if (type == 0) {
	    return getIndex(_metaTag);
	} else {
	    makeArray(char, tagName, strlen(_metaTag) + 20);

	    sprintf(tagName, "%s%u", _metaTag, type);
	    return getIndex(tagName);
	}
    }
}

// Get a word's name by its index
VocabString
Vocab::getWord(VocabIndex index)
{
    if (index < byIndex.base() || index >= nextIndex) {
	return 0;
    } else {
	return (*(Array<VocabString>*)&byIndex)[index];	// discard const
    }
}

// Delete a word by name
void
Vocab::remove(VocabString name)
{
    if (_toLower) {
	name = mapToLower(name);
    }

    VocabIndex *indexPtr = byName.remove(name);

    if (indexPtr) {
	byIndex[*indexPtr] = 0;
	nonEventMap.remove(*indexPtr);
	metaTagMap.remove(*indexPtr);

	if (*indexPtr == _ssIndex) {
	    _ssIndex = Vocab_None;
	}
	if (*indexPtr == _seIndex) {
	    _seIndex = Vocab_None;
	}
	if (*indexPtr == _unkIndex) {
	    _unkIndex = Vocab_None;
	}
	if (*indexPtr == _pauseIndex) {
	    _pauseIndex = Vocab_None;
	}
    }
}

// Delete a word by index
void
Vocab::remove(VocabIndex index)
{
    if (index < byIndex.base() || index >= nextIndex) {
	return;
    } else {
	VocabString name = byIndex[index];
	if (name) {
	    byName.remove(name);
	    byIndex[index] = 0;
	    nonEventMap.remove(index);
	    metaTagMap.remove(index);

	    if (index == _ssIndex) {
		_ssIndex = Vocab_None;
	    }
	    if (index == _seIndex) {
		_seIndex = Vocab_None;
	    }
	    if (index == _unkIndex) {
		_unkIndex = Vocab_None;
	    }
	    if (index == _pauseIndex) {
		_pauseIndex = Vocab_None;
	    }
	}
    }
}

// Convert index sequence to string sequence
unsigned int
Vocab::getWords(const VocabIndex *wids, VocabString *words,
						    unsigned int max)
{
    unsigned int i;

    for (i = 0; i < max && wids[i] != Vocab_None; i++) {
	words[i] = getWord(wids[i]);
    }
    if (i < max) {
	words[i] = 0;
    }
    return i;
}

// Convert word sequence to index sequence, adding words if needed
unsigned int
Vocab::addWords(const VocabString *words, VocabIndex *wids, unsigned int max)
{
    unsigned int i;

    for (i = 0; i < max && words[i] != 0; i++) {
	wids[i] = addWord(words[i]);
    }
    if (i < max) {
	wids[i] = Vocab_None;
    }
    return i;
}

// Convert word sequence to index sequence (without adding words)
unsigned int
Vocab::getIndices(const VocabString *words,
		  VocabIndex *wids, unsigned int max,
		  VocabIndex unkIndex)
{
    unsigned int i;

    for (i = 0; i < max && words[i] != 0; i++) {
	wids[i] = getIndex(words[i], unkIndex);
    }
    if (i < max) {
	wids[i] = Vocab_None;
    }
    return i;
}

// parse strings into words and update stats
unsigned int
Vocab::parseWords(char *sentence, VocabString *words, unsigned int max)
{
    char *word;
    int i = 0;

    for (word = strtok(sentence, wordSeparators);
	 i < max && word != 0;
	 i++, word = strtok(0, wordSeparators))
    {
	words[i] = word;
    }
    if (i < max) {
	words[i] = 0;
    }

    return i;
}

/*
 * Length of Ngrams
 */
unsigned int
Vocab::length(const VocabIndex *words)
{
    unsigned int len = 0;

    while (words[len] != Vocab_None) len++;
    return len;
}

unsigned int
Vocab::length(const VocabString *words)
{
    unsigned int len = 0;

    while (words[len] != 0) len++;
    return len;
}

/*
 * Copying (a la strcpy())
 */
VocabIndex *
Vocab::copy(VocabIndex *to, const VocabIndex *from)
{
    unsigned i;
    for (i = 0; from[i] != Vocab_None; i ++) {
	to[i] = from[i];
    }
    to[i] = Vocab_None;

    return to;
}

VocabString *
Vocab::copy(VocabString *to, const VocabString *from)
{
    unsigned i;
    for (i = 0; from[i] != 0; i ++) {
	to[i] = from[i];
    }
    to[i] = 0;

    return to;
}

/*
 * Word containment
 */
Boolean
Vocab::contains(const VocabIndex *words, VocabIndex word)
{
    unsigned i;

    for (i = 0; words[i] != Vocab_None; i++) {
	if (words[i] == word) {
	    return true;
	}
    }
    return false;
}

/*
 * Reversal of Ngrams
 */
VocabIndex *
Vocab::reverse(VocabIndex *words)
{
    int i, j;	/* j can get negative ! */

    for (i = 0, j = length(words) - 1;
	 i < j;
	 i++, j--)
    {
	VocabIndex x = words[i];
	words[i] = words[j];
	words[j] = x;
    }
    return words;
}

VocabString *
Vocab::reverse(VocabString *words)
{
    int i, j;	/* j can get negative ! */

    for (i = 0, j = length(words) - 1;
	 i < j;
	 i++, j--)
    {
	VocabString x = words[i];
	words[i] = words[j];
	words[j] = x;
    }
    return words;
}

/*
 * Output of Ngrams
 */
Vocab *Vocab::outputVocab;	/* vocabulary implicitly used by operator<< */

void
Vocab::write(File &file, const VocabString *words)
{
    for (unsigned int i = 0; words[i] != 0; i++) {
	fprintf(file, "%s%s", (i > 0 ? " " : ""), words[i]);
    }
}

ostream &
operator<< (ostream &stream, const VocabString *words)
{
    for (unsigned int i = 0; words[i] != 0; i++) {
	stream << (i > 0 ? " " : "") << words[i];
    }
    return stream;
}

ostream &
operator<< (ostream &stream, const VocabIndex *words)
{
    for (unsigned int i = 0; words[i] != Vocab_None; i++) {
	VocabString word = Vocab::outputVocab->getWord(words[i]);

	stream << (i > 0 ? " " : "")
	       << (word ? word : "UNKNOWN");
    }
    return stream;
}

/* 
 * Sorting of words and word sequences
 */
Vocab *Vocab::compareVocab;	/* implicit parameter to compare() */

// compare to word indices by their associated word strings
// This should be a non-static member, so we don't have to pass the
// Vocab in a global variable, but then we couldn't use this function
// with qsort() and friends.
// If compareVocab == 0 then comparison by index is performed.
int
Vocab::compare(VocabIndex word1, VocabIndex word2)
{
    if (compareVocab == 0) {
	return word2 - word1;
    } else {
	return strcmp(compareVocab->getWord(word1),
		      compareVocab->getWord(word2));
    }
}

int
Vocab::compare(const VocabString *words1, const VocabString *words2)
{
    unsigned int i = 0;

    for (i = 0; ; i++) {
	if (words1[i] == 0) {
	    if (words2[i] == 0) {
		return 0;
	    } else {
		return -1;	/* words1 is shorter */
	    }
	} else {
	    if (words2[i] == 0) {
		return 1;	/* words2 is shorted */
	    } else {
		int comp = compare(words1[i], words2[i]);
		if (comp != 0) {
		    return comp;	/* they differ as pos i */
		}
	    }
	}
    }
    /*NOTREACHED*/
}

int
Vocab::compare(const VocabIndex *words1, const VocabIndex *words2)
{
    unsigned int i = 0;

    for (i = 0; ; i++) {
	if (words1[i] == Vocab_None) {
	    if (words2[i] == Vocab_None) {
		return 0;
	    } else {
		return -1;	/* words1 is shorter */
	    }
	} else {
	    if (words2[i] == Vocab_None) {
		return 1;	/* words2 is shorted */
	    } else {
		int comp = compare(words1[i], words2[i]);
		if (comp != 0) {
		    return comp;	/* they differ as pos i */
		}
	    }
	}
    }
    /*NOTREACHED*/
}

/*
 * These are convenience methods which set the implicit Vocab used
 * by the comparison functions and returns a pointer to them.
 * Suitable to generate the 'sort' argument used by the iterators.
 */
VocabIndexComparator
Vocab::compareIndex() const
{
    compareVocab = (Vocab *)this;	// discard const
    return &Vocab::compare;
}

VocabIndicesComparator
Vocab::compareIndices() const
{
    compareVocab = (Vocab *)this;	// discard const
    return &Vocab::compare;
}

// Write vocabulary to file
void
Vocab::write(File &file, Boolean sorted) const
{
    VocabIter iter(*this, sorted);
    VocabString word;

    while (!file.error() && (word = iter.next())) {
	fprintf(file, "%s\n", word);
    }
}

// Read vocabulary from file
unsigned int
Vocab::read(File &file)
{
    char *line;
    unsigned int howmany = 0;

    while (line = file.getline()) {
	/*
	 * getline() returns only non-empty lines, so strtok()
	 * will find at least one word.  Any further ones on that
	 * are ignored.
	 */
	VocabString word = strtok(line, wordSeparators);

	if (addWord(word) == Vocab_None) {
	    file.position() << "warning: failed to add " << word
			    << " to vocabulary\n";
	    continue;
	}
	howmany++;
    }
    return howmany;
}

VocabIndex
Vocab::highIndex() const
{
    if (nextIndex == 0) {
	return Vocab_None;
    } else {
	return nextIndex - 1;
    }
}

/*
 * Iteration
 */
VocabIter::VocabIter(const Vocab &vocab, Boolean sorted)
    : myIter(vocab.byName, !sorted ? 0 : (int(*)(const char*,const char*))strcmp)
{
}

void
VocabIter::init()
{
    myIter.init();
}

VocabString
VocabIter::next(VocabIndex &index)
{
    VocabString word;
    VocabIndex *idx;
    if (idx = myIter.next(word)) {
	index = *idx;
	return word;
    } else {
	return 0;
    }
}

