/*
 * BayesMix.h --
 *	Bayesian Mixture language model
 *
 * A Bayesian mixture LM interpolates two LMs M_1 and M_2 according to a
 * local estimate of the posterior model probabilities.
 *
 *	p(w | context)  = p(M_1 | context) p(w | context, M_1) +
 *	                  p(M_2 | context) p(w | context, M_2)
 *
 * where p(M_i | context) is proportional to p(M_i) p(context | M_i)
 * and p(context | M_i) is either the full sentence prefix probability,
 * or a marginal probability of a truncated context.  For example, for
 * mixtures of bigram models, the p(context | M_i) would simply be the unigram 
 * probability of the last word according to M_i.
 *
 * Copyright (c) 1995-2002 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/BayesMix.h,v 1.9 2007/12/05 00:31:49 stolcke Exp $
 *
 */

#ifndef _BayesMix_h_
#define _BayesMix_h_

#include "LM.h"

class BayesMix: public LM
{
public:
    BayesMix(Vocab &vocab, LM &lm1, LM &lm2,
		    unsigned clength = 0, Prob prior = 0.5,
		    double llscale = 1.0);

    unsigned clength;		/* context length used for posteriors */
    Prob prior;			/* prior weight for lm1 */
    double llscale;		/* log likelihood scaling factor */

    /*
     * LM interface
     */
    virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
    virtual void *contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length);
    virtual Boolean isNonWord(VocabIndex word);
    virtual void setState(const char *state);

    virtual Boolean addUnkWords()
       { return lm1.addUnkWords() || lm2.addUnkWords(); };

    /*
     * Propagate changes to running state to component models
     */
    virtual Boolean running() { return _running; }
    virtual Boolean running(Boolean newstate)
      { Boolean old = _running; _running = newstate; 
	lm1.running(newstate); lm2.running(newstate); return old; };

    /*
     * Propagate changes to Debug state to component models
     */
    void debugme(unsigned level)
	{ lm1.debugme(level); lm2.debugme(level); Debug::debugme(level); };
    ostream &dout() { return Debug::dout(); };
    ostream &dout(ostream &stream)  /* propagate dout changes to sub-lms */
	{ lm1.dout(stream); lm2.dout(stream); return Debug::dout(stream); };

protected:
    LM &lm1, &lm2;				/* component models */
};


#endif /* _BayesMix_h_ */
