/*
 * MultiwordLM.cc --
 *	Multiword wrapper language model
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2001,2002 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/MultiwordLM.cc,v 1.3 2002/08/25 17:27:45 stolcke Exp $";
#endif

#include <stdlib.h>

#include "MultiwordLM.h"

LogP
MultiwordLM::wordProb(VocabIndex word, const VocabIndex *context)
{
    /*
     * buffer holding expanded context, with room to prepend expanded
     * word
     */
    VocabIndex expandedBuffer[2 * maxWordsPerLine + 1];

    /*
     * expand the context with all multiwords
     */
    VocabIndex *expandedContext = &expandedBuffer[maxWordsPerLine];
    unsigned expandedContextLength =
	vocab.expandMultiwords(context, expandedContext, maxWordsPerLine, true);

    VocabIndex multiWord[2];
    multiWord[0] = word;
    multiWord[1] = Vocab_None;

    VocabIndex expandedWord[maxWordsPerLine + 1];
    unsigned expandedWordLength =
	    vocab.expandMultiwords(multiWord, expandedWord, maxWordsPerLine);

    LogP prob = LogP_One;
    for (unsigned j = 0; j < expandedWordLength; j ++) {
	prob += lm.wordProb(expandedWord[j],
			    &expandedBuffer[maxWordsPerLine - j]);

	expandedBuffer[maxWordsPerLine - 1 - j] = expandedWord[j];
    }

    return prob;
}

void *
MultiwordLM::contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length)
{
    VocabIndex expandedContext[maxWordsPerLine + 1];

    unsigned expandedContextLength =
	vocab.expandMultiwords(context, expandedContext, maxWordsPerLine, true);

    /*
     * Overestimate the used context length by using the context length used 
     * in the underlying LM.  But make sure we don't claim more than the 
     * actual length of the context.
     */
    void *cid = lm.contextID(expandedContext, length);
    if (length > expandedContextLength) {
	length = expandedContextLength;
    }

    return cid;
}

Boolean
MultiwordLM::isNonWord(VocabIndex word)
{
    /*
     * Map candidate word to underlying LM vocab, and check if it is 
     * a non-word there.
     */
    VocabIndex oneWord[2];
    oneWord[0] = word;
    oneWord[1] = Vocab_None;

    VocabIndex expanded[2];
    unsigned expandedLength = vocab.expandMultiwords(oneWord, expanded, 2);

    return (expandedLength == 1) && lm.isNonWord(expanded[0]);
}

void
MultiwordLM::setState(const char *state)
{
    /*
     * Global state changes are propagated to the underlying models
     */
    lm.setState(state);
}

