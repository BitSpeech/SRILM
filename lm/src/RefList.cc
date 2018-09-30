/*
 * RefList.cc --
 *	List of reference transcripts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998, 2002 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/RefList.cc,v 1.5 2002/11/01 21:04:59 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "RefList.h"

#include "LHash.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(RefString, VocabIndex *);
#endif

/*
 * List of known filename suffixes that can be stripped to infer 
 * utterance ids.
 */
static char *suffixes[] = {
	".Z", ".gz", ".score", ".wav", ".wav_cep", ".wv", ".wv1", ".sph", 0
};

/*
 * Locate utterance id in filename
 *	Result is returned in temporary buffer that is valid until next
 *	call to this function.
 */
RefString
idFromFilename(const char *filename)
{
    static char *result = 0;

    const char *root = strrchr(filename, '/');

    if (root) {
	root += 1;
    } else {
	root = filename;
    }

    if (result) free(result);
    result = strdup(root);
    assert(result != 0);

    unsigned rootlen = strlen(result);

    for (unsigned i = 0; suffixes[i] != 0; i++) {
	unsigned suffixlen = strlen(suffixes[i]);

	if (suffixlen < rootlen &&
	    strcmp(&result[rootlen - suffixlen], suffixes[i]) == 0)
	{
	    result[rootlen - suffixlen] = '\0';
	    rootlen -= suffixlen;
	}
    }
    return result;
}

RefList::RefList(Vocab &vocab)
    : vocab(vocab)
{
}

RefList::~RefList()
{
    LHashIter<RefString, VocabIndex *> iter(reflist);

    RefString id;
    VocabIndex **words;
    while (words = iter.next(id)) {
	delete [] *words;
    }
}

Boolean
RefList::read(File &file, Boolean addWords)
{
    char *line;
    
    while (line = file.getline()) {
	VocabString words[maxWordsPerLine + 2];
	unsigned nWords = Vocab::parseWords(line, words, maxWordsPerLine + 2);

	if (nWords == maxWordsPerLine + 2) {
	    file.position() << "too many words\n";
	    continue;
	}

	VocabIndex *wids = new VocabIndex[nWords + 1];
	assert(wids != 0);

	if (addWords) {
	    vocab.addWords(words + 1, wids, nWords + 1);
	} else {
	    vocab.getIndices(words + 1, wids, nWords + 1, vocab.unkIndex);
	}

	VocabIndex **oldWids = reflist.insert((RefString)words[0]);
	delete [] *oldWids;

	*oldWids = wids;
    }

    return true;
}

Boolean
RefList::write(File &file)
{
    LHashIter<RefString, VocabIndex *> iter(reflist, strcmp);

    RefString id;
    VocabIndex **wids;

    while (wids = iter.next(id)) {
	VocabString words[maxWordsPerLine + 1];
	vocab.getWords(*wids, words, maxWordsPerLine + 1);

	fprintf(file, "%s ", id);
	Vocab::write(file, words);
	fprintf(file, "\n");
    }

    return true;
}

VocabIndex *
RefList::findRef(RefString id)
{
    VocabIndex **wids = reflist.find(id);

    if (wids) {
	return *wids;
    } else {
	return 0;
    }
}

