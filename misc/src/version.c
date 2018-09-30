/*
 * version.c --
 *	Print version information
 * 
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2004 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/version.c,v 1.1 2004/12/03 04:24:36 stolcke Exp $";
#endif

#include <stdio.h>

#include "version.h"
#include "SRILMversion.h"

void
printVersion(const char *rcsid)
{
	printf("SRILM release %s", SRILM_RELEASE);
#ifndef EXCLUDE_CONTRIB
	printf(" (with third-party contributions)");
#endif /* EXCLUDE_CONTRIB_END */
	printf("\n\nProgram version %s\n", rcsid);
 	puts(SRILM_COPYRIGHT);
}

