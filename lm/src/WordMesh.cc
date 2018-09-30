/*
 * WordMesh.cc --
 *	Word Meshes
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/WordMesh.cc,v 1.7 1998/02/21 01:16:38 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "WordMesh.h"
#include "WordAlign.h"

#include "Array.cc"
#include "LHash.cc"

/*
 * Special token used to represent an empty position in an alignment column
 */
const VocabString deleteWord = "*DELETE*";

WordMesh::WordMesh(Vocab &vocab)
    : MultiAlign(vocab), numAligns(0), totalPosterior(0.0)
{
    deleteIndex = vocab.addWord(deleteWord);
}

WordMesh::~WordMesh()
{
    for (unsigned i = 0; i < numAligns; i ++) {
	delete aligns[i];
    }
}
   
Boolean
WordMesh::isEmpty()
{
    return numAligns == 0;
}

Boolean
WordMesh::write(File &file)
{
    fprintf(file, "numaligns %u\n", numAligns);

    for (unsigned i = 0; i < numAligns; i ++) {
	fprintf(file, "align %u", i);

	LHashIter<VocabIndex,Prob> alignIter(*aligns[sortedAligns[i]]);

	Prob *prob;
	VocabIndex word;

	while (prob = alignIter.next(word)) {
	    fprintf(file, " %s %lg", vocab.getWord(word), *prob);
	}
	fprintf(file, "\n");
    }

    return true;
}

Boolean
WordMesh::read(File &file)
{
    for (unsigned i = 0; i < numAligns; i ++) {
	delete aligns[i];
    }
   
    char *line;

    while (line = file.getline()) {
	char arg1[100];
	double arg2;
	unsigned parsed;
	unsigned index;

	if (sscanf(line, "numaligns %u", &numAligns) == 1) {
	    for (unsigned i = 0; i < numAligns; i ++) {
		aligns[i] = new LHash<VocabIndex,Prob>;
		assert(aligns[i] != 0);
	    }
	} else if (sscanf(line, "align %u%n", &index, &parsed) == 1)
	{
	    assert(index < numAligns);
	    sortedAligns[index] = index;

	    char *cp = line + parsed;
	    while (sscanf(cp, "%100s %lg%n", arg1, &arg2, &parsed) == 2) {
		VocabIndex word = vocab.addWord(arg1);

		*aligns[index]->insert(word) = arg2;
		
		cp += parsed;
	    }
	} else {
	    file.position() << "unknown keyword\n";
	    return false;
	}
    }
    return true;
}

/*
 * Align new words to existing alignment columns, expanding them as required
 * (derived from WordAlign())
 */
void
WordMesh::alignWords(const VocabIndex *words, Prob score, Prob *wordScores)
{
    unsigned hypLength = Vocab::length(words);
    unsigned refLength = numAligns;

    typedef enum {
	CORR, SUB, INS, DEL
    } ErrorType;

    typedef struct {
	double cost;			// minimal cost of partial alignment
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

	for (unsigned i = 0; i <= maxRefLength; i ++) {
	    chart[i] = new ChartEntry[maxHypLength + 1];
	    assert(chart[i] != 0);
	}

    }

    /*
     * Initialize the 0'th row
     */
    chart[0][0].cost = 0.0;
    chart[0][0].error = CORR;

    for (unsigned j = 1; j <= hypLength; j ++) {
	/* insert error prob = totalPosterior */
	chart[0][j].cost = chart[0][j-1].cost + totalPosterior;
	chart[0][j].error = INS;
    }

    /*
     * Fill in the rest of the chart, row by row.
     */
    for (unsigned i = 1; i <= refLength; i ++) {

	/* deletion error prob = totalPosterior - deletion prob */
	Prob *deleteProb = aligns[sortedAligns[i-1]]->find(deleteIndex);
	double deletePenalty = 
			    totalPosterior - (deleteProb ? *deleteProb : 0.0);

	chart[i][0].cost = chart[i-1][0].cost + deletePenalty;
	chart[i][0].error = DEL;

	for (unsigned j = 1; j <= hypLength; j ++) {
	    double minCost;
	    ErrorType minError;

	    Prob *wordProb = aligns[sortedAligns[i-1]]->find(words[j-1]);
	    if (wordProb) {
		minCost = chart[i-1][j-1].cost + totalPosterior - *wordProb;
		minError = CORR;
	    } else {
		minCost = chart[i-1][j-1].cost + totalPosterior;
		minError = SUB;
	    }

	    double delCost = chart[i-1][j].cost + deletePenalty;
	    if (delCost < minCost) {
		minCost = delCost;
		minError = DEL;
	    }

	    double insCost = chart[i][j-1].cost + totalPosterior;
	    if (insCost < minCost) {
		minCost = insCost;
		minError = INS;
	    }

	    chart[i][j].cost = minCost;
	    chart[i][j].error = minError;
	}
    }

    /*
     * Backtrace and add new words to alignment columns.
     * Store word posteriors if requested.
     */
    {
	unsigned i = refLength;
	unsigned j = hypLength;

	while (i > 0 || j > 0) {

	    switch (chart[i][j].error) {
	    case CORR:
	    case SUB:
		*aligns[sortedAligns[i-1]]->insert(words[j-1]) += score;

		if (wordScores) {
		    wordScores[j-1] =
				*aligns[sortedAligns[i-1]]->insert(words[j-1]);
		}

		i --; j --;
		break;
	    case DEL:
		*aligns[sortedAligns[i-1]]->insert(deleteIndex) += score;
		i --;
		break;
	    case INS:
		/*
		 * Make room for new alignment column in sortedAligns
		 * and position new column
		 */
		for (unsigned k = numAligns; k > i; k --) {
		    sortedAligns[k] = sortedAligns[k-1];
		}
		sortedAligns[i] = numAligns;

		aligns[numAligns] = new LHash<VocabIndex,Prob>;
		assert(aligns[numAligns] != 0);

		if (totalPosterior != 0.0) {
		    *aligns[numAligns]->insert(deleteIndex) = totalPosterior;
		}
		*aligns[numAligns]->insert(words[j-1]) = score;

		if (wordScores) {
		    wordScores[j-1] = score;
		}

		numAligns ++;
		j --;
		break;
	    }
	}
    }

    totalPosterior += score;
}

/*
 * Compute minimal word error with respect to existing alignment columns
 * (derived from WordAlign())
 */
unsigned
WordMesh::wordError(const VocabIndex *words,
				unsigned &sub, unsigned &ins, unsigned &del)
{
    unsigned hypLength = Vocab::length(words);
    unsigned refLength = numAligns;

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

    if (hypLength > maxHypLength || refLength > maxRefLength) {
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

	for (unsigned i = 0; i <= maxRefLength; i ++) {
	    chart[i] = new ChartEntry[maxHypLength + 1];
	    assert(chart[i] != 0);
	}

	/*
	 * Initialize the 0'th row, which never changes
	 */
	chart[0][0].cost = 0;
	chart[0][0].error = CORR;

	for (unsigned j = 1; j <= maxHypLength; j ++) {
	    chart[0][j].cost = chart[0][j-1].cost + INS_COST;
	    chart[0][j].error = INS;
	}
    }

    /*
     * Fill in the rest of the chart, row by row.
     */
    for (unsigned i = 1; i <= refLength; i ++) {

	/*
	 * deletion error only if alignment column doesn't have *DELETE*
	 */
	Prob *delProb = aligns[sortedAligns[i-1]]->find(deleteIndex);
	unsigned THIS_DEL_COST = delProb && *delProb > 0.0 ? 0 : DEL_COST;
	
	chart[i][0].cost = chart[i-1][0].cost + THIS_DEL_COST;
	chart[i][0].error = DEL;

	for (unsigned j = 1; j <= hypLength; j ++) {
	    unsigned minCost;
	    ErrorType minError;
	
	    if (aligns[sortedAligns[i-1]]->find(words[j-1])) {
		minCost = chart[i-1][j-1].cost;
		minError = CORR;
	    } else {
		minCost = chart[i-1][j-1].cost + SUB_COST;
		minError = SUB;
	    }

	    unsigned delCost = chart[i-1][j].cost + THIS_DEL_COST;
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
     * Backtrace and add new words to alignment columns
     */
    {
	unsigned i = refLength;
	unsigned j = hypLength;

	sub = ins = del = 0;

	while (i > 0 || j > 0) {

	    switch (chart[i][j].error) {
	    case CORR:
		i --; j--;
		break;
	    case SUB:
		sub ++;
		i --; j --;
		break;
	    case DEL:
		/*
		 * deletion error only if alignment column doesn't 
		 * have *DELETE*
		 */
		{
		    Prob *delProb =
				aligns[sortedAligns[i-1]]->find(deleteIndex);
		    if (!delProb || *delProb == 0.0) {
			del ++;
		    }
		}
		i --;
		break;
	    case INS:
		ins ++;
		j --;
		break;
	    }
	}
    }

    return sub + ins + del;
}

double
WordMesh::minimizeWordError(VocabIndex *words, unsigned length,
				double &sub, double &ins, double &del,
				unsigned flags)
{
    unsigned numWords = 0;
    sub = ins = del = 0.0;

    for (unsigned i = 0; i < numAligns; i ++) {

	LHashIter<VocabIndex,Prob> alignIter(*aligns[sortedAligns[i]]);

	Prob deleteProb = 0.0;
	Prob bestProb;
	VocabIndex bestWord = Vocab_None;

	Prob *prob;
	VocabIndex word;
	while (prob = alignIter.next(word)) {
	    if (bestWord == Vocab_None || *prob > bestProb) {
		bestWord = word;
		bestProb = *prob;
	    }
	    if (word == deleteIndex) {
		deleteProb = *prob;
	    }
	}

	if (bestWord != deleteIndex) {
	    if (numWords < length) {
		words[numWords ++] = bestWord;
	    }
	    ins += deleteProb;
	    sub += totalPosterior - deleteProb - bestProb;
	} else {
	    del += totalPosterior - deleteProb;
	}
    }
    if (numWords < length) {
	words[numWords] = Vocab_None;
    }

    return sub + ins + del;
}

