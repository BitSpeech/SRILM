/*
 * Ngram.h --
 *	N-gram backoff language models
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/Ngram.h,v 1.30 2001/01/19 01:54:33 stolcke Exp $
 *
 */

#ifndef _Ngram_h_
#define _Ngram_h_

#include <stdio.h>

#include "LM.h"
#include "NgramStats.h"
#include "Discount.h"

#ifdef USE_SARRAY

# define PROB_INDEX_T   SArray
# define PROB_ITER_T    SArrayIter
# include "SArray.h"

#else /* ! USE_SARRAY */

# define PROB_INDEX_T   LHash
# define PROB_ITER_T    LHashIter
# include "LHash.h"

#endif /* USE_SARRAY */

#include "Trie.h"

typedef struct {
    LogP			bow;		/* backoff weight */
    PROB_INDEX_T<VocabIndex,LogP>	probs;	/* word probabilities */
} BOnode;

typedef Trie<VocabIndex,BOnode> BOtrie;

const unsigned defaultNgramOrder = 3;

class Ngram: public LM
{
    friend class NgramBOsIter;

public:
    Ngram(Vocab &vocab, unsigned order = defaultNgramOrder);
    virtual ~Ngram() {};

    unsigned setorder(unsigned neworder = 0);   /* change/return ngram order */

    /*
     * LM interface
     */
    virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
    virtual void *contextID(const VocabIndex *context, unsigned &length);

    virtual Boolean read(File &file);
    virtual void write(File &file) { writeWithOrder(file, order); };
    virtual void writeWithOrder(File &file, unsigned int order);

    Boolean skipOOVs;		/* backward compatiability: return
				 * zero prob if <unk> is in context */
    Boolean trustTotals;	/* use lower-order counts for ngram totals */

    /*
     * Estimation
     */
    virtual Boolean estimate(NgramStats &stats,
			unsigned *mincount = 0,
			unsigned *maxcounts = 0);
    virtual Boolean estimate(NgramStats &stats, Discount **discounts);
    virtual Boolean estimate(NgramCounts<FloatCount> &stats,
							Discount **discounts);
    virtual void mixProbs(Ngram &lm1, Ngram &lm2, double lambda);
    virtual void recomputeBOWs();
    virtual void pruneProbs(double threshold, unsigned minorder = 2);
    virtual void pruneLowProbs(unsigned minorder = 2);

    /*
     * Statistics
     */
    virtual unsigned int numNgrams(unsigned int n);
    virtual void memStats(MemStats &stats);

    VocabIndex dummyIndex;			/* <dummy> tag */

    /*
     * Low-level access
     */
    LogP *findBOW(const VocabIndex *context);
    LogP *insertBOW(const VocabIndex *context);
    LogP *findProb(VocabIndex word, const VocabIndex *context);
    LogP *insertProb(VocabIndex word, const VocabIndex *context);
    void removeBOW(const VocabIndex *context);
    void removeProb(VocabIndex word, const VocabIndex *context);

protected:
    BOtrie contexts;				/* n-1 gram context trie */
    unsigned int order; 			/* maximal ngram order */

    /*
     * Helper functions
     */
    virtual LogP wordProbBO(VocabIndex word, const VocabIndex *context,
							unsigned int clen);
    template <class CountType>
	Boolean estimate2(NgramCounts<CountType> &stats, Discount **discounts);
    virtual void fixupProbs();
    virtual void distributeProb(Prob mass, VocabIndex *context);
    virtual LogP computeContextProb(const VocabIndex *context);
    virtual Boolean computeBOW(BOnode *node, const VocabIndex *context, 
			    unsigned clen, Prob &numerator, Prob &denominator);
};

/*
 * Iteration over all backoff nodes of a given order
 */
class NgramBOsIter
{
public:
    NgramBOsIter(Ngram &lm, VocabIndex *keys, unsigned order,
			int (*sort)(VocabIndex, VocabIndex) = 0)
	 : myIter(lm.contexts, keys, order, sort) {};

    void init() { myIter.init(); };
    BOnode *next()
	{ Trie<VocabIndex,BOnode> *node = myIter.next();
	  return node ? &(node->value()) : 0; }
private:
    TrieIter2<VocabIndex,BOnode> myIter;
};

/*
 * Iteration over all probs at a backoff node
 */
class NgramProbsIter
{
public:
    NgramProbsIter(BOnode &bonode, 
			int (*sort)(VocabIndex, VocabIndex) = 0)
	: myIter(bonode.probs, sort) {};

    void init() { myIter.init(); };
    LogP *next(VocabIndex &word) { return myIter.next(word); };

private:
    PROB_ITER_T<VocabIndex,LogP> myIter;
};

#endif /* _Ngram_h_ */
