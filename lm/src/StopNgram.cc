/*
 * StopNgram.cc --
 *	N-gram LM with stop words removed from context
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1996, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/spot71/srilm/devel/lm/src/RCS/StopNgram.cc,v 1.1 1996/12/10 09:28:49 stolcke Exp $";
#endif

#include "StopNgram.h"

/*
 * Debug levels used
 */
#define DEBUG_NGRAM_HITS 2		/* from Ngram.cc */

StopNgram::StopNgram(Vocab &vocab, SubVocab &stopWords, unsigned neworder)
    : Ngram(vocab, neworder), stopWords(stopWords)
{
}

/*
 * The only difference to a standard Ngram model is that stop words are
 * removed from the context before conditional probabilities are computed.
 */
LogP
StopNgram::wordProb(VocabIndex word, const VocabIndex *context)
{
    LogP result;
    VocabIndex usedContext[maxNgramOrder + 1];

    /*
     * Extract the non-stop words from the context
     */
    unsigned i, j = 0;
    for (i = 0; i < maxNgramOrder && context[i] != Vocab_None ; i++) {
	if (!stopWords.getWord(context[i])) {
	    usedContext[j ++] = context[i];
	}
    }
    usedContext[j] = Vocab_None;

    return Ngram::wordProb(word, usedContext);
}

