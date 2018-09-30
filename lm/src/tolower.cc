/*
 * tolower --
 *	Map input to lowercase.
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/speech/stolcke/project/srilm/devel/lm/src/RCS/tolower.cc,v 1.2 1996/05/31 05:25:44 stolcke Exp $";
#endif

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

main()
{

    //setlocale(LC_CTYPE, "iso_8859_1");
    setlocale(LC_CTYPE, "");

    char line[1000];

    while (fgets(line, sizeof(line), stdin)) {
	unsigned i;
	for (i = 0; i < sizeof(line) && line[i] != 0; i ++) {
	    line[i] = tolower(line[i]);	    
	}
	fputs(line, stdout);
    }
    exit(0);
}

