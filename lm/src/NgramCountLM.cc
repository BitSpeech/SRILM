/*
 * NgramCountLM.cc --
 *	LM based on interpolated N-gram counts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2006-2010 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/lm/src/NgramCountLM.cc,v 1.14 2010/06/02 05:49:58 stolcke Exp $";
#endif

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <stdlib.h>
#include <math.h>
#if !defined(_MSC_VER) && !defined(WIN32)
#include <sys/param.h>
#endif
#include <assert.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#include "NgramCountLM.h"
#include "File.h"
#include "Array.cc"

/*
 * Debug levels used
 */
#define DEBUG_NGRAM_HITS 2
#define DEBUG_PRINT_WORD_PROBS	2
#define DEBUG_PRINT_POSTERIORS	1
#define DEBUG_PRINT_LIKELIHOODS	0

NgramCountLM::NgramCountLM(Vocab &vocab, unsigned neworder)
    : LM(vocab), order(neworder), ngramCounts(vocab, neworder)
{
    if (order < 1) {
    	order = 1;
    }

    numWeights = 0;
    for (unsigned j = 0; j < order; j ++) {
	mixWeights[0][j] = 0.0;
    }

    countModulus = 1;
    totalCount = 0;
    vocabSize = 0;
    countsName = 0;
    useGoogle = false;
    writeCounts = true;
    training = false;

    /*
     * create a buffer into which ngrams can be copied in left-to-right order
     * for count lookup
     */
    ngramBuffer = new VocabIndex[order + 1];
    assert(ngramBuffer != 0);
}

NgramCountLM::~NgramCountLM()
{
    clear();
    delete ngramBuffer;
}

void
NgramCountLM::clear()
{
    ngramCounts.clear();

    /* reset all weights */
    for (unsigned i = 0; i <= numWeights; i ++) {
	for (unsigned j = 0; j < order; j ++) {
	    mixWeights[i][j] = 0.0;
	}
    }
    numWeights = 0;
    countModulus = 1;

    /* reset total statistics */
    totalCount = 0;
    vocabSize = 0;

    if (countsName) free(countsName);
    countsName = 0;
    useGoogle = false;
}

void
NgramCountLM::memStats(MemStats &stats)
{
    stats.total += sizeof(*this);

    stats.total -= sizeof(ngramCounts);
    ngramCounts.memStats(stats);

    stats.total -= sizeof(mixWeights);
    mixWeights.memStats(stats);

    for (unsigned i = 0; i <= numWeights; i ++) {
	stats.total -= sizeof(mixWeights[i]);
	mixWeights[i].memStats(stats);
    }
}

/*
 * Helper function to fill the ngram buffer with left-to-right ngram
 * Returns start of valid ngram (pointer into ngramBuffer)
 */
VocabIndex *
NgramCountLM::setupNgram(VocabIndex word, const VocabIndex *context)
{
    unsigned clen;
    ngramBuffer[order - 1] = word;
    ngramBuffer[order] = Vocab_None;

    for (clen = 0; clen < order - 1 && context[clen] != Vocab_None; clen ++) {
    	ngramBuffer[order - clen - 2] = context[clen];
    }

    return &ngramBuffer[order - clen - 1];
}

/*
 * Tally vocabulary size and total unigram counts (if not already set)
 */
void
NgramCountLM::computeTotals()
{
    /*
     * Vocabulary size 
     */
    if (vocabSize == 0) {
	VocabIter iter(vocab);
	VocabIndex wid;

	vocabSize = 0;

	while (iter.next(wid)) {
	    if (!isNonWord(wid)) {
		vocabSize += 1;
	    }
	}

	if (vocabSize == 0) {
	    vocabSize = 1;
	}
    }

    /*
     * Total count
     */
    if (totalCount == 0) {
        /*
	 * Sum all unigram counts 
	 */
	ngramCounts.sumCounts(1);

	VocabIndex empty[1]; empty[0] = Vocab_None;
	NgramCount *root = ngramCounts.findCount(empty);

	/*
	 * subtract the <s> count
	 */
	VocabIndex sstart[2];
	sstart[0] = vocab.ssIndex();
	sstart[1] = Vocab_None;

	NgramCount *ssCount = ngramCounts.findCount(sstart);
	if (ssCount != 0) {
	    *root -= *ssCount;
	}

	/*
	 * Make sure total is at least 1
	 */
	if (*root == 0) {
	    *root = 1;
	}

	totalCount = *root;

	if (debug(DEBUG_NGRAM_HITS)) {
	    dout() << "vocabsize = " << vocabSize
		   << " totalcount = " << totalCount
		   << endl;
	}
    }
}

Boolean
NgramCountLM::write(File &file)
{
    fprintf(file, "order %u\n", order);
    fprintf(file, "mixweights %u\n", numWeights);
    for (unsigned i = 0; i <= numWeights; i ++) {
    	for (unsigned j = 0; j < order; j ++) {
	    fprintf(file, " %lg", mixWeights[i][j]);
	}
	fprintf(file, "\n");
    }

    fprintf(file, "countmodulus %s\n", countToString(countModulus));
    fprintf(file, "vocabsize %s\n", countToString(vocabSize));
    fprintf(file, "totalcount %s\n", countToString(totalCount));

    if (countsName == 0 || writeCounts & !useGoogle) {
	fprintf(file, "counts -\n");
	if (writeInBinary) {
	    if (!ngramCounts.writeBinary(file, 0)) {
		return false;
	    }
	} else {
	    ngramCounts.write(file, 0);
	}
    } else if (useGoogle) {
	fprintf(file, "google-counts %s\n", countsName);
    } else {
	fprintf(file, "counts %s\n", countsName);
    }

    return true;
}

Boolean
NgramCountLM::read(File &file, Boolean limitVocab)
{
    char *line;

    clear();

    while ((line = file.getline())) {
	char arg1[MAXPATHLEN];
	double arg2;
	unsigned arg3;

	if (sscanf(line, "order %u", &arg3) == 1) {
	    /* changes in order are ignored */
	    ;
	} else if (sscanf(line, "vocabsize %99s", arg1) == 1 &&
		   stringToCount(arg1, vocabSize)) {
	    ;
	} else if (sscanf(line, "totalcount %99s", arg1) == 1 &&
		   stringToCount(arg1, totalCount)) {
	    /* 
	     * copy total count into the unigram trie root
	     */
	    VocabIndex empty[1]; empty[0] = Vocab_None;
	    *ngramCounts.findCount(empty) = totalCount;

	} else if (sscanf(line, "countmodulus %99s", arg1) == 1 &&
		   stringToCount(arg1, countModulus)) {
	    ;
	} else if (sscanf(line, "mixweights %u", &arg3) == 1) {
	    numWeights = arg3;
	    for (unsigned i = 0; i <= numWeights; i ++) {
	    	line = file.getline();
		if (!line) {
		    file.position() << "premature end to mixture weights\n";
		    return false;
		}

		for (unsigned j = 0; j < order; j ++) {
		    int scanned;
		    if (sscanf(line, "%lg%n", &arg2, &scanned) == 1) {
		    	mixWeights[i][j] = arg2;
			line += scanned;
		    } else {
			file.position() << "incomplete mixture weight vector\n";
			return false;
		    }
		}
	    }
	} else if (sscanf(line, "counts %1023s", arg1) == 1) {
	    ngramCounts.clear();
	    useGoogle = false;

	    if (strcmp(arg1, "-") == 0) {
	    	if (countsName) free(countsName);
		countsName = 0;
		if (!ngramCounts.read(file, order, limitVocab)) {
		    return false;
		}
	    } else {
	    	if (countsName) free(countsName);
		countsName = strdup(arg1);
		assert(countsName != 0);

	    	File countFile(countsName, "r");
		if (!ngramCounts.read(countFile, order, limitVocab)) {
		    return false;
		}
	    }
	} else if (sscanf(line, "google-counts %1023s", arg1) == 1) {
	    ngramCounts.clear();
	    useGoogle = true;

	    if (countsName) free(countsName);
	    countsName = strdup(arg1);
	    assert(countsName != 0);

	    if (!ngramCounts.readGoogle(countsName, order, limitVocab)) {
		return false;
	    }
	} else {
	    file.position() << "unknown keyword\n";
	    return false;
	}
    }

    computeTotals();

    return true;
}

LogP
NgramCountLM::wordProb(VocabIndex word, const VocabIndex *context)
{
    if (isNonWord(word)) {
    	return LogP_Zero;
    }

    if (training) {
    	return wordProbTrain(word, context);
    }

    VocabIndex *ngram = setupNgram(word, context);
    unsigned nlen = Vocab::length(ngram);

    Prob p = 1.0 / vocabSize;

    if (running() && debug(DEBUG_NGRAM_HITS)) {
	dout() << "[" << totalCount;
    }

    for (unsigned i = 1; i <= order && i <= nlen; i ++) {
        VocabIndex *useNgram = &ngram[nlen - i];

	ngram[nlen - 1] = Vocab_None;
	NgramCount *denom = ngramCounts.findCount(useNgram);

	ngram[nlen - 1] = word;
	NgramCount *numer = ngramCounts.findCount(useNgram);

	Count scaledDenom = (denom == 0) ? (Count)0 : *denom / countModulus;

	Count contextCount = 
		    scaledDenom <= numWeights ? scaledDenom : (Count)numWeights;

	Prob lambda = mixWeights[contextCount][i-1];

	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "," << lambda
	           << "," << (numer ? (unsigned)*numer : 0);
	}

	if (numer == 0 || *numer == 0) {
	    p = (1 - lambda) * p;
	} else if (denom == 0 || *denom == 0) {
	    cerr << "warning: zero denominator count for ngram "
	         << (vocab.use(), useNgram) << endl;
	    return LogP_Zero;
	} else {
	    p = (1 - lambda) * p + lambda * ((Prob)*numer / (Prob)*denom);
	}
    }

    if (running() && debug(DEBUG_NGRAM_HITS)) {
	dout() << "]";
    }

    return ProbToLogP(p);
}

void *
NgramCountLM::contextID(VocabIndex word, const VocabIndex *context,
						unsigned &length)
{
    /*
     * Note: word is ignored, since we do not compute backoff weights
     */
    VocabIndex *ngram = setupNgram(Vocab_None, context);

    /*
     * Find the longest context that has non-zero count
     */
    for (unsigned i = 0; ngram[i] != Vocab_None; i ++) {
	NgramCount *cnt = ngramCounts.findCount(&ngram[i]);

	if (cnt != 0 && *cnt > 0) {
	    length = Vocab::length(&ngram[i]);
	    return (void *)cnt;
	}
    }

    length = 0;
    return (void *)-1;
}

/*
 * Mixture weight estimation --
 * 	- enable training mode
 *	- clear posterior statistics
 *	- run probability computation over training counts, accumulating
 *	  posterior statistics
 *	- reestimate weights
 *	- repeat until convergence
 */
Boolean
NgramCountLM::estimate(NgramStats &counts)
{
    training = true;

    LogP like;

    for (unsigned iter = 0; iter < maxEMiters; iter ++) {
	/*
	 * Clear posterior statistics
	 */
	for (unsigned i = 0; i <= numWeights; i ++) {
	    for (unsigned j = 0; j < order; j ++) {
		mixCounts[i][j] = 0.0;
		mixCountTotals[i][j] = 0.0;
	    }
	}

	/*
	 * Compute training set likelihood (collecting counts as a side-effect)
	 * Disable debugging output while doing this.
	 */
	unsigned oldlevel = debuglevel();
	debugme(0);

	TextStats stats;
	LogP newLike = countsProb(counts, stats, order);

	debugme(oldlevel);

	if (debug(DEBUG_PRINT_LIKELIHOODS)) {
	   dout() << "iteration " << iter << ": "
		  << "log likelihood = " << newLike
		  << endl;
	}

	/*
	 * Check for convergence
	 */
	if (iter > 1 && fabs((newLike - like)/like) < minEMdelta) {
	    break;
	}

	/*
	 * Reestimate weights
	 */
	for (unsigned i = 0; i <= numWeights; i ++) {
	    if (debug(DEBUG_PRINT_POSTERIORS)) {
		dout() << "posterior counts " << i << ":";
	    }

	    for (unsigned j = 0; j < order; j ++) {
		if (debug(DEBUG_PRINT_POSTERIORS)) {
		    dout() << " " << mixCounts[i][j]
			   << "/" << mixCountTotals[i][j];
		}

	    	if (mixCountTotals[i][j] > 0.0) {
		    mixWeights[i][j] = mixCounts[i][j] / mixCountTotals[i][j];
		} else if (j > 0) {
		    if (!debug(DEBUG_PRINT_POSTERIORS)) {
			cerr << "warning: no data to estimate mixture weight "
			     << "for count " << i << ", order " << j << endl;
		    }
		}
	    }
	    if (debug(DEBUG_PRINT_POSTERIORS)) {
		dout() << endl;
	    }
	}

	like = newLike;
    }

    training = false;

    return true;
}

/*
 * Variant of wordProb() that computes and accumulates posterior counts
 * for each mixture weight
 */
LogP
NgramCountLM::wordProbTrain(VocabIndex word, const VocabIndex *context)
{
    VocabIndex *ngram = setupNgram(word, context);
    unsigned nlen = Vocab::length(ngram);

    /*
     * skip ngrams that are not of maximal order and don't start with <s>.
     * This ensures we compute the training set likelihoods according to 
     * the model of intended order.
     */
    if (nlen != order && !(nlen > 1 && ngram[0] == vocab.ssIndex())) {
    	return LogP_One;
    }

    Prob p = 1.0 / vocabSize;

    makeArray(Count, contextCounts, nlen);
    makeArray(Prob, posteriors, nlen);		/* posterior probs for selecting
    						 * the higher-order ngram
						 * estimate at each branch */
    unsigned i;

    /*
     * "Forward pass": compute probabilities
     */
    for (i = 1; i <= order && i <= nlen; i ++) {
        VocabIndex *useNgram = &ngram[nlen - i];

	ngram[nlen - 1] = Vocab_None;
	NgramCount *denom = ngramCounts.findCount(useNgram);

	ngram[nlen - 1] = word;
	NgramCount *numer = ngramCounts.findCount(useNgram);

	Count scaledDenom = (denom == 0) ? (Count)0 : *denom / countModulus;

	contextCounts[i-1] =
		   scaledDenom <= numWeights ? scaledDenom : (Count)numWeights;

	Prob lambda = mixWeights[contextCounts[i-1]][i-1];

	if (numer == 0 || *numer == 0) {
	    p = (1 - lambda) * p;
	    posteriors[i-1] = 0.0;
	} else if (denom == 0 || *denom == 0) {
	    cerr << "warning: zero denominator count for ngram "
	         << (vocab.use(), useNgram) << endl;
	    return LogP_Zero;
	} else {
	    Prob p0 = (Prob)*numer / (Prob)*denom;

	    p = (1 - lambda) * p + lambda * p0;
	    if (p > 0.0) {
		posteriors[i-1] = lambda * p0 / p;
	    } else {
		posteriors[i-1] = 0.5;
	    }
	}
    }

    /*
     * "Backward pass": compute posteriors
     */
    Prob backwardCount = 1.0;

    for (i -- ; i > 0; i --) {
	Count contextCount = contextCounts[i-1];

	mixCounts[contextCount][i-1] += backwardCount * posteriors[i-1];
	mixCountTotals[contextCount][i-1] += backwardCount;

	backwardCount = backwardCount * (1.0 - posteriors[i-1]);
    }

    return ProbToLogP(p);
}

