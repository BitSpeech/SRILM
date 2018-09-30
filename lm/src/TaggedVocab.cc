/*
 * TaggedVocab.cc --
 *	Tagged vocabulary implementation
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/speech/stolcke/project/srilm/devel/lm/src/RCS/TaggedVocab.cc,v 1.2 1996/05/31 05:25:44 stolcke Exp $";
#endif

#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "TaggedVocab.h"

const char tagSep = '/';		/* delimiter separating word from tag */

static char *
findTagSep(VocabString name)
{
    char *sep = strchr(name, tagSep);

    /*
     * Don't mistake '/' inside SGML tags as tag separators
     */
    if (sep > name && *(sep - 1) == '<') {
	return strchr(sep + 1, tagSep);
    } else {
	return sep;
    }
}

/*
 * Initialize the word Vocab making sure indices will fit in the lower-order
 * bits.  Let tag indices start at one, so 0 can be used for the untagged
 * case.
 */
TaggedVocab::TaggedVocab(VocabIndex start, VocabIndex end)
    : Vocab(start, end > maxTaggedIndex ? maxTaggedIndex : end),
      _tags(1, maxTagIndex)
{
    if (end > maxTaggedIndex) {
	cerr << "warning: maximum tagged index lowered to "
	     << maxTaggedIndex << endl;
    }
}

void
TaggedVocab::memStats(MemStats &stats)
{
    Vocab::memStats(stats);
    stats.total -= sizeof(_tags);
    _tags.memStats(stats);
}


VocabIndex
TaggedVocab::addWord(VocabString name)
{
    /*
     * Check if the string is a tagged word, and if so add both word and
     * tag string to the respective vocabs.
     */
    char *sep = findTagSep(name);

    if (sep) {
	char saved = *sep;
	*sep = '\0';

	/*
	 * empty words are allowed to denote tags by themselves
	 */
	VocabIndex wordId = (sep == name) ? Tagged_None : Vocab::addWord(name);
	VocabIndex tagId = _tags.addWord(sep + 1);

	*sep = saved;

	return tagWord(wordId, tagId);
    } else {
	return Vocab::addWord(name);
    }
}

VocabString
TaggedVocab::getWord(VocabIndex index)
{
    /*
     * Check if index contains tag, and if so construct a word/tag string
     * for it.
     */
    if (getTag(index) != Tag_None || unTag(index) == Tagged_None) {
	VocabString wordStr =
		isTag(index) ? "" : Vocab::getWord(unTag(index));
	VocabString tagStr =
		(getTag(index) == Tag_None) ? "" : _tags.getWord(getTag(index));

	if (wordStr == 0 || tagStr == 0) {
	    return 0;
	} else {
	    unsigned resultLen = strlen(wordStr) + 1 + strlen(tagStr) + 1;

	    /*
	     * XXX: Kludge alert -- We keep a large buffer of word/tag strings
	     * so that this function can be called often enough without
	     * interfering with previous result strings.
	     */
	    static char resultBuffer[100 * maxWordLength];
	    static char *nextResult = resultBuffer;

	    char *thisResult;
	    if (nextResult + resultLen >= resultBuffer + sizeof(resultBuffer)) {
		thisResult = resultBuffer;
		nextResult = resultBuffer + resultLen;
	    } else {
		thisResult = nextResult;
		nextResult += resultLen;
	    }

	    sprintf(thisResult, "%s%c%s", wordStr, tagSep, tagStr);
	    return thisResult;
	}
    } else {
	return Vocab::getWord(index);
    }
}

VocabIndex
TaggedVocab::getIndex(VocabString name, VocabIndex unkIndex)
{
    /*
     * Check if the string is a tagged word, and if so return a tagged index
     */
    char *sep = findTagSep(name);

    if (sep) {
	char saved = *sep;
	*sep = '\0';

	VocabIndex wordId =
		(sep == name) ? Tagged_None : Vocab::getIndex(name, Vocab_None);
	VocabIndex tagId = _tags.getIndex(sep + 1, Vocab_None);

	*sep = saved;

	if (wordId == Vocab_None || tagId == Vocab_None) {
	    return unkIndex;
	} else {
	    return tagWord(wordId, tagId);
	}
    } else {
	return Vocab::getIndex(name, unkIndex);
    }
}

void
TaggedVocab::remove(VocabString name)
{
    /*
     * Check if the string is a tagged word, and if so remove both word
     * and tag.
     */
    char *sep = findTagSep(name);

    if (sep) {
	char saved = *sep;
	*sep = '\0';

	if (sep != name) {
	    Vocab::remove(name);
	}
	_tags.remove(sep + 1);

	*sep = saved;
    } else {
	Vocab::remove(name);
    }
}

void
TaggedVocab::remove(VocabIndex index)
{
    /*
     * Check if index contains tag, and if so remove both word and tag.
     */
    if (getTag(index) != Tag_None) {
	if (!isTag(index)) {
	    Vocab::remove(unTag(index));
	}
	_tags.remove(getTag(index));
    } else {
	Vocab::remove(index);
    }
}

// Write vocabulary to file
void
TaggedVocab::write(File &file, Boolean sorted)
{
    Vocab::write(file, sorted);

    VocabIter iter(_tags, sorted);
    VocabString word;

    while (!file.error() && (word = iter.next())) {
	fprintf(file, "%c%s\n", tagSep, word);
    }
}

