/*
 * LoglinearMix.cc --
 *	Log-linear mixture language model
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2005-2010 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/lm/src/LoglinearMix.cc,v 1.7 2010/06/02 06:22:48 stolcke Exp $";
#endif

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <stdlib.h>
#include <math.h>

#include "LoglinearMix.h"

#include "Array.cc"
#include "Trie.cc"

/*
 * Debug levels used
 */
#define DEBUG_NGRAM_HITS 2

LoglinearMix::LoglinearMix(Vocab &vocab, LM &lm1, LM &lm2, Prob prior)
    : LM(vocab)
{
    if (prior < 0.0 || prior > 1.0) {
	cerr << "warning: mixture prior out of range: " << prior << endl;
	prior = 0.5;
    }

    numLMs = 2;
    subLMs[0] = &lm1;
    subLMs[1] = &lm2;

    priors[0] = prior;
    priors[1] = 1.0 - prior;
}

LoglinearMix::LoglinearMix(Vocab &vocab, Array<LM *> &subLMs,
							Array<Prob> &priors)
    : LM(vocab), numLMs(subLMs.size()), priors(priors), subLMs(subLMs)
{
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

    TruncatedContext usedContext(context, usedContextLength);

    LogP numerator = 0;
    for (int i = 0; i < numLMs; i++) {
	numerator += priors[i] * subLMs[i]->wordProb(word, context);
	if (numerator == LogP_Zero) {
	    break;
	}
    }

    Boolean foundp;
    LogP *denominator = denomProbs.insert(usedContext, foundp);

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
	makeArray(Boolean, wasRunning, numLMs);

	for (int i = 0; i < numLMs; i++) {
	    wasRunning[i] = subLMs[i]->running(false);
	}

	/*
	 * Compute denominator by summing over all words in context
	 */
	Prob sum = 0.0;

	VocabIter iter(vocab);
	VocabIndex wid;

	while (iter.next(wid)) {
	    if (!isNonWord(wid)) {
		/*
		 * Use wordProbRecompute() here since the context stays
		 * the same and it might save work.
		 */
		LogP probSum = 0;
		for (int i = 0; i < numLMs; i++) {
		    probSum += priors[i] *
				subLMs[i]->wordProbRecompute(wid, context);
		}

		sum += LogPtoProb(probSum);
	    }
	}

	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "[denom=" << sum << "]";
	}

	*denominator = ProbToLogP(sum);

	for (int i = 0; i < numLMs; i++) {
	    subLMs[i]->running(wasRunning[i]);
	}
    }

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
    unsigned maxLength = 0;
    void *maxCid = 0;

    for (int i = 0; i < numLMs; i++) {
	unsigned len;
	void *cid = subLMs[i]->contextID(context, len);
	if (len > maxLength) {
	    maxLength = len;
	    maxCid = cid;
	}
    }

    length = maxLength;
    return maxCid;
}

Boolean
LoglinearMix::isNonWord(VocabIndex word)
{
    /*
     * A non-word in either of our component models is a non-word.
     * This ensures that state names, hidden vocabulary, etc. are not
     * treated as regular words in the respectively other component.
     */
    for (int i = 0; i < numLMs; i++) {
	if (subLMs[i]->isNonWord(word)) {
	    return true;
	}
    }
    return false;
}

void
LoglinearMix::setState(const char *state)
{
    /*
     * Global state changes are propagated to the component models
     */
    for (int i = 0; i < numLMs; i++) {
	subLMs[i]->setState(state);
    }
}

Boolean
LoglinearMix::running(Boolean newstate)
{
    /*
     * Propagate changes to running state to component models
     */
    Boolean old = _running;
    _running = newstate;
    for (int i = 0; i < numLMs; i++) {
	subLMs[i]->running(newstate);
    }

    return old;
};

void
LoglinearMix::debugme(unsigned level) {
    /*
     * Propagate changes to Debug state to component models
     */
    for (int i = 0; i < numLMs; i++) {
	subLMs[i]->debugme(level);
    }

    Debug::debugme(level);
}

ostream &
LoglinearMix::dout(ostream &stream) {
    /*
     * Propagate dout changes to sub-lms
     */
    for (int i = 0; i < numLMs; i++) {
	subLMs[i]->dout(stream);
    }

    return Debug::dout(stream);
}

