/*
 * WordAlign.h --
 *	Word alignment and error computation
 *
 * Copyright (c) 1996,1997 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/spot71/srilm/devel/lm/src/RCS/WordAlign.h,v 1.2 1997/07/15 02:42:26 stolcke Exp $
 *
 */

#ifndef _WORD_ALIGN_H_
#define _WORD_ALIGN_H_

#include "Vocab.h"

/*
 * Costs for individual error types.  These are the conventional values
 * used in speech recognition word alignment.
 */
const unsigned SUB_COST = 4;
const unsigned DEL_COST = 3;
const unsigned INS_COST = 3;

unsigned wordError(const VocabIndex *hyp, const VocabIndex *ref,
			unsigned &sub, unsigned &ins, unsigned &del);
					/* computes total word error */

#endif /* _WORD_ALIGN_H_ */

