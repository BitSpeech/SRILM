/*
 * matherr.c --
 *	Math error handling
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1996 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/matherr.c,v 1.1 1998/01/18 03:51:12 stolcke Exp $";
#endif

#include <math.h>

int
matherr(struct exception *x)
{
    if (x->type == SING && strcmp(x->name, "log10") == 0) {
	/*
	 * suppress warnings about log10(0.0)
	 */
	return 1;
    } else {
	return 0;
    }
}

