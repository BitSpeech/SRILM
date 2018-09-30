/*
 * MultiAlign.h --
 *	Multiple Word alignments
 *
 * Copyright (c) 1998,2001 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/MultiAlign.h,v 1.8 2001/08/07 17:38:17 stolcke Exp $
 *
 */

#ifndef _MultiAlign_h_
#define _MultiAlign_h_

#include "Boolean.h"
#include "File.h"
#include "Vocab.h"
#include "Prob.h"
#include "NBest.h"

typedef unsigned short HypID;	/* index identifying a sentence in the
				 * alignment (short to save space) */

const HypID refID = (HypID)-2; 	/* pseudo-hyp ID used to identify the reference
				 * in an N-best alignment
				 * (make sure !Map_noKeyP(refID)) */

class MultiAlign
{
public:
    MultiAlign(Vocab &vocab) : vocab(vocab) {};
    virtual ~MultiAlign() {};

    Vocab &vocab;		// vocabulary used for words

    virtual Boolean read(File &file) = 0;
    virtual Boolean write(File &file) = 0;

    /*
     * add word string without forcing alignment (optional)
     */
    virtual void addWords(const VocabIndex *words, Prob score,
							const HypID *hypID = 0)
	{ alignWords(words, score, 0, hypID); };
    virtual void addWords(const NBestWordInfo *winfo, Prob score,
							const HypID *hypID = 0)
	{ alignWords(winfo, score, 0, hypID); };

    /*
     * add word string forcing alignment
     * if posterior array is given, return word posterior probs.
     * if hypID is non-null, record sentence ID for words
     */
    virtual void alignWords(const VocabIndex *words, Prob score,
			    Prob *wordScores = 0, const HypID *hypID = 0) = 0;
    /*
     * default for aligning NBestWordInfo array is to just align the words
     */
    virtual void alignWords(const NBestWordInfo *winfo, Prob score,
			    Prob *wordScores = 0, const HypID *hypID = 0)
	{ unsigned numWords = 0;
	  for (unsigned i = 0; winfo[i].word != Vocab_None; i ++) numWords ++;
	  VocabIndex words[numWords + 1];
	  for (unsigned i = 0; i <= numWords; i ++) words[i] = winfo[i].word;
	  alignWords(words, score, wordScores, hypID);
	};

    /*
     * record reference word string (optional)
     */
    void alignReference(const VocabIndex *words)
	{ HypID id = refID;
	  alignWords(words, 0.0, 0, &id); };

    /*
     * compute minimal word errr
     */
    virtual unsigned wordError(const VocabIndex *words,
			    unsigned &sub, unsigned &ins, unsigned &del) = 0;

    /*
     * compute minimum expected word error string
     */
    virtual double minimizeWordError(VocabIndex *words, unsigned length,
				double &sub, double &ins, double &del,
				unsigned flags = 0, double delBias = 1.0) = 0;

    virtual Boolean isEmpty() = 0;
};

#endif /* _MultiAlign_h_ */

