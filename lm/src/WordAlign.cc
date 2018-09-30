/*
 * WordAlign.cc --
 *	Word alignment and error computation
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,1997 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/WordAlign.cc,v 1.7 1998/02/21 01:16:38 stolcke Exp $";
#endif

#include "WordAlign.h"

/*
 * wordError --
 *	Compute word error counts between hyp and a ref word string
 *
 * Result:
 *	Total word error count
 *
 * Side effect:
 *	sub, ins, del are set to the number of substitution, insertion and
 *	deletion errors, respectively.
 */
unsigned
wordError(const VocabIndex *ref, const VocabIndex *hyp,
			unsigned &sub, unsigned &ins, unsigned &del)
{
    unsigned hypLength = Vocab::length(hyp);
    unsigned refLength = Vocab::length(ref);

    typedef enum {
	CORR, SUB, INS, DEL
    } ErrorType;

    typedef struct {
	unsigned cost;			// minimal cost of partial alignment
	ErrorType error;		// best predecessor
    } ChartEntry;

    /* 
     * Allocate chart statically, enlarging on demand
     */
    static unsigned maxHypLength = 0;
    static unsigned maxRefLength = 0;
    static ChartEntry **chart = 0;

    if (chart == 0 || hypLength > maxHypLength || refLength > maxRefLength) {
	/*
	 * Free old chart
	 */
	if (chart != 0) {
	    for (unsigned i = 0; i <= maxRefLength; i ++) {
		delete [] chart[i];
	    }
	    delete [] chart;
	}

	/*
	 * Allocate new chart
	 */
	maxHypLength = hypLength;
	maxRefLength = refLength;
    
	chart = new ChartEntry*[maxRefLength + 1];
	assert(chart != 0);

	unsigned i, j;

	for (i = 0; i <= maxRefLength; i ++) {
	    chart[i] = new ChartEntry[maxHypLength + 1];
	    assert(chart[i] != 0);
	}

	/*
	 * Initialize the 0'th row and column, which never change
	 */
	chart[0][0].cost = 0;
	chart[0][0].error = CORR;

	/*
	 * Initialize the top-most row in the alignment chart
	 * (all words inserted).
	 */
	for (j = 1; j <= maxHypLength; j ++) {
	    chart[0][j].cost = chart[0][j-1].cost + INS_COST;
	    chart[0][j].error = INS;
	}

	for (i = 1; i <= maxRefLength; i ++) {
	    chart[i][0].cost = chart[i-1][0].cost + DEL_COST;
	    chart[i][0].error = DEL;
	}
    }

    /*
     * Fill in the rest of the chart, row by row.
     */
    for (unsigned i = 1; i <= refLength; i ++) {

	for (unsigned j = 1; j <= hypLength; j ++) {
	    unsigned minCost;
	    ErrorType minError;

	    if (hyp[j-1] == ref[i-1]) {
		minCost = chart[i-1][j-1].cost;
		minError = CORR;
	    } else {
		minCost = chart[i-1][j-1].cost + SUB_COST;
		minError = SUB;
	    }

	    unsigned delCost = chart[i-1][j].cost + DEL_COST;
	    if (delCost < minCost) {
		minCost = delCost;
		minError = DEL;
	    }

	    unsigned insCost = chart[i][j-1].cost + INS_COST;
	    if (insCost < minCost) {
		minCost = insCost;
		minError = INS;
	    }

	    chart[i][j].cost = minCost;
	    chart[i][j].error = minError;
	}
    }

    /*
     * Backtrace
     */
    unsigned totalErrors;

    {
	unsigned i = refLength;
	unsigned j = hypLength;

	sub = del = ins = 0;

	while (i > 0 || j > 0) {

	    switch (chart[i][j].error) {
	    case CORR:
		i --; j --;
		break;
	    case SUB:
		i --; j --;
		sub ++;
		break;
	    case DEL:
		i --;
		del ++;
		break;
	    case INS:
		j --;
		ins ++;
		break;
	    }
	}

	totalErrors = sub + del + ins;
    }

    return totalErrors;
}

