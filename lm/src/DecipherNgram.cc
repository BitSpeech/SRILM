/*
 * DecipherNgram.cc --
 *	Approximate N-gram backoff language model used in Decipher recognizer
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/DecipherNgram.cc,v 1.6 1997/11/03 21:40:54 stolcke Exp $";
#endif

#include <iostream.h>
#include <stdlib.h>

#include "DecipherNgram.h"

#define DEBUG_NGRAM_HITS 2	/* from Ngram.cc */

DecipherNgram::DecipherNgram(Vocab &vocab, unsigned order)
      : Ngram(vocab, order), backoffHack(true)
{
}

#define RoundToBytelog(x)	BytelogToLogP(LogPtoBytelog(x))

/*
 * The backoff algorithm is similar to Ngram:wordProbBO(),
 * with the following modifications:
 * - bows and probs are truncated to ByteLog precision
 * - the backed-off probability is returned if it is larger than the
 *   direct ngram probability.
 */
LogP
DecipherNgram::wordProbBO(VocabIndex word, const VocabIndex *context, unsigned int clen)
{
    LogP result;
    /*
     * To use the context array for lookup we truncate it to the
     * indicated length, restoring the mangled item on exit.
     */
    VocabIndex saved = context[clen];
    ((VocabIndex *)context)[clen] = Vocab_None;	/* discard const */

    LogP *prob = findProb(word, context);
    LogP boProb = LogP_Zero;

    /*
     * Compute the backed-off probability always
     */
    if (clen > 0) {
	LogP *bow = findBOW(context);

	if (bow) {
	   boProb = RoundToBytelog(*bow) + wordProbBO(word, context, clen - 1);
	} else {
	   boProb = wordProbBO(word, context, clen - 1);
	}
    }
    
    if (prob) {
	result = RoundToBytelog(*prob);

	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "[" << (clen + 1) << "gram]";
	}

	if (backoffHack && boProb > result) {
	    result = boProb;

	    if (running() && debug(DEBUG_NGRAM_HITS)) {
		dout() << "[bo]";
	    }
	}
    } else {
	result = boProb;
    }
    ((VocabIndex *)context)[clen] = saved;	/* discard const */
    return result;
}

