/*
 * LoglinearMix.h --
 *	Log-linear Mixture language model
 *
 * Copyright (c) 2005 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/LoglinearMix.h,v 1.1 2005/07/18 03:39:36 stolcke Exp $
 *
 */

#ifndef _LoglinearMix_h_
#define _LoglinearMix_h_

#include "LM.h"
#include "Trie.h"

class LoglinearMix: public LM
{
public:
    LoglinearMix(Vocab &vocab, LM &lm1, LM &lm2, Prob prior = 0.5);

    Prob prior;			/* prior weight for lm1 */

    /*
     * LM interface
     */
    virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
    virtual void *contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length);
    virtual Boolean isNonWord(VocabIndex word);
    virtual void setState(const char *state);

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

    Trie<VocabIndex, LogP> denomProbs;		/* cached normalizers */
};

#endif /* _LoglinearMix_h_ */

