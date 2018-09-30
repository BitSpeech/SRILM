/*
 * Test File class
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/testFile.cc,v 1.2 1999/10/13 09:16:47 stolcke Exp $";
#endif

#include "File.h"

int
main()
{
	File file(stdin);

	char *line;

	while (line = file.getline()) {
		file.position() << line;
	}

	exit(0);
}

