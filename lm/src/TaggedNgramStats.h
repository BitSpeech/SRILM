/*
 * TaggedNgramStats.h --
 *	N-gram statistics on word/tag pairs
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /export/d/stolcke/project/srilm/src/RCS/TaggedNgramStats.h,v 1.1 1995/08/23 03:19:03 stolcke Exp $
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

    virtual unsigned int countSentence(const VocabIndex *word);

    TaggedVocab &vocab;			/* vocabulary */

protected:
    void incrementTaggedCounts(const VocabIndex *words);
};

#endif /* _TaggedNgramStats_h_ */

