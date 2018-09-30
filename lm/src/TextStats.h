/*
 * TextStats.h --
 *	Text source statistics
 *
 * TextStats objects are used to pass and accumulate various 
 * statistics of text sources (training or test).
 *
 * Copyright (c) 1995,2005 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/TextStats.h,v 1.3 2006/01/05 20:21:27 stolcke Exp $
 *
 */

#ifndef _TextStats_h_
#define _TextStats_h_

#include <iostream>
using namespace std;

#include "Prob.h"

class TextStats
{
public:
    TextStats() : prob(0.0), zeroProbs(0),
	numSentences(0), numWords(0), numOOVs(0) {};

    void reset() { prob = 0.0, zeroProbs = 0,
	numSentences = numWords = numOOVs = 0; };
    TextStats &increment(const TextStats &stats);

    LogP prob;
    unsigned zeroProbs;
    unsigned numSentences;
    unsigned numWords;
    unsigned numOOVs;
};

ostream &operator<<(ostream &, const TextStats &stats);

#endif /* _TextStats_h_ */

