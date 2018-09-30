/*
 * TextStats.cc --
 *	Text statistics
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2011 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/lm/src/TextStats.cc,v 1.7 2012/07/22 22:21:52 stolcke Exp $";
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

    /* 
     * Ranking and loss metrics
     */
    r1  += stats.r1;
    r5  += stats.r5;
    r10 += stats.r10;

    r1se  += stats.r1se;
    r5se  += stats.r5se;
    r10se += stats.r10se;

    rTotal += stats.rTotal;

    posQuadLoss += stats.posQuadLoss;
    posAbsLoss += stats.posAbsLoss;
      
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

	/*
	 * Ranking and loss metrics
	 */
	if (stats.rTotal > 0) {
	    FloatCount denom1 = stats.rTotal - stats.numSentences;
	    FloatCount denom2 = stats.rTotal;

	    if (denom2 > 0) {
		stream << endl << denom1 << " words,";
		stream << " rank1= " << (denom1 > 0 ? stats.r1 / denom1 : 0.0);
		stream << " rank5= " << (denom1 > 0 ? stats.r5 / denom1 : 0.0);
		stream << " rank10= " << (denom1 > 0 ? stats.r10 / denom1 : 0.0);

		stream << endl << denom2 << " words+sents,";
		stream << " rank1wSent= " << (stats.r1 + stats.r1se) / denom2;
		stream << " rank5wSent= " << (stats.r5 + stats.r5se) / denom2;
		stream << " rank10wSent= " << (stats.r10 + stats.r10se) / denom2;
		stream << " qloss= " << sqrt(stats.posQuadLoss / denom2);
		stream << " absloss= " << stats.posAbsLoss / denom2;
	    }
        }

	stream << endl;
    }
    return stream;
}

