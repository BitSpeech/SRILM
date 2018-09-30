/*
 * TextStats.cc --
 *	Text statistics
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2009 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/TextStats.cc,v 1.4 2009/09/25 23:15:25 stolcke Exp $";
#endif

#include "TextStats.h"

/*
 * Increments from other source
 */
TextStats &
TextStats::increment(const TextStats &stats)
{
    numSentences += stats.numSentences;
    numWords += stats.numWords;
    numOOVs += stats.numOOVs;
    prob += stats.prob;
    zeroProbs += stats.zeroProbs;

    return *this;
}

/*
 * Format stats for stream output
 */
ostream &
operator<< (ostream &stream, const TextStats &stats)
{

    stream << stats.numSentences << " sentences, " 
           << stats.numWords << " words, "
	   << stats.numOOVs << " OOVs" << endl;
    if (stats.numWords + stats.numSentences > 0) {
	stream << stats.zeroProbs << " zeroprobs, "
	       << "logprob= " << stats.prob;

	double denom = stats.numWords - stats.numOOVs - stats.zeroProbs
							+ stats.numSentences;

	if (denom > 0) {
	    stream << " ppl= " << LogPtoPPL(stats.prob / denom);
	} else {
	    stream << " ppl= undefined";
	}

	denom -= stats.numSentences;

	if (denom > 0) {
	    stream << " ppl1= " << LogPtoPPL(stats.prob / denom);
	} else {
	    stream << " ppl1= undefined";
	}

	stream << endl;
    }
    return stream;
}

