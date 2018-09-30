/*
 * StopNgramStats.h --
 *	N-gram statistics with contexts excluding stop words
 *
 * Copyright (c) 1996, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/StopNgramStats.h,v 1.1 1996/12/10 09:28:59 stolcke Exp $
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

    virtual unsigned int countSentence(const VocabIndex *word);

    const SubVocab &stopWords;		/* stop word set */

protected:
    void incrementCounts(const VocabIndex *words);
};

#endif /* _StopNgramStats_h_ */

