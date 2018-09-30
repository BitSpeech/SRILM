/*
 * MultiAlign.h --
 *	Multiple Word alignments
 *
 * Copyright (c) 1998 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/MultiAlign.h,v 1.3 1998/02/21 00:31:24 stolcke Exp $
 *
 */

#ifndef _MultiAlign_h_
#define _MultiAlign_h_

#include "Boolean.h"
#include "File.h"
#include "Vocab.h"
#include "Prob.h"

class MultiAlign
{
public:
    MultiAlign(Vocab &vocab) : vocab(vocab) {};
    virtual ~MultiAlign() {};

    Vocab &vocab;		// vocabulary used for words

    virtual Boolean read(File &file) = 0;
    virtual Boolean write(File &file) = 0;

    /*
     * add word string without forcing alignment
     */
    virtual void addWords(const VocabIndex *words, Prob score)
	{ alignWords(words, score); };

    /*
     * add word string forcing alignment
     * if posterior array is given, return word posterior probs.
     */
    virtual void alignWords(const VocabIndex *words, Prob score,
						Prob *wordScores = 0) = 0;

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
				unsigned flags = 0) = 0;

    virtual Boolean isEmpty() = 0;
};

#endif /* _MultiAlign_h_ */

