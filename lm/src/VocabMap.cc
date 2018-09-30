/*
 * VocabMap.cc --
 *	Probabilistic mappings between vocabularies
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/VocabMap.cc,v 1.8 2000/08/05 17:17:57 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "VocabMap.h"

#include "Map2.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_MAP2(VocabIndex,VocabIndex,Prob);
#endif

VocabMap::VocabMap(Vocab &v1, Vocab &v2, Boolean logmap)
    : vocab1(v1), vocab2(v2), logmap(logmap)
{
    /*
     * Establish default mappings between special vocab items
     */
    if (v1.ssIndex != Vocab_None && v2.ssIndex != Vocab_None) {
	*map.insert(v1.ssIndex, v2.ssIndex) = logmap ? LogP_One : 1.0;
    }
    if (v1.seIndex != Vocab_None && v2.seIndex != Vocab_None) {
	*map.insert(v1.seIndex, v2.seIndex) = logmap ? LogP_One : 1.0;
    }
    if (v1.unkIndex != Vocab_None && v2.unkIndex != Vocab_None) {
	*map.insert(v1.unkIndex, v2.unkIndex) = logmap ? LogP_One : 1.0;
    }
}

Prob
VocabMap::get(VocabIndex w1, VocabIndex w2)
{
    Prob *prob = map.find(w1, w2);
    if (prob) {
	return *prob;
    } else {
	return 0.0;
    }
}

void
VocabMap::put(VocabIndex w1, VocabIndex w2, Prob prob)
{
    *map.insert(w1, w2) = prob;
}

void
VocabMap::remove(VocabIndex w1, VocabIndex w2)
{
    (void)map.remove(w1, w2);
}

Boolean
VocabMap::read(File &file)
{
    char *line;

    while (line = file.getline()) {
	VocabString words[maxWordsPerLine];

	unsigned howmany = Vocab::parseWords(line, words, maxWordsPerLine);

	if (howmany == maxWordsPerLine) {
	    file.position() << "map line has too many fields\n";
	    return false;
	}

	/*
	 * The first word is always the source of the map
	 */
	VocabIndex w1 = vocab1.addWord(words[0]);

	if (map.numEntries(w1) > 0) {
	    file.position() << "warning: map redefining entry "
			    << words[0] << endl;
	}

	/*
	 * Parse the remaining words as either probs or target words
	 */
	unsigned i = 1;

	while (i < howmany) {
	    double prob;

	    VocabIndex w2 = vocab2.addWord(words[i++]);

	    if (i < howmany && sscanf(words[i], "%lf", &prob)) {
		i ++;
	    } else {
		prob = logmap ? LogP_One : 1.0;
	    }

	    *(map.insert(w1, w2)) = prob;
	}
    }

    return true;
}

Boolean
VocabMap::write(File &file)
{
    Map2Iter<VocabIndex,VocabIndex,Prob> iter1(map);

    VocabIndex w1;

    while (iter1.next(w1)) {

	VocabString word1 = vocab1.getWord(w1);
	assert(word1 != 0);

	fprintf(file, "%s", word1);

	Map2Iter2<VocabIndex,VocabIndex,Prob> iter2(map, w1);

	VocabIndex w2;
	Prob *prob;

	unsigned i = 0;
	while (prob = iter2.next(w2)) {
	    VocabString word2 = vocab2.getWord(w2);
	    assert(word1 != 0);

	    char sep = (i++ == 0)  ? '\t' : ' ';

	    if (*prob == (logmap ? LogP_One : 1.0)) {
		fprintf(file, "%c%s", sep, word2);
	    } else {
		fprintf(file, "%c%s %lg", sep, word2, *prob);
	    }
	}
	fprintf(file, "\n");
    }
    return true;
}

/*
 * Iteration over map entries
 */

VocabMapIter::VocabMapIter(VocabMap &vmap, VocabIndex w) :
   mapIter(vmap.map, w)
{
}

void
VocabMapIter::init()
{
    mapIter.init();
}

Boolean
VocabMapIter::next(VocabIndex &w, Prob &prob)
{
    Prob *myprob = mapIter.next(w);

    if (myprob) {
	prob = *myprob;
	return true;
    } else {
	return false;
    }
}

