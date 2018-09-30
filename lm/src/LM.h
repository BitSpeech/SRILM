/*
 * LM.h --
 *	Generic LM interface
 *
 * The LM class defines an abstract languge model interface which all
 * other classes refine and inherit from.
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/LM.h,v 1.26 1999/07/17 10:38:06 stolcke Exp $
 *
 */

#ifndef _LM_h_
#define _LM_h_

#include <iostream.h>

#include "Boolean.h"
#include "Prob.h"
#include "File.h"
#include "Vocab.h"
#include "Debug.h"
#include "MemStats.h"

typedef unsigned int Count;		/* a count of something */
typedef double FloatCount;		/* a fractional count */

class LM;		/* forward declaration */
class TextStats;	/* forward declaration */

/*
 * This is the iter class from which more specialized iters can be
 * derived.  Not to be confused with the wrapper object above.
 * The default behavior implemented here is to simply enumerate all
 * words in the vocabulary.
 */
class _LM_FollowIter
{
public:
    _LM_FollowIter(LM &lm, const VocabIndex *context);

    virtual void init();
    virtual VocabIndex next();
    virtual VocabIndex next(LogP &prob);

private:
    LM &myLM;
    const VocabIndex *myContext;
    VocabIter myIter;
};

class LM: public Debug
{
    friend class _LM_FollowIter;

public:
    LM(Vocab &vocab);
    virtual ~LM();

    virtual LogP wordProb(VocabIndex word, const VocabIndex *context) = 0;
    virtual LogP wordProb(VocabString word, const VocabString *context);

    virtual LogP wordProbRecompute(VocabIndex word, const VocabIndex *context);
		    /* recompute word prob using last wordProb() context */

    virtual LogP sentenceProb(const VocabIndex *sentence, TextStats &stats);
    virtual LogP sentenceProb(const VocabString *sentence, TextStats &stats);

    virtual unsigned pplFile(File &file, TextStats &stats,
				const char *escapeString = 0);
    virtual unsigned rescoreFile(File &file, double lmScale, double wtScale,
			       LM &oldLM, double oldLmScale, double oldWtScale,
			       const char *escapeString = 0);

    virtual void setState(const char *state);	/* hook to manipulate global
						   LM state */

    virtual Prob wordProbSum(const VocabIndex *context);
						/* sum of all word probs */

    virtual VocabIndex generateWord(const VocabIndex *context);
    virtual VocabIndex *generateSentence(unsigned maxWords = maxWordsPerLine,
				VocabIndex *sentence = 0);
    virtual VocabString *generateSentence(unsigned maxWords = maxWordsPerLine,
				VocabString *sentence = 0);

    virtual void *contextID(const VocabIndex *context)
	{ unsigned length; return contextID(context, length); };
    virtual void *contextID(const VocabIndex *context, unsigned &length);
				    /* context ID and length used by LM */

    virtual Boolean isNonWord(VocabIndex word);

    virtual Boolean read(File &file);
    virtual void write(File &file);

    virtual Boolean running() { return _running; }
    virtual Boolean running(Boolean newstate)
      { Boolean old = _running; _running = newstate; return old; };

    virtual _LM_FollowIter *followIter(const VocabIndex *context)
	{ return new _LM_FollowIter(*this, context); };

    virtual void memStats(MemStats &stats);

    Vocab &vocab;			/* vocabulary */

    VocabIndex noiseIndex;		/* [noise] tag */

    virtual VocabIndex *removeNoise(VocabIndex *words);
					/* strip noise and pause tags */

    const char *stateTag;		/* tag introducing global state info */

    Boolean reverseWords;		/* compute word probs in reverse */

protected:
    Boolean _running;	/* indicates the LM is being used for sequential
			 * word prob computation */
    unsigned prepareSentence(const VocabIndex *sentence,
				VocabIndex *reversed, unsigned len);
			/* reverse sentence for wordProb computation */
};

/*
 * LMFollowIter --
 *	Iterator enumerating possible follow words and their probabilities
 *
 * The idea here is that the user can declare an iterator 
 *    LM_FollowIter(lm)
 * without refering to the classname of lm itself.  This will create
 * the following wrapper object that contains a pointer to the actual
 * class-specific iterator, using the LM::followIter virtual function.
 * All iterator operations then simply dispatch to the real iterator.
 */
class LM_FollowIter
{
public:
    LM_FollowIter(LM &lm, VocabIndex *context)
	: realIter(lm.followIter(context)) {};
    ~LM_FollowIter() { delete realIter; };

    virtual void init() { realIter->init(); };
    virtual VocabIndex next() { LogP prob; return next(prob); }
    virtual VocabIndex next(LogP &prob) { return realIter->next(prob); }

private:
    _LM_FollowIter *realIter;		/* LM-specific iterator */
};


/*
 * TextStats objects are used to pass and accumulate various 
 * statistics of text sources (training or test).
 */
class TextStats
{
public:
    TextStats() : prob(0.0), zeroProbs(0),
	numSentences(0), numWords(0), numOOVs(0) {};

    void reset() { prob = 0.0, zeroProbs = 0,
	numSentences = numWords = numOOVs = 0; };
    TextStats &increment(TextStats &stats);

    LogP prob;
    unsigned zeroProbs;
    unsigned numSentences;
    unsigned numWords;
    unsigned numOOVs;
};

ostream &operator<<(ostream &, TextStats &stats);

#endif /* _LM_h_ */
