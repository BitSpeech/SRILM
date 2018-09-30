/*
 * SubVocab.h --
 *	Vocabulary subset class
 *
 * Copyright (c) 1996,1999,2003 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/SubVocab.h,v 1.4 2003/10/10 01:23:39 stolcke Exp $
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

    // parameters tied to the base vocabulary 
    virtual Boolean &unkIsWord() { return _baseVocab.unkIsWord(); };
    virtual Boolean &toLower() { return _baseVocab.toLower(); };
    virtual VocabString &metaTag() { return _baseVocab.metaTag(); };

    inline Vocab &baseVocab() { return _baseVocab; };

protected:
    Vocab &_baseVocab;
};

#endif /* _SubVocab_h_ */
