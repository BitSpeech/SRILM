/*
 * LoglinearMix.h --
 *	Log-linear Mixture language model
 *
 * Copyright (c) 2005 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/LoglinearMix.h,v 1.2 2007/12/29 08:39:12 stolcke Exp $
 *
 */

#ifndef _LoglinearMix_h_
#define _LoglinearMix_h_

#include "LM.h"
#include "Array.h"
#include "Trie.h"

class LoglinearMix: public LM
{
public:
    LoglinearMix(Vocab &vocab, LM &lm1, LM &lm2, Prob prior = 0.5);
    LoglinearMix(Vocab &vocab, Array<LM *> &subLMs, Array<Prob> &priors);

    int numLMs;			/* number of component LMs */
    Array<Prob> priors;		/* prior weights */

    /*
     * LM interface
     */
    virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
    virtual void *contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length);
    virtual Boolean isNonWord(VocabIndex word);
    virtual void setState(const char *state);

    virtual Boolean running() { return _running; }
    virtual Boolean running(Boolean newstate);

    void debugme(unsigned level);
    ostream &dout() { return Debug::dout(); };
    ostream &dout(ostream &stream);

protected:
    Array<LM *> subLMs;				/* component models */

    Trie<VocabIndex, LogP> denomProbs;		/* cached normalizers */
};

#endif /* _LoglinearMix_h_ */

