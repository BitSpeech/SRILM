/*
 * NBestSet.cc --
 *	Set of N-best lists
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/NBestSet.cc,v 1.4 2001/04/13 03:54:35 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "NBestSet.h"
#include "NullLM.h"

#include "LHash.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(RefString, NBestSetElement);
#endif

/*
 * Debugging levels used
 */
#define DEBUG_READ	1

NBestSet::NBestSet(Vocab &vocab, RefList &refs,
		    unsigned maxNbest, Boolean incremental, Boolean multiwords)
    : vocab(vocab), refs(refs), maxNbest(maxNbest), incremental(incremental),
      multiwords(multiwords), warn(true)
{
}

NBestSet::~NBestSet()
{
    LHashIter<RefString, NBestSetElement> iter(lists);

    RefString id;
    NBestSetElement *elt;
    while (elt = iter.next(id)) {
	delete [] elt->filename;
	delete elt->nbest;
    }
}

Boolean
NBestSet::read(File &file)
{
    char *line;
    
    while (line = file.getline()) {
	VocabString filename[2];

	/*
	 * parse filename from input file
	 */
	if (Vocab::parseWords(line, filename, 2) != 1) {
	    file.position() << "one filename expected\n";
	    return false;
	}
	
	char *filenameCopy = new char[strlen(filename[0]) + 1];
	assert(filenameCopy != 0);
	strcpy(filenameCopy, filename[0]);

	/*
	 * Locate utterance id in filename
	 */
	const char *id = strrchr(filename[0], '/');
	if (id) {
	    id += 1;
	} else {
	    id = filename[0];
	}

	char *suffix = strchr(id, '.');
	if (suffix) {
	    *suffix = '\0';
	}

	NBestSetElement *elt = lists.insert(id);

	delete [] elt->filename;
	elt->filename = filenameCopy;

	delete elt->nbest;
	if (incremental) {
	    elt->nbest = 0;
	} else if (!readList(id, *elt)) {
	    file.position() << "error reading N-best list\n";
	    elt->nbest = 0;
	}
    }

    return true;
}

Boolean
NBestSet::readList(RefString id, NBestSetElement &elt)
{
    elt.nbest = new NBestList(vocab, maxNbest, multiwords);
    assert(elt.nbest != 0);

    /*
     * Read Nbest list
     */
    File nbestFile(elt.filename, "r");
    if (!elt.nbest->read(nbestFile)) {
	return false;
    }

    if (debug(DEBUG_READ)) {
	dout() << id << ": " << elt.nbest->numHyps() << " hyps\n";
    }

    /*
     * Remove pause and noise tokens
     */
    NullLM nullLM(vocab);
    elt.nbest->removeNoise(nullLM);

    /*
     * Compute word errors
     */
    VocabIndex *ref = refs.findRef(id);

    if (!ref) {
	if (warn) {
	    cerr << "No reference found for " << id << endl;
	}
    } else {
	unsigned subs, inss, dels;
	elt.nbest->wordError(ref, subs, inss, dels);
    }

    return true;
}

void
NBestSet::freeList(NBestSetElement &elt)
{
    delete elt.nbest;
    elt.nbest = 0;
}

/*
 * Iteration over N-best sets
 */

NBestSetIter::NBestSetIter(NBestSet &set)
    : mySet(set), myIter(set.lists, strcmp), lastElt(0)
{
}

void
NBestSetIter::init()
{
    myIter.init();
    lastElt = 0;
}

NBestList *
NBestSetIter::next(RefString &id)
{
    if (lastElt) {
	mySet.freeList(*lastElt);
	lastElt = 0;
    }
	
    NBestSetElement *nextElt = myIter.next(id);
    if (nextElt) {
	if (mySet.incremental) {
	    if (!mySet.readList(id, *nextElt)) {
	        cerr << "error reading N-best list" << id << endl;
		return 0;
	    } else {
		lastElt = nextElt;
	    }
	}
	return nextElt->nbest;
    } else {
	return 0;
    }
}

