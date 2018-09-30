/*
 * TextStats.cc --
 *	Text statistics
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/TextStats.cc,v 1.1 1999/10/07 08:22:10 stolcke Exp $";
#endif

#include "TextStats.h"

/*
 * Increments from other source
 */
TextStats &
TextStats::increment(TextStats &stats)
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
operator<< (ostream &stream, TextStats &stats)
{

    stream << stats.numSentences << " sentences, " 
           << stats.numWords << " words, "
	   << stats.numOOVs << " OOVs" << endl;
    if (stats.numWords + stats.numSentences > 0) {
	double ppl = LogPtoPPL(stats.prob / (stats.numWords
					     - stats.numOOVs
					     - stats.zeroProbs
					     + stats.numSentences));
	double ppl1 = LogPtoPPL(stats.prob / (stats.numWords
					     - stats.numOOVs
					     - stats.zeroProbs));
	stream << stats.zeroProbs << " zeroprobs, "
	       << "logprob= " << stats.prob << " "
	       << "ppl= " << ppl << " "
	       << "ppl1= " << ppl1 << endl;
    }
    return stream;
}

