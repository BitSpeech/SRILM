/*
 * LM.cc --
 *	Generic LM methods
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/LM.cc,v 1.49 2006/01/05 08:44:25 stolcke Exp $";
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

extern "C" {
    double drand48();		/* might be missing from math.h or stdlib.h */
}

#include "LM.h"
#include "NgramStats.h"
#include "NBest.h"
#include "Array.cc"

/*
 * Debugging levels used in this file
 */
#define DEBUG_PRINT_DOC_PROBS		0
#define DEBUG_PRINT_SENT_PROBS		1
#define DEBUG_PRINT_WORD_PROBS		2
#define DEBUG_PRINT_PROB_SUMS		3

const char *defaultStateTag = "<LMstate>";

/*
 * Initialization
 *	The LM is created with a reference to a Vocab, so various
 *	LMs and other objects can share one Vocab.  The LM will typically
 *	add words to the Vocab as needed.
 */
LM::LM(Vocab &vocab)
    : vocab(vocab), noiseVocab(vocab)
{
    _running = false;
    reverseWords = false;
    stateTag = defaultStateTag;
}

LM::~LM()
{
}

/*
 * Contextual word probabilities from strings
 *	The default method for word strings looks up the word indices
 *	for both the word and its context and gets its probabilities
 *	from the LM.
 */
LogP
LM::wordProb(VocabString word, const VocabString *context)
{
    unsigned int len = vocab.length(context);
    makeArray(VocabIndex, cids, len + 1);
    vocab.getIndices(context, cids, len + 1, vocab.unkIndex());

    LogP prob = wordProb(vocab.getIndex(word, vocab.unkIndex()), cids);

    return prob;
}

/* Word probability with cached context
 *	Recomputes the conditional probability of a word using a context
 *	that is guaranteed to be identical to the last call to wordProb.
 * This implementation compute prob from scratch, but the idea is that
 * other language models use caches that depend on the context.
 */
LogP
LM::wordProbRecompute(VocabIndex word, const VocabIndex *context)
{
    return wordProb(word, context);
}

/*
 * Non-word testing
 *	Returns true for pseudo-word tokens that don't correspond to
 *	observable events (e.g., context tags or hidden events).
 */
Boolean
LM::isNonWord(VocabIndex word)
{
    return vocab.isNonEvent(word);
}

/*
 * Total probabilites
 *	For debugging purposes, compute the sum of all word probs
 *	in a context.
 */
Prob
LM::wordProbSum(const VocabIndex *context)
{
    double total = 0.0;
    VocabIter iter(vocab);
    VocabIndex wid;
    Boolean first = true;

    /*
     * prob summing interrupts sequential processing mode
     */
    Boolean wasRunning = running(false);

    while (iter.next(wid)) {
	if (!isNonWord(wid)) {
	    total += LogPtoProb(first ?
				wordProb(wid, context) :
				wordProbRecompute(wid, context));
	    first = false;
	}
    }

    running(wasRunning);
    return total;
}

/*
 * Sentence probabilities from strings
 *	The default method for sentences of word strings is to translate
 *	them to word index sequences and get its probability from the LM.
 */
LogP
LM::sentenceProb(const VocabString *sentence, TextStats &stats)
{
    unsigned int len = vocab.length(sentence);
    makeArray(VocabIndex, wids, len + 1);
    vocab.getIndices(sentence, wids, len + 1, vocab.unkIndex());

    LogP prob = sentenceProb(wids, stats);

    return prob;
}

/*
 * Convenience function that reverses a sentence (for wordProb computation),
 * adds begin/end sentence tokens, and removes pause tokens.
 * It returns the number of words excluding these special tokens.
 */
unsigned
LM::prepareSentence(const VocabIndex *sentence, VocabIndex *reversed,
								unsigned len)
{
    unsigned i, j = 0;

    /*
     * Add </s> token if now already there.
     */
    if (sentence[reverseWords ? 0 : len - 1] != vocab.seIndex()) {
	reversed[j++] = vocab.seIndex();
    }

    for (i = 1; i <= len; i++) {
	VocabIndex word = sentence[reverseWords ? i - 1 : len - i];

	if (word == vocab.pauseIndex() || noiseVocab.getWord(word)) {
	    continue;
	}

	reversed[j++] = word;
    }

    /*
     * Add <s> token if not already there
     */
    if (sentence[reverseWords ? len - 1 : 0] != vocab.ssIndex()) {
	reversed[j++] = vocab.ssIndex();
    }
    reversed[j] = Vocab_None;

    return j - 2;
}

/*
 * Convenience functions that strips noise and pause tags from a words string
 */
VocabIndex *
LM::removeNoise(VocabIndex *words)
{
    unsigned from, to;

    for (from = 0, to = 0; words[from] != Vocab_None; from ++) {
	if (words[from] != vocab.pauseIndex() &&
	    !noiseVocab.getWord(words[from]))
	{
	    words[to++] = words[from];
	}
    }
    words[to] = Vocab_None;

    return words;
}

/*
 * Sentence probabilities from indices
 *	The default method is to accumulate the contextual word
 *	probabilities including that of the sentence end.
 */
LogP
LM::sentenceProb(const VocabIndex *sentence, TextStats &stats)
{
    unsigned int len = vocab.length(sentence);
    makeArray(VocabIndex, reversed, len + 2 + 1);
    int i;

    /*
     * Indicate to lm methods that we're in sequential processing
     * mode.
     */
    Boolean wasRunning = running(true);

    /*
     * Contexts are represented most-recent-word-first.
     * Also, we have to prepend the sentence-begin token,
     * and append the sentence-end token.
     */
    len = prepareSentence(sentence, reversed, len);

    LogP totalProb = 0.0;
    unsigned totalOOVs = 0;
    unsigned totalZeros = 0;

    for (i = len; i >= 0; i--) {
	LogP probSum;

	if (debug(DEBUG_PRINT_WORD_PROBS)) {
	    dout() << "\tp( " << vocab.getWord(reversed[i]) << " | "
		   << vocab.getWord(reversed[i+1])
		   << (i < len ? " ..." : " ") << ") \t= " ;

	    if (debug(DEBUG_PRINT_PROB_SUMS)) {
		/*
		 * XXX: because wordProb can change the state of the LM
		 * we need to compute wordProbSum first.
		 */
		probSum = wordProbSum(&reversed[i + 1]);
	    }
	}

	LogP prob = wordProb(reversed[i], &reversed[i + 1]);

	if (debug(DEBUG_PRINT_WORD_PROBS)) {
	    dout() << " " << LogPtoProb(prob) << " [ " << prob << " ]";
	    if (debug(DEBUG_PRINT_PROB_SUMS)) {
		dout() << " / " << probSum;
		if (fabs(probSum - 1.0) > 0.0001) {
		    cerr << "\nwarning: word probs for this context sum to "
			 << probSum << " != 1 : " 
			 << (vocab.use(), &reversed[i + 1]) << endl;
		}
	    }
	    dout() << endl;
	}
	/*
	 * If the probability returned is zero but the
	 * word in question is <unk> we assume this is closed-vocab
	 * model and count it as an OOV.  (This allows open-vocab
	 * models to return regular probabilties for <unk>.)
	 * If this happens and the word is not <unk> then we are
	 * dealing with a broken language model that return
	 * zero probabilities for known words, and we count them
	 * as a "zeroProb".
	 */
	if (prob == LogP_Zero) {
	    if (reversed[i] == vocab.unkIndex()) {
		totalOOVs ++;
	    } else {
		totalZeros ++;
	    }
	} else {
	    totalProb += prob;
	}
    }

    running(wasRunning);

    /*
     * Update stats with this sentence
     */
    stats.numSentences ++;
    stats.numWords += len;
    stats.numOOVs += totalOOVs;
    stats.zeroProbs += totalZeros;
    stats.prob += totalProb;

    return totalProb;
}

/*
 * Compute joint probability of a word context (a reversed word sequence)
 */
LogP
LM::contextProb(const VocabIndex *context, unsigned clength)
{
    unsigned useLength = Vocab::length(context);
    LogP jointProb = LogP_One;

    if (clength < useLength) {
	useLength = clength;
    }

    /*
     * If the context is empty there is nothing left to do: return LogP_One 
     */
    if (useLength > 0) {
	/*
	 * Turn off debugging for contextProb computation
	 */
	Boolean wasRunning = running(false);

	/*
	 * The usual hack: truncate the context temporarily
	 */
	VocabIndex saved = context[useLength];
	((VocabIndex *)context)[useLength] = Vocab_None;

	/*
	 * Accumulate conditional probs for all words in context
	 */
	for (unsigned i = useLength; i > 0; i--) {
	    VocabIndex word = context[i - 1];

	    /*
	     * If we're computing the marginal probability of the unigram
	     * <s> context we have to look up </s> instead since the former
	     * has prob = 0.
	     */
	    if (i == useLength && word == vocab.ssIndex()) {
		word = vocab.seIndex();
	    }

	    LogP wprob = wordProb(word, &context[i]);

	    /*
	     * If word is a non-event it has probability zero in the model,
	     * so the best we can do is to skip it.
	     * Note that above mapping turns <s> into a non-non-event, so
	     * it will be included.
	     */
	    if (wprob != LogP_Zero || !vocab.isNonEvent(word)) {
		jointProb += wprob;
	    }
	}

	((VocabIndex *)context)[useLength] = saved;

	running(wasRunning);
    }

    return jointProb;
}

/*
 * Compute an aggregate log probability, perplexity, etc., much like
 * sentenceProb, except that it uses counts instead of actual 
 * sentences.
 */
LogP
LM::countsProb(NgramStats &counts, TextStats &stats, unsigned countorder,
							    Boolean entropy)
{
    makeArray(VocabIndex, ngram, countorder + 1);

    LogP totalProb = 0.0;

    /*
     * Indicate to lm methods that we're in sequential processing
     * mode.
     */
    Boolean wasRunning = running(true);

    /*
     * Enumerate all counts up the order indicated
     */
    for (unsigned i = 1; i <= countorder; i++ ) {
	// use sorted enumeration in debug mode only
	NgramsIter ngramIter(counts, ngram, i,
					!debug(DEBUG_PRINT_WORD_PROBS) ? 0 :
							vocab.compareIndex());

	NgramCount *count;

	/*
	 * This enumerates all ngrams of the given order
	 */
	while (count = ngramIter.next()) {
	    TextStats ngramStats;

	    /*
	     * Skip zero counts since they don't contribute anything to
	     * the probability
	     */
	    if (*count == 0) {
		continue;
	    }

	    /*
	     * reverse ngram for lookup
	     */
	    Vocab::reverse(ngram);

	    /*
	     * The rest of this loop is patterned after LM::sentenceProb()
	     */

	    if (debug(DEBUG_PRINT_WORD_PROBS)) {
		dout() << "\tp( " << vocab.getWord(ngram[0]) << " | "
		       << (vocab.use(), &ngram[1])
		       << " ) \t= " ;
	    }
	    LogP prob = wordProb(ngram[0], &ngram[1]);

	    LogP jointProb = !entropy ? LogP_One :
					contextProb(ngram, countorder);
	    Prob weight = *count * LogPtoProb(jointProb);

	    if (debug(DEBUG_PRINT_WORD_PROBS)) {
		dout() << " " << LogPtoProb(prob) << " [ " << prob;

		/* 
		 * Include ngram count if not unity, so we can compute the
		 * aggregate log probability from the output
		 */
		if (weight != 1.0) {
			dout() << " *" << weight;
		}
		dout() << " ]";

		if (debug(DEBUG_PRINT_PROB_SUMS)) {
		    Prob probSum = wordProbSum(&ngram[1]);
		    dout() << " / " << probSum;
		    if (fabs(probSum - 1.0) > 0.0001) {
			cerr << "\nwarning: word probs for this context sum to "
			     << probSum << " != 1 : " 
			     << (vocab.use(), &ngram[1]) << endl;
		    }
		}
		dout() << endl;
	    }

	    /*
	     * ngrams ending in </s> are counted as sentences, all others
	     * as words.  This keeps the output compatible with that of
	     * LM::pplFile().
	     */
	    if (ngram[0] == vocab.seIndex()) {
		ngramStats.numSentences = *count;
	    } else {
		ngramStats.numWords = *count;
	    }

	    /*
	     * If the probability returned is zero but the
	     * word in question is <unk> we assume this is closed-vocab
	     * model and count it as an OOV.  (This allows open-vocab
	     * models to return regular probabilties for <unk>.)
	     * If this happens and the word is not <unk> then we are
	     * dealing with a broken language model that return
	     * zero probabilities for known words, and we count them
	     * as a "zeroProb".
	     */
	    if (prob == LogP_Zero) {
		if (ngram[0] == vocab.unkIndex()) {
		    ngramStats.numOOVs = *count;
		} else {
		    ngramStats.zeroProbs = *count;
		}
	    } else {
		totalProb +=
		    (ngramStats.prob = weight * prob);
	    }

	    stats.increment(ngramStats);

	    Vocab::reverse(ngram);
	}
    }

    running(wasRunning);

    /* 
     * If computing entropy set total number of events to 1 so that 
     * ppl computation reflects entropy.
     */
    if (entropy) {
	stats.numSentences = 0;
	stats.numWords = 1;
    }

    return totalProb;
}

/*
 * Perplexity from counts
 *	The escapeString is an optional line prefix that marks information
 *	that should be passed through unchanged.  This is useful in
 *	constructing rescoring filters that feed hypothesis strings to
 *	pplCountsFile(), but also need to pass other information to downstream
 *	processing.
 *	If the entropy flag is true, the count log probabilities will be 
 *	weighted by the joint probabilities on the ngrams.  I.e., the
 *	output will be p(w,h) log p(pw|h) for each ngram, and the overall 
 *	result will be the entropy of the conditional N-gram distribution.
 */
unsigned int
LM::pplCountsFile(File &file, unsigned order, TextStats &stats,
				    const char *escapeString, Boolean entropy)
{
    char *line;
    unsigned escapeLen = escapeString ? strlen(escapeString) : 0;
    unsigned stateTagLen = stateTag ? strlen(stateTag) : 0;

    VocabString words[maxNgramOrder + 1];
    makeArray(VocabIndex, wids, order + 1);
    NgramStats *counts = 0;
    TextStats sentenceStats;

    while (line = file.getline()) {

	if (escapeString && strncmp(line, escapeString, escapeLen) == 0) {
	    /*
	     * Output sentence-level statistics before each escaped line
	     */
	    if (counts) {
		countsProb(*counts, sentenceStats, order, entropy);

		if (debug(DEBUG_PRINT_SENT_PROBS)) {
		    dout() << sentenceStats << endl;
		}

		stats.increment(sentenceStats);
		sentenceStats.reset();

		delete counts;
		counts = 0;
	    }
	    dout() << line;
	    continue;
	}

	/*
	 * check for directives to change the global LM state
	 */
	if (stateTag && strncmp(line, stateTag, stateTagLen) == 0) {
	    /*
	     * pass the state info the lm to let it do whatever
	     * it wants with it
	     */
	    setState(&line[stateTagLen]);
	    continue;
	}

	if (!counts) {
	    counts = new NgramStats(vocab, order);
	    assert(counts != 0);
	}

	NgramCount count;
	unsigned howmany =
		    counts->parseNgram(line, words, maxNgramOrder + 1, count);

	/*
	 * Skip this entry if the length of the ngram exceeds our 
	 * maximum order
	 */
	if (howmany == 0) {
	    file.position() << "malformed N-gram count or more than "
			    << maxNgramOrder << " words per line\n";
	    continue;
	} else if (howmany > order) {
	    continue;
	}

	/* 
	 * Map words to indices
	 */
	vocab.getIndices(words, wids, order + 1, vocab.unkIndex());

	/*
	 *  Update the counts
	 */
	*counts->insertCount(wids) += count;
    }

    /* 
     * Output and update final sentence-level statistics
     */
    if (counts) {
	countsProb(*counts, sentenceStats, order, entropy);

	if (debug(DEBUG_PRINT_SENT_PROBS)) {
	    dout() << sentenceStats << endl;
	}

	stats.increment(sentenceStats);
	delete counts;
    }

    return stats.numWords;
}

/*
 * Perplexity from text
 *	The escapeString is an optional line prefix that marks information
 *	that should be passed through unchanged.  This is useful in
 *	constructing rescoring filters that feed hypothesis strings to
 *	pplFile(), but also need to pass other information to downstream
 *	processing.
 */
unsigned int
LM::pplFile(File &file, TextStats &stats, const char *escapeString)
{
    char *line;
    unsigned escapeLen = escapeString ? strlen(escapeString) : 0;
    unsigned stateTagLen = stateTag ? strlen(stateTag) : 0;
    VocabString sentence[maxWordsPerLine + 1];
    unsigned totalWords = 0;
    unsigned sentNo = 0;
    TextStats documentStats;
    Boolean printDocumentStats = false;

    while (line = file.getline()) {

	if (escapeString && strncmp(line, escapeString, escapeLen) == 0) {
            if (sentNo > 0 && debuglevel() == DEBUG_PRINT_DOC_PROBS) {
		dout() << documentStats << endl;
		documentStats.reset();
		printDocumentStats = true;
            }
	    dout() << line;
	    continue;
	}

	/*
	 * check for directives to change the global LM state
	 */
	if (stateTag && strncmp(line, stateTag, stateTagLen) == 0) {
	    /*
	     * pass the state info the lm to let it do whatever
	     * it wants with it
	     */
	    setState(&line[stateTagLen]);
	    continue;
	}

	sentNo ++;

	unsigned int numWords =
			vocab.parseWords(line, sentence, maxWordsPerLine + 1);

	if (numWords == maxWordsPerLine + 1) {
	    file.position() << "too many words per sentence\n";
	} else {
	    TextStats sentenceStats;

	    if (debug(DEBUG_PRINT_SENT_PROBS)) {
		dout() << sentence << endl;
	    }
	    LogP prob = sentenceProb(sentence, sentenceStats);

	    totalWords += numWords;

	    if (debug(DEBUG_PRINT_SENT_PROBS)) {
		dout() << sentenceStats << endl;
	    }

	    stats.increment(sentenceStats);
	    documentStats.increment(sentenceStats);
	}
    }

    if (printDocumentStats) {
	dout() << documentStats << endl;
    }

    return totalWords;
}

unsigned
LM::rescoreFile(File &file, double lmScale, double wtScale,
		   LM &oldLM, double oldLmScale, double oldWtScale,
		   const char *escapeString)
{
    char *line;
    unsigned escapeLen = escapeString ? strlen(escapeString) : 0;
    unsigned stateTagLen = stateTag ? strlen(stateTag) : 0;
    unsigned sentNo = 0;

    while (line = file.getline()) {

	if (escapeString && strncmp(line, escapeString, escapeLen) == 0) {
	    fputs(line, stdout);
	    continue;
	}

	/*
	 * check for directives to change the global LM state
	 */
	if (stateTag && strncmp(line, stateTag, stateTagLen) == 0) {
	    /*
	     * pass the state info the lm to let let if do whatever
	     * it wants with it
	     */
	    setState(&line[stateTagLen]);
	    continue;
	}

	sentNo ++;

	/*
	 * parse an n-best hyp from this line
	 */
	NBestHyp hyp;

	if (!hyp.parse(line, vocab)) {
	    file.position() << "bad n-best hyp format\n";
	} else {
	    hyp.decipherFix(oldLM, oldLmScale, oldWtScale);
	    hyp.rescore(*this, lmScale, wtScale);
	    // hyp.write((File)stdout, vocab);
	    /*
	     * Instead of writing only the total score back to output,
	     * keep all three scores: acoustic, LM, word transition penalty.
	     * Also, write this in straight log probs, not bytelog.
	     */
	    fprintf(stdout, "%g %g %d",
			hyp.acousticScore, hyp.languageScore, hyp.numWords);
	    for (unsigned i = 0; hyp.words[i] != Vocab_None; i++) {
		fprintf(stdout, " %s", vocab.getWord(hyp.words[i]));
	    }
	    fprintf(stdout, "\n");
	}
    }
    return sentNo;
}

/*
 * Random sample generation
 */
VocabIndex
LM::generateWord(const VocabIndex *context)
{
    /*
     * Algorithm: generate random number between 0 and 1, and partition
     * the interval 0..1 into pieces corresponding to the word probs.
     * Chose the word whose interval contains the random value.
     */
    Prob rval = drand48();
    Prob totalProb = 0.0;

    VocabIter iter(vocab);
    VocabIndex wid;

    while (totalProb <= rval && iter.next(wid)) {
	if (!isNonWord(wid)) {
	    totalProb += LogPtoProb(wordProb(wid, context));
	}
    }
    return wid;
}

VocabIndex *
LM::generateSentence(unsigned maxWords, VocabIndex *sentence)
{
    static unsigned defaultResultSize = 0;
    static VocabIndex *defaultResult = 0;

    /*
     * If no result buffer is supplied use our own.
     */
    if (sentence == 0) {
	if (maxWords + 1 > defaultResultSize) {
	    defaultResultSize = maxWords + 1;
	    if (defaultResult) {
		delete defaultResult;
	    }
	    defaultResult = new VocabIndex[defaultResultSize];
	    assert(defaultResult != 0);
	}
	sentence = defaultResult;
    }

    /*
     * Since we need to add the begin/end sentences tokens, and
     * partial contexts are represented in reverse we use a second
     * buffer for partial sentences.
     */
    makeArray(VocabIndex, genBuffer, maxWords + 3);

    unsigned last = maxWords + 2;
    genBuffer[last] = Vocab_None;
    genBuffer[--last] = vocab.ssIndex();

    /*
     * Generate words one-by-one until hitting an end-of-sentence.
     */
    while (last > 0 && genBuffer[last] != vocab.seIndex()) {
	last --;
	genBuffer[last] = generateWord(&genBuffer[last + 1]);
    }
    
    /*
     * Copy reversed sentence to output buffer
     */
    unsigned i, j;
    for (i = 0, j = maxWords; j > last; i++, j--) {
	sentence[i] = genBuffer[j];
    }
    sentence[i] = Vocab_None;

    return sentence;
}

VocabString *
LM::generateSentence(unsigned maxWords, VocabString *sentence)
{
    static unsigned defaultResultSize = 0;
    static VocabString *defaultResult = 0;

    /*
     * If no result buffer is supplied use our own.
     */
    if (sentence == 0) {
	if (maxWords + 1 > defaultResultSize) {
	    defaultResultSize = maxWords + 1;
	    if (defaultResult) {
		delete defaultResult;
	    }
	    defaultResult = new VocabString[defaultResultSize];
	    assert(defaultResult != 0);
	}
	sentence = defaultResult;
    }

    /*
     * Generate words indices, then map them to strings
     */
    vocab.getWords(generateSentence(maxWords, (VocabIndex *)0),
			    sentence, maxWords + 1);

    return sentence;
}

/*
 * Context identification
 *	This returns a unique ID for the portion of a context used in
 *	computing follow-word probabilities. Used for path merging in
 *	lattice search (see the HTK interface).
 *	The length parameter returns the number of words used in context.
 *	The default is to return 0, to indicate all contexts are unique.
 */
void *
LM::contextID(VocabIndex word, const VocabIndex *context, unsigned &length)
{
    length = Vocab::length(context);
    return 0;
}

/*
 * Back-off weight
 *	Computes the backoff weight applied to probabilities that are 
 *	computed from a truncated context.  Used for weight computation in
 *	lattice expansion (see Lattice::expandNodeToLM()).
 */
LogP
LM::contextBOW(const VocabIndex *context, unsigned length)
{
    return LogP_One;
}

/*
 * Global state changes (ignored)
 */
void
LM::setState(const char *state)
{
}

/*
 * LM reading/writing (dummy)
 */
Boolean
LM::read(File &file, Boolean limitVocab)
{
    cerr << "read() method not implemented\n";
    return false;
}

void
LM::write(File &file)
{
    cerr << "write() method not implemented\n";
}

/*
 * Memory statistics
 */
void
LM::memStats(MemStats &stats)
{
    stats.total += sizeof(*this);
}

/*
 * Iteration over follow words
 *	The generic follow-word iterator enumerates all of vocab.
 */

_LM_FollowIter::_LM_FollowIter(LM &lm, const VocabIndex *context)
    : myLM(lm), myContext(context), myIter(lm.vocab)
{
}

void
_LM_FollowIter::init()
{
    myIter.init();
}

VocabIndex
_LM_FollowIter::next()
{
    VocabIndex index = Vocab_None;
    (void)myIter.next(index);
    return index;
}

VocabIndex
_LM_FollowIter::next(LogP &prob)
{
    VocabIndex index = Vocab_None;
    (void)myIter.next(index);
    
    if (index != Vocab_None) {
	prob = myLM.wordProb(index, myContext);
    }

    return index;
}

