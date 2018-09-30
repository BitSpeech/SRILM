/*
 * TaggedNgramStats.h --
 *	N-gram statistics on word/tag pairs
 *
 * Copyright (c) 1995,2002 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/TaggedNgramStats.h,v 1.2 2002/08/09 08:46:54 stolcke Exp $
 *
 */

#ifndef _TaggedNgramStats_h_
#define _TaggedNgramStats_h_

#include "NgramStats.h"
#include "TaggedVocab.h"

class TaggedNgramStats: public NgramStats
{
public:
    TaggedNgramStats(TaggedVocab &vocab, unsigned int maxOrder);

    virtual unsigned countSentence(const VocabIndex *words, NgramCount factor);

    TaggedVocab &vocab;			/* vocabulary */

protected:
    void incrementTaggedCounts(const VocabIndex *words, NgramCount factor);
};

#endif /* _TaggedNgramStats_h_ */

