/*
 * Test File class
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998-2010 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/misc/src/testFile.cc,v 1.4 2010/06/02 04:47:32 stolcke Exp $";
#endif

#include <stdlib.h>

#include "File.h"

int
main()
{
	File file(stdin);

	char *line;

	while ((line = file.getline())) {
		file.position() << line;
	}

	exit(0);
}

