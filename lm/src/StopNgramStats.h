/*
 * StopNgramStats.h --
 *	N-gram statistics with contexts excluding stop words
 *
 * Copyright (c) 1996,2002 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/StopNgramStats.h,v 1.2 2002/08/09 08:46:54 stolcke Exp $
 *
 */

#ifndef _StopNgramStats_h_
#define _StopNgramStats_h_

#include "NgramStats.h"
#include "SubVocab.h"

class StopNgramStats: public NgramStats
{
public:
    StopNgramStats(Vocab &vocab, SubVocab &stopWords, unsigned maxOrder);

    virtual unsigned countSentence(const VocabIndex *words, NgramCount factor);

    const SubVocab &stopWords;		/* stop word set */

protected:
    void incrementCounts(const VocabIndex *words, NgramCount factor);
};

#endif /* _StopNgramStats_h_ */

