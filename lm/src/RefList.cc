/*
 * RefList.cc --
 *	List of reference transcripts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/RefList.cc,v 1.3 2000/03/31 09:10:24 stolcke Exp $";
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

