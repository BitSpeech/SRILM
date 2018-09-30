/*
 * Test File class
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1998, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/testFile.cc,v 1.1 1998/07/10 07:35:47 stolcke Exp $";
#endif

#include "File.h"

main()
{
	File file(stdin);

	char *line;

	while (line = file.getline()) {
		file.position() << line;
	}

	exit(0);
}

