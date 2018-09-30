/*
 * VocabDistance.h --
 *	Distance metrics over vocabularies
 *
 * Copyright (c) 2000,2004 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/VocabDistance.h,v 1.3 2004/08/03 23:21:31 stolcke Exp $
 *
 */

#ifndef _VocabDistance_h_
#define _VocabDistance_h_

#include "Vocab.h"
#include "SubVocab.h"
#include "VocabMultiMap.h"
#include "Map2.h"

class VocabDistance
{
public:
    virtual double penalty(VocabIndex w1) = 0;	/* insertion/deletion penalty */
    virtual double distance(VocabIndex w1, VocabIndex w2) = 0; /* substitution*/
};

/*
 * Binary distance
 */
class IdentityDistance: public VocabDistance
{
public:
    IdentityDistance(Vocab &vocab) {};
    double penalty(VocabIndex w1) { return 1.0; };
    double distance(VocabIndex w1, VocabIndex w2) 
	{ return (w1 == w2) ? 0.0 : 1.0; };
};

/*
 * Distance constrained by sub-vocabulary membership
 * Return distance
 *	0 if words are identical,
 *	1 if they both belong or don't belong to SubVocab,
 *	and infinity otherwise.
 */
class SubVocabDistance: public VocabDistance
{
public:
    SubVocabDistance(Vocab &vocab, SubVocab &subVocab, double infinity = 10000)
       : subVocab(subVocab), infinity(infinity) {};

    double penalty(VocabIndex w1) { return 1.0; };
    double distance(VocabIndex w1, VocabIndex w2) {
	if (w1 == w2) return 0.0;
	else if ((subVocab.getWord(w1) != 0) == (subVocab.getWord(w2) != 0))
	    return 1.0;
	else return infinity;
    }
private:
    SubVocab &subVocab;
    double infinity;
};

/*
 * Relative phonetic distance according to dictionary
 */
class DictionaryDistance: public VocabDistance
{
public:
    DictionaryDistance(Vocab &vocab, VocabMultiMap &dictionary);

    double penalty(VocabIndex w1) { return 1.0; };
    double distance(VocabIndex w1, VocabIndex w2);

private:
    VocabMultiMap &dictionary;
    Map2<VocabIndex,VocabIndex,double> cache;
};

/*
 * Absolute phonetic distance
 */
class DictionaryAbsDistance: public VocabDistance
{
public:
    DictionaryAbsDistance(Vocab &vocab, VocabMultiMap &dictionary);

    double penalty(VocabIndex w1);
    double distance(VocabIndex w1, VocabIndex w2);

private:
    VocabIndex emptyIndex;
    VocabMultiMap &dictionary;
    Map2<VocabIndex,VocabIndex,double> cache;
};

#endif /* _VocabDistance_h_ */

