/*
 * HiddenSNgram.cc --
 *	N-gram backoff language model with hidden sentence boundaries
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/spot71/srilm/devel/lm/src/RCS/HiddenSNgram.cc,v 1.1 1996/09/14 22:45:42 stolcke Exp $";
#endif

#include <iostream.h>
#include <stdlib.h>

#include "HiddenSNgram.h"

#define DEBUG_PRINT_WORD_PROBS  2       /* from LM.cc */

HiddenSNgram::HiddenSNgram(Vocab &vocab, unsigned order)
    : Ngram(vocab, order)
{
    hiddenSIndex = vocab.addWord(HiddenSent);
}

/*
 * LM interface
 */

LogP
HiddenSNgram::wordProb(VocabIndex word, const VocabIndex *context)
{
    VocabIndex hiddenSContext[2];

    hiddenSContext[0] = hiddenSIndex;
    hiddenSContext[1] = Vocab_None;

    /*
     * Quick hack: take the max if p(w|context) and p(w|<#s>) p(<#s>|context).
     * Replace this with dynamic programming later.
     */

    LogP p1 = Ngram::wordProb(word, context);
    LogP p2 = Ngram::wordProb(hiddenSIndex, context) + 
		  Ngram::wordProb(word, hiddenSContext);

    if (p1 >= p2) {
	return p1;
    } else {
	if (running() && debug(DEBUG_PRINT_WORD_PROBS)) {
	    dout() << "[" << HiddenSent << "]";
	}
	return p2;
    }
}

