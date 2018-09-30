/*
 * BayesMix.cc --
 *	Bayesian mixture language model
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /export/di/ws97/tools/srilm-0.97beta/lm/src/RCS/BayesMix.cc,v 1.7 1997/07/22 15:06:08 stolcke Exp $";
#endif

#include <iostream.h>
#include <stdlib.h>
#include <math.h>

#include "BayesMix.h"

/*
 * Debug levels used
 */
#define DEBUG_MIX_WEIGHTS	2

BayesMix::BayesMix(Vocab &vocab, LM &lm1, LM &lm2,
			    unsigned int clength, Prob prior, double llscale)
    : LM(vocab), lm1(lm1), lm2(lm2),
      clength(clength), prior(prior), llscale(llscale)
{
    if (prior < 0.0 || prior > 1.0) {
	cerr << "warning: mixture prior out of range: " << prior << endl;
	prior = 0.5;
    }
}


/*
 * compute context joint probability for context according to one LM
 */
LogP
BayesMix::contextProb(LM &lm, const VocabIndex *context)
{
    unsigned useLength = Vocab::length(context);
    LogP jointProb = 0.0;

    if (clength < useLength) {
	useLength = clength;
    }

    /*
     * If the context is empty there is nothing left to do:
     * we return 0.0 and the mixture will just by the priors
     */
    if (useLength > 0) {
	/*
	 * Turn off debugging for contextProb computation
	 */
	Boolean wasRunning = lm.running(false);

	/*
	 * The usual hack: truncate the context temporarily
	 */
	VocabIndex saved = context[useLength];
	((VocabIndex *)context)[useLength] = Vocab_None;

	/*
	 * If the context starts with <s>, we compute a prefix prob
	 * for it.  Otherwise initialize the jointProb with the
	 * unigram probability of the first word.
	 */
	if (context[useLength - 1] != vocab.ssIndex) {
	    jointProb =
		lm.wordProb(context[useLength - 1], &context[useLength]);
	}

	/*
	 * Accumulate conditional word probs for the remaining context
	 */
	for (unsigned i = useLength - 1; i > 0; i--) {
	    jointProb += lm.wordProb(context[i - 1], &context[i]);
	}

	((VocabIndex *)context)[useLength] = saved;

	lm.running(wasRunning);
    }

    return jointProb;
}

LogP
BayesMix::wordProb(VocabIndex word, const VocabIndex *context)
{
    Prob lm1Prob = LogPtoProb(lm1.wordProb(word, context));
    Prob lm2Prob = LogPtoProb(lm2.wordProb(word, context));

    Prob lm1Weight = prior *
			LogPtoProb(llscale * contextProb(lm1, context));
    Prob lm2Weight = (1.0 - prior) *
			LogPtoProb(llscale * contextProb(lm2, context));

    /*
     * If both LMs don't know this context revert to the prior
     */
    if (lm1Weight == 0.0 && lm2Weight == 0.0) {
	lm1Weight = prior;
	lm2Weight = 1.0 - prior;
    }

    if (running() && debug(DEBUG_MIX_WEIGHTS)) {
	if (clength > 0) {
	    dout() << "[post=" << (lm1Weight/(lm1Weight + lm2Weight))<< "]";
	}
    }

    return ProbToLogP((lm1Weight * lm1Prob + lm2Weight * lm2Prob) /
						(lm1Weight + lm2Weight));
}

void *
BayesMix::contextID(const VocabIndex *context, unsigned &length)
{
    /*
     * Return the context ID of the component model that uses the longer
     * context.
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

void
BayesMix::setState(const char *state)
{
    /*
     * Global state changes are propagated to the component models
     */
    lm1.setState(state);
    lm2.setState(state);
}

