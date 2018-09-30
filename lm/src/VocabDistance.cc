/*
 * VocabDistance.cc --
 *	Distance metrics over vocabularies
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2000-2010 SRI International, 2012 Microsoft Corp.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/lm/src/VocabDistance.cc,v 1.5 2012/07/04 23:47:07 stolcke Exp $";
#endif

#include "VocabDistance.h"
#include "WordAlign.h"

#include "Map2.cc"
#ifdef INSTANTIATE_TEMPLATES
//INSTANTIATE_MAP2(VocabIndex,VocabIndex,double);
#endif

/*
 * Relative phonetic distance
 * (Levenshtein distance of pronunciations normalized by pronunciation length)
 */

DictionaryDistance::DictionaryDistance(Vocab &vocab, VocabMultiMap &dictionary)
    : dictionary(dictionary)
{
    /* 
     * Dictionary must be applicable to word vocabulary
     */
    assert(&vocab == &dictionary.vocab1);
}

double
DictionaryDistance::distance(VocabIndex w1, VocabIndex w2)
{
    if (w2 < w1) {
	VocabIndex h = w2;
	w2 = w1;
	w1 = h;
    } else if (w1 == w2) {
	return 0.0;
    }

    double *dist;
    Boolean foundP;
    dist = cache.insert(w1, w2, foundP);

    if (foundP) {
	return *dist;
    } else {
	double minDistance = -1.0;

	VocabMultiMapIter iter1(dictionary, w1);
	const VocabIndex *pron1;
	Prob p1;

	while ((pron1 = iter1.next(p1))) {
	    unsigned len1 = Vocab::length(pron1);

	    VocabMultiMapIter iter2(dictionary, w2);
	    const VocabIndex *pron2;
	    Prob p2;

	    while ((pron2 = iter2.next(p2))) {
		unsigned len2 = Vocab::length(pron2);

		unsigned maxLen = (len1 > len2) ? len1 : len2;

		unsigned sub, ins, del;
		double thisDistance =
			(double)wordError(pron1, pron2, sub, ins, del) /
					(maxLen > 0 ? maxLen : 1);

		if (minDistance < 0.0 || thisDistance < minDistance) {
		    minDistance = thisDistance;
		}
	    }
	}

	/*
	 * If no dictionary entries were found use 1 as default distance
	 */
	if (minDistance < 0.0) {
	    minDistance = 1.0;
	}

	*dist = minDistance;
	return minDistance;
    }
}

/*
 * Absolute phonetic distance
 */

const double defaultDistance = 1.0; /* for word without dictionary entries */
const char *emptyWord = "*EMPTY*WORD*";

DictionaryAbsDistance::DictionaryAbsDistance(Vocab &vocab,
						VocabMultiMap &dictionary)
    : dictionary(dictionary)
{
    /* 
     * Dictionary must be applicable to word vocabulary
     */
    assert(&vocab == &dictionary.vocab1);

    emptyIndex = vocab.addWord(emptyWord);
}

double
DictionaryAbsDistance::penalty(VocabIndex w)
{
    double *dist;
    Boolean foundP;
    dist = cache.insert(w, emptyIndex, foundP);

    if (foundP) {
	return *dist;
    } else {
	double minLength = -1.0;

	VocabMultiMapIter iter(dictionary, w);
	const VocabIndex *pron;
	Prob p;

	while ((pron = iter.next(p))) {
	    unsigned len = Vocab::length(pron);

	    if (minLength < 0.0 || len < minLength) {
		minLength = len;
	    }
	}

	/*
	 * If no dictionary entries were found use default distance
	 */
	if (minLength < 0.0) {
	    minLength = defaultDistance;
	}

	*dist = minLength;
	return minLength;
    }
}

double
DictionaryAbsDistance::distance(VocabIndex w1, VocabIndex w2)
{
    if (w2 < w1) {
	VocabIndex h = w2;
	w2 = w1;
	w1 = h;
    } else if (w1 == w2) {
	return 0.0;
    }

    double *dist;
    Boolean foundP;
    dist = cache.insert(w1, w2, foundP);

    if (foundP) {
	return *dist;
    } else {
	double minDistance = -1.0;

	VocabMultiMapIter iter1(dictionary, w1);
	const VocabIndex *pron1;
	Prob p1;

	while ((pron1 = iter1.next(p1))) {
	    VocabMultiMapIter iter2(dictionary, w2);
	    const VocabIndex *pron2;
	    Prob p2;

	    while ((pron2 = iter2.next(p2))) {
		unsigned sub, ins, del;
		double thisDistance =
			(double)wordError(pron1, pron2, sub, ins, del);

		if (minDistance < 0.0 || thisDistance < minDistance) {
		    minDistance = thisDistance;
		}
	    }
	}

	/*
	 * If no dictionary entries were found use default distance
	 */
	if (minDistance < 0.0) {
	    minDistance = defaultDistance;
	}

	*dist = minDistance;
	return minDistance;
    }
}

/*
 * Word distances defined by a matrix
 */

const VocabString deleteWord = "*DELETE*"; 

MatrixDistance::MatrixDistance(Vocab &vocab, VocabMap &map)
    : map(map)
{
    deleteIndex = vocab.addWord(deleteWord);
}

double
MatrixDistance::penalty(VocabIndex w1)
{
     return distance(w1, deleteIndex);
}

double
MatrixDistance::distance(VocabIndex w1, VocabIndex w2)
{
     Prob d = map.get(w1, w2);
     if (d == 0.0) {
	Prob d2 = map.get(w2, w1);
	if (d2 > 0.0) {
	    return d2;
	}
     }
     return d;
}

