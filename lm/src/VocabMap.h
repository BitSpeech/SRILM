/*
 * VocabMap.h --
 *	Probabilistic mappings between vocabularies
 *
 * Copyright (c) 1995,1998 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/VocabMap.h,v 1.4 2000/08/05 17:17:57 stolcke Exp $
 *
 */

#ifndef _VocabMap_h_
#define _VocabMap_h_

#include "Boolean.h"
#include "Prob.h"
#include "Vocab.h"
#include "Map2.h"

class VocabMap
{
    friend class VocabMapIter;

public:
    VocabMap(Vocab &v1, Vocab &v2, Boolean logmap = false);
    
    Prob get(VocabIndex w1, VocabIndex w2);
    void put(VocabIndex w1, VocabIndex w2, Prob prob);
    void remove(VocabIndex w1, VocabIndex w2);

    virtual Boolean read(File &file);
    virtual Boolean write(File &file);
    
    Vocab &vocab1;
    Vocab &vocab2;

private:
    /*
     * The map is implemented by a two-level map where the first index is
     * from vocab1 and the second from vocab2
     */
    Map2<VocabIndex,VocabIndex,Prob> map;

    Boolean logmap;			/* treat probabilities as log probs */
};

/*
 * Map from an integer (`position') to a Vocab
 */
class PosVocabMap: public VocabMap
{
public:
    PosVocabMap(Vocab &vocab, Boolean logmap = false)
	: posVocab(0),
	  /*
	   * We remove <s>, </s>, <unk> from the vocabulary so
           * the map doesn't create the default mappings for these.
	   */
	  VocabMap((posVocab.ssIndex = posVocab.seIndex =
		    posVocab.unkIndex = Vocab_None, posVocab),
		   vocab, logmap) {};

    /* 
     * not implemented yet (or ever)
     */
    Boolean read(File &file) { return false; };
    Boolean write(File &file) { return false; };

private:
    Vocab posVocab;
};

/*
 * Iteration over the mappings of a word
 */
class VocabMapIter
{
public:
    VocabMapIter(VocabMap &vmap, VocabIndex w);

    void init();
    Boolean next(VocabIndex &w, Prob &prob);

private:
    Map2Iter2<VocabIndex,VocabIndex,Prob> mapIter;
};

#endif /* _VocabMap_h_ */

