/*
 * SubVocab.cc --
 *	Vocabulary subset class
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1996,1999 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/SubVocab.cc,v 1.5 1999/10/12 23:06:44 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "SubVocab.h"

#include "LHash.h"
#include "Array.h"

SubVocab::SubVocab(Vocab &baseVocab)
    : _baseVocab(baseVocab)
{
    /*
     * These defaults are inherited from the base vocab.
     */
    outputVocab = &baseVocab;
    unkIsWord = baseVocab.unkIsWord;
    toLower = baseVocab.toLower;

    /*
     * sub-vocabularies don't have any special tokens by default
     */
    remove(unkIndex);
    remove(ssIndex);
    remove(seIndex);
    remove(pauseIndex);
}

// Add word to vocabulary
VocabIndex
SubVocab::addWord(VocabString name)
{
    /*
     * Try to find word in base vocabulary
     * If it doesn't exist there, add it first to the base vocabulary.
     * Then use the same index here.
     */
    VocabIndex wid = _baseVocab.addWord(name);

    if (wid == Vocab_None) {
	return Vocab_None;
    } else {
	return addWord(wid);
    }
}

// Add index to sub-vocabulary
VocabIndex
SubVocab::addWord(VocabIndex wid)
{
    /*
     * Index has to already exist in the base vocabulary.
     * If not, we fail.
     */
    VocabString baseName = _baseVocab.getWord(wid);

    if (baseName == 0) {
	return Vocab_None;
    } else {
	Boolean found;

	// use baseName here in case base Vocab changed capitalization
	VocabIndex *indexPtr = byName.insert(baseName, found);

	if (found) {
	    assert(*indexPtr == wid);
	} else {
	    *indexPtr = wid;
	    byIndex[wid] = byName.getInternalKey(baseName);

	    /*
	     * Initialize word strings between last highest and new index 
	     * (so that lookups return 0)
	     */
	    for (unsigned i = nextIndex; i < wid; i ++) {
		byIndex[i] = 0;
	    }
	    
	    /*
	     * nextIndex is 1 plus the highest word index used.
	     */
	    if (wid + 1 > nextIndex) {
		nextIndex = wid + 1;
	    }
	} 
	return wid;
    }
}

