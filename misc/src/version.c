/*
 * version.c --
 *	Print version information
 * 
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2004 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/version.c,v 1.2 2008/12/21 23:10:21 stolcke Exp $";
#endif

#include <stdio.h>

#include "zio.h"
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
#ifndef NO_ZIO
	printf("\nSupport for compressed files is included.\n");
#endif
 	puts(SRILM_COPYRIGHT);
}

