/*
 * AdaptiveMarginals.cc --
 *	Adaptive marginals language model (Kneser et al, Eurospeech 97)
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2004 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/AdaptiveMarginals.cc,v 1.3 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;
#include <stdlib.h>
#include <math.h>

#include "AdaptiveMarginals.h"

#include "LHash.cc"
#include "Trie.cc"

/*
 * Debug levels used
 */
#define DEBUG_ADAPT_RATIOS	2

AdaptiveMarginals::AdaptiveMarginals(Vocab &vocab, LM &baseLM,
			    LM &baseMarginals, LM&adaptMarginals, double beta)
    : LM(vocab), baseLM(baseLM), baseMarginals(baseMarginals),
      adaptMarginals(adaptMarginals), beta(beta),
      computeRatios(false), haveAlphas(false)
{
    if (beta < 0.0 || beta > 1.0) {
	cerr << "warning: adaptation weight out of range: " << beta << endl;
    }
}

/*
 * Precompute adaptation alpha values for all words
 */
void
AdaptiveMarginals::computeAlphas()
{
    VocabIndex emptyContext[1];
    emptyContext[0] = Vocab_None;

    /*
     * interrupt sequential processing mode
     */
    Boolean wasRunning1 = baseMarginals.running(false);
    Boolean wasRunning2 = adaptMarginals.running(false);

    VocabIter iter(vocab);
    VocabIndex wid;

    while (iter.next(wid)) {
	LogP adaptedProb = adaptMarginals.wordProb(wid, emptyContext);
	LogP baseProb = baseMarginals.wordProb(wid, emptyContext);
	
	LogP alpha;
	if (baseProb == LogP_Zero || adaptedProb == LogP_Zero) {
	    if (beta == 0.0) {
	    	alpha = LogP_One;
	    } else {
		alpha = LogP_Zero;
	    }
	} else {
	    alpha = beta * (adaptedProb - baseProb);
	}

	*adaptAlphas.insert(wid) = alpha;
    }

    baseMarginals.running(wasRunning1);
    adaptMarginals.running(wasRunning2);
    
    haveAlphas = true;
}

LogP
AdaptiveMarginals::wordProb(VocabIndex word, const VocabIndex *context)
{
    if (!haveAlphas) {
	computeAlphas();
    }

    LogP *alpha = adaptAlphas.find(word);
    assert(alpha != 0);

    /* 
     * Truncate context to used length, for denominator caching.
     * By definition, the wordProb computation won't be affected by this.
     */
    unsigned usedContextLength;
    baseLM.contextID(Vocab_None, context, usedContextLength);
    VocabIndex saved = context[usedContextLength];
    ((VocabIndex *)context)[usedContextLength] = Vocab_None;

    LogP numerator = baseLM.wordProb(word, context) + *alpha;

    if (running() && debug(DEBUG_ADAPT_RATIOS)) {
	dout() << "[alpha=" << LogPtoProb(*alpha) << "]";
    }

    Boolean foundp;
    LogP *denominator = denomProbs.insert(context, foundp);

    /*
     * *denominator will be 0 for lower-order N-grams that have been created
     * as a side-effect of inserting higher-order N-grams. Hence we 
     * don't trust them as cached values.  Hopefully denominator = 0 will be
     * rare, so we don't lose much cache efficiency due to this.
     */
    if (foundp && *denominator != 0.0) {
	if (running() && debug(DEBUG_ADAPT_RATIOS)) {
	    dout() << "[cached=" << LogPtoProb(*denominator) << "]";
	}
    } else {
	/*
	 * interrupt sequential processing mode
	 */
	Boolean wasRunning = baseLM.running(false);

	/*
	 * Compute denominator by summing over all words in context
	 */
	Prob sum = 0.0;

	VocabIter iter(vocab);
	VocabIndex wid;

	while (iter.next(wid)) {
	    if (!baseLM.isNonWord(wid)) {
		LogP *alpha2 = adaptAlphas.find(wid);
		assert(alpha2 != 0);

		/*
		 * Use wordProbRecompute() here since the context stays
		 * the same and it might save work.
		 */
		sum += LogPtoProb(baseLM.wordProbRecompute(wid, context) +
								    *alpha2);
	    }
	}

	if (running() && debug(DEBUG_ADAPT_RATIOS)) {
	    dout() << "[denom=" << sum << "]";
	}

	*denominator = ProbToLogP(sum);

	baseLM.running(wasRunning);
    }

    ((VocabIndex *)context)[usedContextLength] = saved;

    if (computeRatios) {
	return *alpha - *denominator;
    } else {
	return numerator - *denominator;
    }
}

void *
AdaptiveMarginals::contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length)
{
    /*
     * Context used is determined by the base LM alone.
     */
    return baseLM.contextID(word, context, length);
}

Boolean
AdaptiveMarginals::isNonWord(VocabIndex word)
{
    return baseLM.isNonWord(word);
}

void
AdaptiveMarginals::setState(const char *state)
{
    /*
     * Global state changes are propagated to the base models
     */
    baseLM.setState(state);
}

