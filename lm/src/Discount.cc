/*
 * Discount.cc --
 *	Discounting methods
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/spot71/srilm/devel/lm/src/RCS/Discount.cc,v 1.9 1996/09/25 17:59:55 stolcke Exp $";
#endif

#include "Discount.h"

#include "Array.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_ARRAY(double);
#endif

GoodTuring::GoodTuring(unsigned mincount, unsigned maxcount)
   : minCount(mincount), maxCount(maxcount), discountCoeffs(0)
{
   /*
    * a zero count cannot be discounted
    */
   discountCoeffs[0] = 1.0;
}

/*
 * Debug levels used here
 */
#define DEBUG_PRINT_GTCOUNTS	1

/*
 * GT discounting uses the formula
 *
 *   c_discounted = (c + 1) * n_(c+1)/n_c
 *
 * where n_c is the count of count c .
 */
double
GoodTuring::discount(Count count, Count totalCount, Count observedVocab)
{
    if (count <= 0) {
	if (count < 0) {
	    cerr << "trying to discount negative count " << count << endl;
	}
	return 1.0;
    } else if (count < minCount) {
	return 0.0;
    } else if (count > maxCount) {
	return 1.0;
    } else {
	return discountCoeffs[count];
    }
}

/*
 * GT Discounting is effectively disabled if the upper cutoff is 0 or less
 * and the minimum count is no greater than 1 (i.e., no ngrams are excluded).
 */
Boolean
GoodTuring::nodiscount()
{
    return (minCount <= 1 && maxCount <= 0);
}

/*
 * Write all internal parameters to file
 */
void
GoodTuring::write(File &file)
{
    fprintf(file, "mincount %d\n", minCount);
    fprintf(file, "maxcount %d\n", maxCount);

    for (unsigned i = 1; !file.error() && i <= maxCount; i++) {
	fprintf(file , "discount %u %lf\n", i, discountCoeffs[i]);
    }
}

/*
 * Read parameters back from file
 */
Boolean
GoodTuring::read(File &file)
{
    char *line;

    while (line = file.getline()) {
	unsigned count;
	double coeff;
	
	if (sscanf(line, "mincount %u", &minCount) == 1) {
	    continue;
	} else if (sscanf(line, "maxcount %u", &maxCount) == 1) {
	    /*
	     * Zero all old discount coeffs
	     */
	    for (Count n = 0; n <= maxCount; n++) {
		discountCoeffs[n] = 0.0;
	    }
	    continue;
	} else if (sscanf(line, "discount %u %lf", &count, &coeff) == 2) {
	    discountCoeffs[(Count)count] = coeff;
	} else {
	    file.position() << "unrecognized parameter\n";
	    return false;
	}
    }

    for (Count n = minCount; n <= maxCount; n++) {
	if (discountCoeffs[n] == 0.0) {
	    file.position() << "warning: discount coefficient " << n
			    << " = 0.0\n";
	}
    }
    return true;
}

/*
 * Estimation of discount coefficients from ngram count-of-counts
 *
 * The Good-Turing formula for this is
 *
	d(c) = (c+1)/c * n_(c+1)/n_c
 */
Boolean
GoodTuring::estimate(NgramStats &counts, unsigned order)
{
    Array<Count> countOfCounts;

    /*
     * First tabulate count-of-counts for the given order of ngrams 
     * Note we need GT count for up to maxCount + 1 inclusive, to apply
     * the GT formula for counts up to maxCount.
     */
    VocabIndex *wids = new VocabIndex[order + 1];
    assert(wids);

    NgramsIter iter(counts, wids, order);
    Count *count, i;

    for (i = 0; i <= maxCount + 1; i++) {
	countOfCounts[i]  = 0;
    }

    while (count = iter.next()) {
	if (counts.vocab.isNonEvent(wids[order - 1])) {
	    continue;
	} else if (*count <= maxCount + 1) {
	    countOfCounts[*count] ++;
	}
    }

    if (debug(DEBUG_PRINT_GTCOUNTS)) {
	for (i = 0; i <= maxCount + 1; i++) {
	    dout() << "GT-count [" << i << "] = " << countOfCounts[i] << endl;
	}
    }

    if (countOfCounts[1] == 0) {
	cerr << "warning: no singleton counts\n";
	maxCount = 0;
    }

    while (maxCount > 0 && countOfCounts[maxCount + 1] == 0) {
	cerr << "warning: count of count " << maxCount + 1 << " is zero "
	     << "-- lowering maxcount\n";
	maxCount --;
    }

    if (maxCount <= 0) {
	cerr << "GT discounting disabled\n";
    } else {
	double commonTerm = (maxCount + 1) *
				(double)countOfCounts[maxCount + 1] /
				    (double)countOfCounts[1];

	for (i = 1; i <= maxCount; i++) {
	    double coeff;

	    if (countOfCounts[i] == 0) {
		cerr << "warning: count of count " << i << " is zero\n";
		coeff = 1.0;
	    } else {
		double coeff0 = (i + 1) * (double)countOfCounts[i+1] /
					    (i * (double)countOfCounts[i]);
		if (coeff0 <= commonTerm || coeff0 > 1.0) {
		    cerr << "warning: discount coeff " << i
			 << " is out of range: " << coeff0 << "\n";
		    coeff = 1.0;
		} else {
		    coeff = (coeff0 - commonTerm) / (1.0 - commonTerm);

		}
	    }
	    discountCoeffs[i] = coeff;
	}
    }

    delete [] wids;
    return true;
}

/*
 * Eric Ristad's Natural Law of Succession --
 *	The discount factor d is identical for all counts,
 *
 *	d = ( n(n+1) + q(1-q) ) / 
 *	    ( n^2 + n + 2q ) 
 *
 *  where n is the total number events tokens, q is the number of observed
 *  event types.  If q equals the vocabulary size no discounting is 
 *  performed.
 */

double
NaturalDiscount::discount(Count count, Count totalCount, Count observedVocab)
{
    double n = totalCount;
    double q = observedVocab;

    if (count < _mincount) {
	return 0.0;
    } else if (q == vocabSize) {
	return 1.0;
    } else {
	return (n * (n+1) + q * (1 - q)) / (n * (n + 1) + 2 * q);
    }
}

Boolean
NaturalDiscount::estimate(NgramStats &counts, unsigned order)
{
    /*
     * Determine the true vocab size, i.e., the number of true event
     * tokens, by enumeration.
     */
    VocabIter viter(counts.vocab);
    VocabIndex wid;

    unsigned total = 0;
    while (viter.next(wid)) {
	if (!counts.vocab.isNonEvent(wid)) {
	    total ++;
	}
    }

    vocabSize = total;
    return true;
}

