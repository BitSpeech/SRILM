/*
 * LMStats.h --
 *	Generic LM statistics interface
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/LMStats.h,v 1.7 1997/07/18 02:27:37 stolcke Exp $
 *
 */

#ifndef _LMStats_h_
#define _LMStats_h_

#include <stdio.h>

#include "Boolean.h"
#include "Vocab.h"
#include "Debug.h"
#include "LM.h"

class LMStats: public Debug
{
public:
    LMStats(Vocab &vocab);
    virtual ~LMStats();

    virtual unsigned int countSentence(const VocabString *words) = 0;
    virtual unsigned int countSentence(const VocabIndex *words) = 0;

    virtual unsigned int countString(char *sentence);
    virtual unsigned int countFile(File &file);

    virtual Boolean read(File &file) = 0;
    virtual void write(File &file) = 0;

    virtual void memStats(MemStats &stats) = 0;
					/* compute memory stats */
    Vocab &vocab;			/* vocabulary */
    Boolean openVocab;			/* whether to add words as needed */

    TextStats stats;			/* training data stats */
};

#endif /* _LMStats_h_ */

