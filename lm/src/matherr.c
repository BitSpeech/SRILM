/*
 * matherr.c --
 *	Math error handling
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1996-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/matherr.c,v 1.4 2006/01/11 06:05:06 stolcke Exp $";
#endif

#include <math.h>
#include <string.h>

int
#if defined(__MINGW32_VERSION) || defined(_MSC_VER)
_matherr(struct _exception *x)
#else
matherr(struct exception *x)
#endif
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

