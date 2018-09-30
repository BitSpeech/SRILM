/*
 * RefList.cc --
 *	List of reference transcripts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998-2010 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/lm/src/RefList.cc,v 1.12 2010/06/02 05:49:58 stolcke Exp $";
#endif

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "RefList.h"

#include "LHash.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(RefString, VocabIndex *);
#endif

#include "Array.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_ARRAY(VocabIndex *);
#endif

/*
 * List of known filename suffixes that can be stripped to infer 
 * utterance ids.
 */
static const char *suffixes[] = {
    ".Z", ".gz", ".score", ".wav", ".wav_cep", ".wv", ".wv1", ".sph", ".lat", 0
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

RefList::RefList(Vocab &vocab, Boolean haveIDs)
    : vocab(vocab), haveIDs(haveIDs)
{
}

RefList::~RefList()
{
    for (unsigned i = 0; i < refarray.size(); i++) {
	delete [] refarray[i];
    }
}

Boolean
RefList::read(File &file, Boolean addWords)
{
    char *line;
    
    while ((line = file.getline())) {
	VocabString words[maxWordsPerLine + 2];
	unsigned nWords = Vocab::parseWords(line, words, maxWordsPerLine + 2);

	if (nWords == maxWordsPerLine + 2) {
	    file.position() << "too many words\n";
	    continue;
	}

	VocabIndex *wids = new VocabIndex[nWords + 2];
	assert(wids != 0);

	if (addWords) {
	    vocab.addWords(haveIDs ? words + 1 : words, wids, nWords + 2);
	} else {
	    vocab.getIndices(haveIDs ? words + 1: words, wids, nWords + 2,
							    vocab.unkIndex());
	}

	refarray[refarray.size()] = wids;

	if (haveIDs) {
	    VocabIndex **oldWids = reflist.insert((RefString)words[0]);
	    delete [] *oldWids;

	    *oldWids = wids;
	}
    }

    return true;
}

Boolean
RefList::write(File &file)
{
    if (haveIDs) {
	/* 
	 * Output sorted by ID
	 */
	LHashIter<RefString, VocabIndex *> iter(reflist, strcmp);

	RefString id;
	VocabIndex **wids;

	while ((wids = iter.next(id))) {
	    VocabString words[maxWordsPerLine + 1];
	    vocab.getWords(*wids, words, maxWordsPerLine + 1);

	    fprintf(file, "%s ", id);
	    Vocab::write(file, words);
	    fprintf(file, "\n");
	}
    } else {
	/*
	 * Output in read order
	 */
	for (unsigned i = 0; i < refarray.size(); i++) {
	    VocabString words[maxWordsPerLine + 1];
	    vocab.getWords(refarray[i], words, maxWordsPerLine + 1);

	    Vocab::write(file, words);
	    fprintf(file, "\n");
	}
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

VocabIndex *
RefList::findRefByNumber(unsigned id)
{
    if (id < refarray.size()) {
	return refarray[id];
    } else {
	return 0;
    }
}

