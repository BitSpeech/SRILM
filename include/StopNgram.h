/*
 * StopNgram.h --
 *	N-gram LM with stop words removed from context
 *
 * Copyright (c) 1996, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/spot71/srilm/devel/lm/src/RCS/StopNgram.h,v 1.1 1996/12/10 09:28:49 stolcke Exp $
 *
 */

#ifndef _StopNgram_h_
#define _StopNgram_h_

#include "Ngram.h"
#include "SubVocab.h"

class StopNgram: public Ngram
{
public:
    StopNgram(Vocab &vocab, SubVocab &stopWords, unsigned int order);

    SubVocab &stopWords;			/* stop word set */

protected:
    virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
};

#endif /* _StopNgram_h_ */
