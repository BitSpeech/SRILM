/*
 * WordAlign.h --
 *	Word alignment and error computation
 *
 * Copyright (c) 1996,1997 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/WordAlign.h,v 1.4 2000/06/12 06:00:27 stolcke Exp $
 *
 */

#ifndef _WORD_ALIGN_H_
#define _WORD_ALIGN_H_

#include "Vocab.h"

/*
 * Error types
 */
typedef enum {
	CORR_ALIGN, SUB_ALIGN, DEL_ALIGN, INS_ALIGN, END_ALIGN
} WordAlignType;

/*
 * Costs for individual error types.  These are the conventional values
 * used in speech recognition word alignment.
 */
const unsigned SUB_COST = 4;
const unsigned DEL_COST = 3;
const unsigned INS_COST = 3;

unsigned wordError(const VocabIndex *hyp, const VocabIndex *ref,
			unsigned &sub, unsigned &ins, unsigned &del,
			WordAlignType *alignment = 0);
					/* computes total word error */

#endif /* _WORD_ALIGN_H_ */

