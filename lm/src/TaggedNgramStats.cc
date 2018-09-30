/*
 * TaggedNgramStats.cc --
 *	N-gram counting for word/tag pairs
 *
 */

#ifndef lint
static char TaggedNgramStats_Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char TaggedNgramStats_RcsId[] = "@(#)$Header: /home/speech/stolcke/project/srilm/devel/lm/src/RCS/TaggedNgramStats.cc,v 1.2 1996/05/31 05:25:44 stolcke Exp $";
#endif

#include <string.h>
#include <iostream.h>

#include "TaggedNgramStats.h"

TaggedNgramStats::TaggedNgramStats(TaggedVocab &vocab, unsigned int maxOrder)
    : NgramStats(vocab, maxOrder), vocab(vocab)
{
}

void
TaggedNgramStats::incrementTaggedCounts(const VocabIndex *words)
{
    VocabIndex wbuffer[maxWordsPerLine + 1];

    unsigned i;
    for (i = 0; i < order && words[i] != Vocab_None; i++) {
	wbuffer[i] = TaggedVocab::unTag(words[i]);
    }
    wbuffer[i] = Vocab_None;

    incrementCounts(wbuffer);

    for (i = 0; i < order && words[i] != Vocab_None; i++) {
	VocabIndex tag = TaggedVocab::getTag(words[i]);

	if (tag != Tag_None) {
	    wbuffer[i] = TaggedVocab::tagWord(Tagged_None, tag);
	    incrementCounts(wbuffer, i + 1);
	}
    }
}

unsigned int
TaggedNgramStats::countSentence(const VocabIndex *words)
{
    unsigned int start;

    for (start = 0; words[start] != Vocab_None; start++) {
        incrementTaggedCounts(words + start);
    }

    /*
     * keep track of word and sentence counts
     */
    stats.numWords += start;
    if (words[0] == vocab.ssIndex) {
	stats.numWords --;
    }
    if (start > 0 && words[start-1] == vocab.seIndex) {
	stats.numWords --;
    }

    stats.numSentences ++;

    return start;
}

