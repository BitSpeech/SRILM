/*
 * VocabDistance.h --
 *	Distance metrics over vocabularies
 *
 * Copyright (c) 2000 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/VocabDistance.h,v 1.2 2000/08/06 03:20:18 stolcke Exp $
 *
 */

#ifndef _VocabDistance_h_
#define _VocabDistance_h_

#include "Vocab.h"
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

