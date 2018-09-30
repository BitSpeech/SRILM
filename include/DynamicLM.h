/*
 * DynamicLM.h
 *	Dynamically loaded language model
 *
 * This model interpret global state change request to load new LMs
 * on demand.  It can be used to implement simple adaptation schemes.
 * (Currently only ngram models are supported.)
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /export/d/stolcke/project/srilm/src/RCS/DynamicLM.h,v 1.1 1995/08/24 01:28:09 stolcke Exp $
 *
 */

#ifndef _DynamicLM_h_
#define _DynamicLM_h_

#include "LM.h"

class DynamicLM: public LM
{
public:
    DynamicLM(Vocab &vocab);

    virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
    virtual void setState(const char *state);

    /*
     * Propagate changes to running state to component models
     */
    virtual Boolean running() { return _running; }
    virtual Boolean running(Boolean newstate)
      { Boolean old = _running; _running = newstate; 
	if (myLM) myLM->running(newstate); return old; };

    /*
     * Propagate changes to Debug state to component models
     */
    void debugme(unsigned level)
	{ if (myLM) myLM->debugme(level); Debug::debugme(level); };
    ostream &dout() { return Debug::dout(); };
    ostream &dout(ostream &stream)  /* propagate dout changes to sub-lms */
	{ if (myLM) myLM->dout(stream); return Debug::dout(stream); };

protected:
    LM *myLM;			/* the current model */
    const char *currentState;	/* last state info */
};


#endif /* _DynamicLM_h_ */
