/*
 * SubVocab.h --
 *	Vocabulary subset class
 *
 * Copyright (c) 1996,1999 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/SubVocab.h,v 1.3 1999/10/12 23:06:44 stolcke Exp $
 *
 */

#ifndef _SubVocab_h_
#define _SubVocab_h_

#include "Vocab.h"

/*
 * SubVocab is a version of Vocab that only contains words also in a base
 * vocabulary.  The indices used by the SubVocab are the same as those in the
 * base vocabulary.
 */
class SubVocab: public Vocab
{
public:
    SubVocab(Vocab &baseVocab);
    ~SubVocab() { };			/* works around g++ 2.7.2 bug */

    virtual VocabIndex addWord(VocabString name);
    virtual VocabIndex addWord(VocabIndex wid);

    inline Vocab &baseVocab() { return _baseVocab; };

protected:
    Vocab &_baseVocab;
};

#endif /* _SubVocab_h_ */
