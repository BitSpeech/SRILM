/*
 * WordMesh.cc --
 *	Word Meshes (aka Confusion Networks aka Sausages)
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2001 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/WordMesh.cc,v 1.21 2001/08/07 06:59:36 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "WordMesh.h"
#include "WordAlign.h"

#include "Array.cc"
#include "LHash.cc"
#include "SArray.cc"

/*
 * Special token used to represent an empty position in an alignment column
 */
const VocabString deleteWord = "*DELETE*";

WordMesh::WordMesh(Vocab &vocab, VocabDistance *distance)
    : MultiAlign(vocab), numAligns(0), totalPosterior(0.0), distance(distance)
{
    deleteIndex = vocab.addWord(deleteWord);
}

WordMesh::~WordMesh()
{
    for (unsigned i = 0; i < numAligns; i ++) {
	delete aligns[i];

	LHashIter<VocabIndex,NBestWordInfo> infoIter(*wordInfo[i]);
	NBestWordInfo *winfo;
	VocabIndex word;
	while (winfo = infoIter.next(word)) {
	    winfo->~NBestWordInfo();
	}
	delete wordInfo[i];

	LHashIter<VocabIndex,Array<HypID> > mapIter(*hypMap[i]);
	Array<HypID> *hyps;
	while (hyps = mapIter.next(word)) {
	    hyps->~Array();
	}
	delete hypMap[i];
    }
}
   
Boolean
WordMesh::isEmpty()
{
    return numAligns == 0;
}

/*
 * alignment set to sort by posterior (parameter to comparePosteriors)
 */
static LHash<VocabIndex, Prob> *compareAlign;

static int
comparePosteriors(VocabIndex w1, VocabIndex w2)
{
    Prob diff = *compareAlign->find(w1) - *compareAlign->find(w2);

    if (diff < 0.0) {
	return 1;
    } else if (diff > 0.0) {
	return -1;
    } else {
	return 0;
    }
}

Boolean
WordMesh::write(File &file)
{
    fprintf(file, "numaligns %u\n", numAligns);
    fprintf(file, "posterior %lg\n", totalPosterior);

    for (unsigned i = 0; i < numAligns; i ++) {
	fprintf(file, "align %u", i);

	compareAlign = aligns[sortedAligns[i]];
	LHashIter<VocabIndex,Prob> alignIter(*compareAlign, comparePosteriors);

	Prob *prob;
	VocabIndex word;
	VocabIndex refWord = Vocab_None;

	while (prob = alignIter.next(word)) {
	    fprintf(file, " %s %lg", vocab.getWord(word), *prob);

	    /*
	     * See if this word is the reference one
	     */
	    Array<HypID> *hypList = hypMap[sortedAligns[i]]->find(word);
	    if (hypList) {
		for (unsigned j = 0; j < hypList->size(); j++) {
		    if ((*hypList)[j] == refID) {
			refWord = word;
			break;
		    }
		}
	    }
	}
	fprintf(file, "\n");

	/* 
	 * Print reference word (if known)
	 */
	if (refWord != Vocab_None) {
	    fprintf(file, "reference %u %s\n", i, vocab.getWord(refWord));
	}

	/*
	 * Dump hyp IDs if known
	 */
	LHashIter<VocabIndex,Array<HypID> >
			mapIter(*hypMap[sortedAligns[i]], comparePosteriors);
	Array<HypID> *hypList;

	while (hypList = mapIter.next(word)) {
	    /*
	     * Only output hyp IDs if they are different from the refID
	     * (to avoid redundancy with "reference" line)
	     */
	    if (hypList->size() > (word == refWord)) {
		fprintf(file, "hyps %u %s", i, vocab.getWord(word));

		for (unsigned j = 0; j < hypList->size(); j++) {
		    if ((*hypList)[j] != refID) {
			fprintf(file, " %d", (int)(*hypList)[j]);
		    }
		}
		fprintf(file, "\n");
	    }
	}

	/*
	 * Dump word backtrace info if known
	 */
	LHashIter<VocabIndex,NBestWordInfo>
			infoIter(*wordInfo[sortedAligns[i]], comparePosteriors);
	NBestWordInfo *winfo;

	while (winfo = infoIter.next(word)) {
	    fprintf(file, "info %u %s ", i, vocab.getWord(word));
	    winfo->write(file);
	    fprintf(file, "\n");
	}
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

		wordInfo[i] = new LHash<VocabIndex,NBestWordInfo>;
		assert(wordInfo[i] != 0);

		hypMap[i] = new LHash<VocabIndex,Array<HypID> >;
		assert(hypMap[i] != 0);
	    }
	} else if (sscanf(line, "posterior %lg", &arg2) == 1) {
	    totalPosterior = arg2;
	} else if (sscanf(line, "align %u%n", &index, &parsed) == 1) {
	    assert(index < numAligns);
	    sortedAligns[index] = index;

	    char *cp = line + parsed;
	    while (sscanf(cp, "%100s %lg%n", arg1, &arg2, &parsed) == 2) {
		VocabIndex word = vocab.addWord(arg1);

		*aligns[index]->insert(word) = arg2;
		
		cp += parsed;
	    }
	} else if (sscanf(line, "reference %u %100s", &index, arg1) == 2) {
	    assert(index < numAligns);
	    sortedAligns[index] = index;

	    VocabIndex refWord = vocab.addWord(arg1);

	    /*
	     * Records word as part of the reference string
	     */
	    Array<HypID> *hypList = hypMap[index]->insert(refWord);
	    (*hypList)[hypList->size()] = refID;
	} else if (sscanf(line, "hyps %u %100s%n", &index, arg1, &parsed) == 2){
	    assert(index < numAligns);
	    sortedAligns[index] = index;

	    VocabIndex word = vocab.addWord(arg1);
	    Array<HypID> *hypList = hypMap[index]->insert(word);

	    /*
	     * Parse and record hyp IDs
	     */
	    char *cp = line + parsed;
	    unsigned hypID;
	    while (sscanf(cp, "%u%n", &hypID, &parsed) == 1) {
		(*hypList)[hypList->size()] = hypID;
		*allHyps.insert(hypID) = hypID;

		cp += parsed;
	    }
	} else if (sscanf(line, "info %u %100s%n", &index, arg1, &parsed) == 2){
	    assert(index < numAligns);
	    sortedAligns[index] = index;

	    VocabIndex word = vocab.addWord(arg1);
	    NBestWordInfo *winfo = wordInfo[index]->insert(word);

	    winfo->word = word;
	    if (!winfo->parse(line + parsed)) {
		file.position() << "invalid word info\n";
		return false;
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
 * If hypID != 0, then *hypID will record a sentence hyp ID for the 
 * aligned words.
 */
void
WordMesh::alignWords(const VocabIndex *words, Prob score,
			    Prob *wordScores, const HypID *hypID)
{
    unsigned numWords = Vocab::length(words);
    NBestWordInfo winfo[numWords + 1];

    /*
     * Fill word info array with word IDs and dummy info
     */
    for (unsigned i = 0; i <= numWords; i ++) {
	winfo[i].word = words[i];
	winfo[i].invalidate();
    }

    alignWords(winfo, score, wordScores, hypID);
}

void
WordMesh::alignWords(const NBestWordInfo *winfo, Prob score,
			    Prob *wordScores, const HypID *hypID)
{
    unsigned refLength = numAligns;
    unsigned hypLength = 0;
    for (unsigned i = 0; winfo[i].word != Vocab_None; i ++) hypLength ++;

    typedef struct {
	double cost;			// minimal cost of partial alignment
	WordAlignType error;		// best predecessor
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
    chart[0][0].error = CORR_ALIGN;

    for (unsigned j = 1; j <= hypLength; j ++) {
	/* insert error prob = totalPosterior */
	if (distance) {
	    chart[0][j].cost = chart[0][j-1].cost + totalPosterior *
					distance->penalty(winfo[j-1].word);
	} else {
	    chart[0][j].cost = chart[0][j-1].cost + totalPosterior;
	}
	chart[0][j].error = INS_ALIGN;
    }

    /*
     * Fill in the rest of the chart, row by row.
     */
    for (unsigned i = 1; i <= refLength; i ++) {

	/* deletion error prob = totalPosterior - deletion prob */
	double deletePenalty;
	if (distance) {
	    deletePenalty = 0.0;

	    LHashIter<VocabIndex,Prob> iter(*aligns[sortedAligns[i-1]]);
	    Prob *prob;
	    VocabIndex alignWord;
	    while (prob = iter.next(alignWord)) {
		if (alignWord != deleteIndex) {
		    deletePenalty += *prob * distance->penalty(alignWord);
		}
	    }
	} else {
	    Prob *deleteProb = aligns[sortedAligns[i-1]]->find(deleteIndex);
	    deletePenalty = totalPosterior - (deleteProb ? *deleteProb : 0.0);
	}

	chart[i][0].cost = chart[i-1][0].cost + deletePenalty;
	chart[i][0].error = DEL_ALIGN;

	for (unsigned j = 1; j <= hypLength; j ++) {
	    double minCost;
	    WordAlignType minError;

	    Prob *wordProb = aligns[sortedAligns[i-1]]->find(winfo[j-1].word);

	    if (distance) {
		/*
		 * Compute distance to existing alignment as a weighted 
		 * combination of distances
		 */
		double totalDistance = 0.0;

	    	LHashIter<VocabIndex,Prob> iter(*aligns[sortedAligns[i-1]]);
		Prob *prob;
		VocabIndex alignWord;
		while (prob = iter.next(alignWord)) {
		    if (alignWord == deleteIndex) {
			totalDistance +=
			    *prob * distance->penalty(winfo[j-1].word);
		    } else {
			totalDistance +=
			    *prob * distance->distance(alignWord, winfo[j-1].word);
		    }
		}

		minCost = chart[i-1][j-1].cost + totalDistance;
		minError = SUB_ALIGN;
	    } else if (wordProb) {
		minCost = chart[i-1][j-1].cost + totalPosterior - *wordProb;
		minError = CORR_ALIGN;
	    } else {
		minCost = chart[i-1][j-1].cost + totalPosterior;
		minError = SUB_ALIGN;
	    }

	    double delCost = chart[i-1][j].cost + deletePenalty;
	    if (delCost < minCost) {
		minCost = delCost;
		minError = DEL_ALIGN;
	    }

	    double insCost = chart[i][j-1].cost;
	    if (distance) {
		insCost += totalPosterior * distance->penalty(winfo[j-1].word);
	    } else {
		insCost += totalPosterior;
	    }
	    if (insCost < minCost) {
		minCost = insCost;
		minError = INS_ALIGN;
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

	    case CORR_ALIGN:
	    case SUB_ALIGN:
		*aligns[sortedAligns[i-1]]->insert(winfo[j-1].word) += score;

		/*
		 * Check for preexisting word info and merge if necesssary
		 */
		if (winfo[j-1].valid()) {
		    Boolean foundP;
		    NBestWordInfo *oldInfo =
		    		wordInfo[sortedAligns[i-1]]->
						insert(winfo[j-1].word, foundP);
		    if (foundP) {
			oldInfo->merge(winfo[j-1]);
		    } else {
			*oldInfo = winfo[j-1];
		    }
		}

		if (hypID) {
		    /*
		     * Add hyp ID to the hyp list for this word 
		     */
		    Array<HypID> &hypList = 
			*hypMap[sortedAligns[i-1]]->insert(winfo[j-1].word);
		    hypList[hypList.size()] = *hypID;
		}

		if (wordScores) {
		    wordScores[j-1] =
			    *aligns[sortedAligns[i-1]]->insert(winfo[j-1].word);
		}

		i --; j --;
		break;
	    case DEL_ALIGN:
		*aligns[sortedAligns[i-1]]->insert(deleteIndex) += score;

		if (hypID) {
		    /*
		     * Add hyp ID to the hyp list for this word 
		     */
		    Array<HypID> &hypList = 
			*hypMap[sortedAligns[i-1]]->insert(deleteIndex);
		    hypList[hypList.size()] = *hypID;
		}

		i --;
		break;
	    case INS_ALIGN:
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

		wordInfo[numAligns] = new LHash<VocabIndex,NBestWordInfo>;
		assert(wordInfo[numAligns] != 0);

		hypMap[numAligns] = new LHash<VocabIndex,Array<HypID> >;
		assert(hypMap[numAligns] != 0);

		if (totalPosterior != 0.0) {
		    *aligns[numAligns]->insert(deleteIndex) = totalPosterior;
		}
		*aligns[numAligns]->insert(winfo[j-1].word) = score;

		/*
		 * Record word info if given
		 */
		if (winfo[j-1].valid()) {
		    *wordInfo[numAligns]->insert(winfo[j-1].word) = winfo[j-1];
		}

		/*
		 * Add all hypIDs previously recorded to the *DELETE*
		 * hypothesis at the newly created position.
		 */
		{
		    Array<HypID> *hypList = 0;

		    SArrayIter<HypID,HypID> myIter(allHyps);
		    HypID id;
		    while (myIter.next(id)) {
			/*
			 * Avoid inserting *DELETE* in hypMap unless needed
			 */
			if (hypList == 0) {
			    hypList = hypMap[numAligns]->insert(deleteIndex);
			}
			(*hypList)[hypList->size()] = id;
		    }
		}

		if (hypID) {
		    /*
		     * Add hyp ID to the hyp list for this word 
		     */
		    Array<HypID> &hypList = 
				*hypMap[numAligns]->insert(winfo[j-1].word);
		    hypList[hypList.size()] = *hypID;
		}

		if (wordScores) {
		    wordScores[j-1] = score;
		}

		numAligns ++;
		j --;
		break;
	    }
	}
    }

    /*
     * Add hyp to global list of IDs
     */
    if (hypID) {
	*allHyps.insert(*hypID) = *hypID;
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

    typedef struct {
	unsigned cost;			// minimal cost of partial alignment
	WordAlignType error;		// best predecessor
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
	chart[0][0].error = CORR_ALIGN;

	for (unsigned j = 1; j <= maxHypLength; j ++) {
	    chart[0][j].cost = chart[0][j-1].cost + INS_COST;
	    chart[0][j].error = INS_ALIGN;
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
	chart[i][0].error = DEL_ALIGN;

	for (unsigned j = 1; j <= hypLength; j ++) {
	    unsigned minCost;
	    WordAlignType minError;
	
	    if (aligns[sortedAligns[i-1]]->find(words[j-1])) {
		minCost = chart[i-1][j-1].cost;
		minError = CORR_ALIGN;
	    } else {
		minCost = chart[i-1][j-1].cost + SUB_COST;
		minError = SUB_ALIGN;
	    }

	    unsigned delCost = chart[i-1][j].cost + THIS_DEL_COST;
	    if (delCost < minCost) {
		minCost = delCost;
		minError = DEL_ALIGN;
	    }

	    unsigned insCost = chart[i][j-1].cost + INS_COST;
	    if (insCost < minCost) {
		minCost = insCost;
		minError = INS_ALIGN;
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
	    case CORR_ALIGN:
		i --; j--;
		break;
	    case SUB_ALIGN:
		sub ++;
		i --; j --;
		break;
	    case DEL_ALIGN:
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
	    case INS_ALIGN:
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
				unsigned flags, double delBias)
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
	    Prob effectiveProb = *prob; // prob adjusted for deletion bias

	    if (word == deleteIndex) {
		effectiveProb *= delBias;
		deleteProb = effectiveProb;
	    }
	    if (bestWord == Vocab_None || effectiveProb > bestProb) {
		bestWord = word;
		bestProb = effectiveProb;
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

