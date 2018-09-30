/*
 * LoglinearMix.cc --
 *	Log-linear mixture language model
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2005 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/LoglinearMix.cc,v 1.2 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;
#include <stdlib.h>
#include <math.h>

#include "LoglinearMix.h"

#include "Trie.cc"

/*
 * Debug levels used
 */
#define DEBUG_NGRAM_HITS 2

LoglinearMix::LoglinearMix(Vocab &vocab, LM &lm1, LM &lm2, Prob prior)
    : LM(vocab), lm1(lm1), lm2(lm2), prior(prior)
{
    if (prior < 0.0 || prior > 1.0) {
	cerr << "warning: mixture prior out of range: " << prior << endl;
	prior = 0.5;
    }
}

LogP
LoglinearMix::wordProb(VocabIndex word, const VocabIndex *context)
{
    /* 
     * Truncate context to used length, for denominator caching.
     * By definition, the wordProb computation won't be affected by this.
     */
    unsigned usedContextLength;
    contextID(Vocab_None, context, usedContextLength);
    VocabIndex saved = context[usedContextLength];
    ((VocabIndex *)context)[usedContextLength] = Vocab_None;

    LogP prob1 = lm1.wordProb(word, context);
    LogP prob2 = lm2.wordProb(word, context);

    LogP numerator = prior * prob1 + (1.0 - prior) * prob2;

    Boolean foundp;
    LogP *denominator = denomProbs.insert(context, foundp);

    /*
     * *denominator will be 0 for lower-order N-grams that have been created
     * as a side-effect of inserting higher-order N-grams. Hence we 
     * don't trust them as cached values.  Hopefully denominator = 0 will be
     * rare, so we don't lose much cache efficiency due to this.
     */
    if (foundp && *denominator != 0.0) {
	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "[cached=" << LogPtoProb(*denominator) << "]";
	}
    } else {
	/*
	 * interrupt sequential processing mode
	 */
	Boolean wasRunning1 = lm1.running(false);
	Boolean wasRunning2 = lm2.running(false);

	/*
	 * Compute denominator by summing over all words in context
	 */
	Prob sum = 0.0;

	VocabIter iter(vocab);
	VocabIndex wid;

	while (iter.next(wid)) {
	    if (!lm1.isNonWord(wid)) {
		/*
		 * Use wordProbRecompute() here since the context stays
		 * the same and it might save work.
		 */
		LogP prob1 = lm1.wordProbRecompute(wid, context);
		LogP prob2 = lm2.wordProbRecompute(wid, context);

		sum += LogPtoProb(prior * prob1 + (1.0 - prior) * prob2);
	    }
	}

	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "[denom=" << sum << "]";
	}

	*denominator = ProbToLogP(sum);

	lm1.running(wasRunning1);
	lm2.running(wasRunning2);
    }

    ((VocabIndex *)context)[usedContextLength] = saved;

    return numerator - *denominator;
}

void *
LoglinearMix::contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length)
{
    /*
     * Return the context ID of the component model that uses the longer
     * context. We must use longest context regardless of predicted word
     * because mixture models don't support contextBOW().
     */
    unsigned len1, len2;

    void *cid1 = lm1.contextID(context, len1);
    void *cid2 = lm2.contextID(context, len2);

    if (len2 > len1) {
	length = len2;
	return cid2;
    } else {
	length = len1;
	return cid1;
    }
}

Boolean
LoglinearMix::isNonWord(VocabIndex word)
{
    /*
     * A non-word in either of our component models is a non-word.
     * This ensures that state names, hidden vocabulary, etc. are not
     * treated as regular words in the respectively other component.
     */
    return lm1.isNonWord(word) || lm2.isNonWord(word);
}

void
LoglinearMix::setState(const char *state)
{
    /*
     * Global state changes are propagated to the component models
     */
    lm1.setState(state);
    lm2.setState(state);
}

