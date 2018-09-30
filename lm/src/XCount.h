/*
 * XCount.h --
 *	Extended counts.
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /export/d/stolcke/project/srilm/src/RCS/XCount.h,v 1.1 1995/06/22 07:47:55 stolcke Exp $
 *
 */

#ifndef _XCOUNT_H_
#define _XCOUNT_H_

#include "Boolean.h"

#define XCOUNT_MAXBITS		(15)
#define XCOUNT_MAXINLINE	((1 << XCOUNT_MAXBITS)-1)

/*
 * We'd really like to declare XCount as a structure with bitfields,
 * but then the compiler will align them on word boundaries and we 
 * won't see a size advantage over fill int's.

typedef struct {
	Boolean extended:1;
	unsigned int count:XCOUNT_MAXBITS;
} XCount;

 */
typedef unsigned short XCount;

unsigned int XCount_Get(XCount xcount);
unsigned int XCount_Set(XCount &xcount, unsigned int value);

#endif /* _XCOUNT_H_ */

