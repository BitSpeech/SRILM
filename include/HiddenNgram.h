/*
 * HiddenNgram.h --
 *	N-gram model with hidden between-word events
 *
 * Copyright (c) 1999, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/HiddenNgram.h,v 1.2 1999/05/13 07:28:23 stolcke Exp $
 *
 */

#ifndef _HiddenNgram_h_
#define _HiddenNgram_h_

#include <stdio.h>

#include "Ngram.h"
#include "Trellis.h"
#include "SubVocab.h"

/* 
 * We use strings over VocabIndex as keys into the trellis.
 */
typedef const VocabIndex *VocabContext;

/*
 * A N-gram language model that sums of hidden events between the
 * observable words, in the style of Stolcke et al., Automatic Detection of
 * Sentence Boundaries and Disfluencies based on Recognized Words. Proc.
 * Intl. Conf. on Spoken Language Processing, vol. 5, pp.  2247-2250, Sydney,
 * 1998.
 */
class HiddenNgram: public Ngram
{
public:
    HiddenNgram(Vocab &vocab, SubVocab &hiddenVocab, unsigned order,
						    Boolean nothidden = false);
    ~HiddenNgram();

    /*
     * LM interface
     */
    LogP wordProb(VocabIndex word, const VocabIndex *context);
    LogP wordProbRecompute(VocabIndex word, const VocabIndex *context);
    Boolean isNonWord(VocabIndex word);
    LogP sentenceProb(const VocabIndex *sentence, TextStats &stats);

protected:
    Trellis<VocabContext> trellis;	/* for DP on hidden events */
    unsigned contextLength;		/* length of last DP context */
    const VocabIndex *prevContext;	/* context from last DP */
    LogP prefixProb(VocabIndex word, const VocabIndex *context,
				LogP &contextProb, TextStats &stats);
					/* prefix probability */

    SubVocab &hiddenVocab;		/* the hidden event vocabulary */
    VocabIndex noEventIndex;		/* the "no-event" event */
    Boolean notHidden;			/* overt event mode */
};

#endif /* _HiddenNgram_h_ */
