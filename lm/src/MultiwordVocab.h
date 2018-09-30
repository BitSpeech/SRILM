/*
 * MultiwordVocab.h --
 *	Vocabulary containing multiwords
 *
 * A multiword is a vocabulary element consisting of words joined by
 * underscores (or some other delimiter), e.g., "i_don't_know".
 * This class provides support for splitting such words into their components.
 *
 * Copyright (c) 2001 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/MultiwordVocab.h,v 1.1 2001/12/30 23:24:12 stolcke Exp $
 *
 */

#ifndef _MultiwordVocab_h_
#define _MultiwordVocab_h_

#include "Vocab.h"

class MultiwordVocab: public Vocab
{
public:
    MultiwordVocab(VocabIndex start, VocabIndex end,
						const char *multiChar = "_")
	: Vocab(start, end), multiChar(multiChar) {};
    MultiwordVocab(const char *multiChar = "_")
	: multiChar(multiChar) {};
    ~MultiwordVocab();

    /*
     * Modified Vocab methods
     */
    virtual VocabIndex addWord(VocabString name);
    virtual void remove(VocabString name);
    virtual void remove(VocabIndex index);

    /*
     * Expansion of vocab strings into their components
     */
    unsigned expandMultiwords(const VocabIndex *words, VocabIndex *expanded,
				unsigned maxExpanded, Boolean reverse = false);

private:
    LHash< VocabIndex, VocabIndex * > multiwordMap;

    const char *multiChar;
};

#endif /* _MultiwordVocab_h_ */
