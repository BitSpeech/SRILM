/*
 * DecipherNgram.h --
 *	Approximate N-gram backoff language model used in Decipher recognizer
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/DecipherNgram.h,v 1.3 1997/11/03 21:40:54 stolcke Exp $
 *
 */

#ifndef _DecipherNgram_h_
#define _DecipherNgram_h_

#include <stdio.h>

#include "Ngram.h"

class DecipherNgram: public Ngram
{
public:
    DecipherNgram(Vocab &vocab, unsigned int order = 2);

    Boolean backoffHack;	/* take backoff path if higher scoring */

protected:
    virtual LogP wordProbBO(VocabIndex word, const VocabIndex *context,
							unsigned int clen);
};

#endif /* _DecipherNgram_h_ */
