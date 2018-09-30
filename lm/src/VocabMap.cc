/*
 * VocabMap.cc --
 *	Probabilistic mappings between vocabularies
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/VocabMap.cc,v 1.7 1998/12/27 19:30:04 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "VocabMap.h"

#include "Trie.cc"
#ifdef INSTANTIATE_TEMPLATES
// XXX: Avoid duplicate Instantiation with NgramStatsFloat.cc
//INSTANTIATE_TRIE(VocabIndex,Prob);
#endif

VocabMap::VocabMap(Vocab &v1, Vocab &v2, Boolean logmap)
    : vocab1(v1), vocab2(v2), logmap(logmap)
{
    VocabIndex keys[3];
    keys[2] = Vocab_None;

    /*
     * Establish default mappings between special vocab items
     */
    if (v1.ssIndex != Vocab_None && v2.ssIndex != Vocab_None) {
	keys[0] = v1.ssIndex;
	keys[1] = v2.ssIndex;
	*map.insert(keys) = logmap ? LogP_One : 1.0;
    }
    if (v1.seIndex != Vocab_None && v2.seIndex != Vocab_None) {
	keys[0] = v1.seIndex;
	keys[1] = v2.seIndex;
	*map.insert(keys) = logmap ? LogP_One : 1.0;
    }
    if (v1.unkIndex != Vocab_None && v2.unkIndex != Vocab_None) {
	keys[0] = v1.unkIndex;
	keys[1] = v2.unkIndex;
	*map.insert(keys) = logmap ? LogP_One : 1.0;
    }
}

Prob
VocabMap::get(VocabIndex w1, VocabIndex w2)
{
    VocabIndex keys[3];
    keys[0] = w1;
    keys[1] = w2;
    keys[2] = Vocab_None;
    
    Prob *prob = map.find(keys);
    if (prob) {
	return *prob;
    } else {
	return 0.0;
    }
}

void
VocabMap::put(VocabIndex w1, VocabIndex w2, Prob prob)
{
    VocabIndex keys[3];
    keys[0] = w1;
    keys[1] = w2;
    keys[2] = Vocab_None;
    
    *map.insert(keys) = prob;
}

void
VocabMap::remove(VocabIndex w1, VocabIndex w2)
{
    VocabIndex keys[3];
    keys[0] = w1;
    keys[1] = w2;
    keys[2] = Vocab_None;
    
    (void)map.remove(keys);
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

	Boolean found;
	VocabMapTrie *node1 = map.insertTrie(w1, found);

	if (found) {
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

	    *(node1->insert(w2)) = prob;
	}
    }

    return true;
}

Boolean
VocabMap::write(File &file)
{
    TrieIter<VocabIndex,Prob> iter1(map);

    VocabIndex w1;
    VocabMapTrie *node1;

    while (node1 = iter1.next(w1)) {

	VocabString word1 = vocab1.getWord(w1);
	assert(word1 != 0);

	fprintf(file, "%s", word1);

	TrieIter<VocabIndex,Prob> iter2(*node1);

	VocabIndex w2;
	VocabMapTrie *node2;

	unsigned i = 0;
	while (node2 = iter2.next(w2)) {
	    VocabString word2 = vocab2.getWord(w2);
	    assert(word1 != 0);

	    Prob prob = node2->value();

	    char sep = (i++ == 0)  ? '\t' : ' ';

	    if (prob == (logmap ? LogP_One : 1.0)) {
		fprintf(file, "%c%s", sep, word2);
	    } else {
		fprintf(file, "%c%s %lg", sep, word2, prob);
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
   mapNode(vmap.map.findTrie(w)),
   mapIter(mapNode ? *mapNode : vmap.map)
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
    if (!mapNode) {
	return false;
    } else {
	VocabMapTrie *nextNode = mapIter.next(w);
	if (nextNode) {
	    prob = nextNode->value();
	    return true;
	} else {
	    return false;
	}
    }
}

