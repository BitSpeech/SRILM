/*
 * TextStats.h --
 *	Text source statistics
 *
 * TextStats objects are used to pass and accumulate various 
 * statistics of text sources (training or test).
 *
 * Copyright (c) 1995-2009 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/TextStats.h,v 1.6 2009/09/25 23:22:51 stolcke Exp $
 *
 */

#ifndef _TextStats_h_
#define _TextStats_h_

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif

#include "Prob.h"
#include "Counts.h"

class TextStats
{
public:
    TextStats() : prob(0.0), zeroProbs(0.0),
	numSentences(0.0), numWords(0.0), numOOVs(0.0) {};

    void reset() { prob = 0.0, zeroProbs = 0.0,
	numSentences = numWords = numOOVs = 0.0; };
    TextStats &increment(const TextStats &stats);

    LogP2 prob;
    FloatCount zeroProbs;
    FloatCount numSentences;
    FloatCount numWords;
    FloatCount numOOVs;
};

ostream &operator<<(ostream &, const TextStats &stats);

#endif /* _TextStats_h_ */

