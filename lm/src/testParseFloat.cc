/*
 * testParseFloat --
 *	Benchmark floating point parsing
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testParseFloat.cc,v 1.4 1999/08/01 09:23:32 stolcke Exp $";
#endif

#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <string.h>

#include "Boolean.h"
#include "Prob.h"


inline Boolean
oldParseLogP(const char *string, LogP &prob) {
    return (sscanf(string, "%f", &prob) == 1) ||
	   ((strncmp(string, "-Inf", 4) == 0 ||
	     strncmp(string, "-inf", 4) == 0) && (prob = LogP_Zero, true)) ||
	   ((strncmp(string, "Inf", 3) == 0 ||
	     strncmp(string, "inf", 3) == 0) && (prob = LogP_Inf, true));
}

int
main(int argc, char **argv)
{
    if (argc < 4) {
	cerr << "usage: " << argv[0] << " new? float repeats\n";
	exit(2);
    }

    int newp = atoi(argv[1]);
    const char *floatString = argv[2];
    unsigned repeats = atoi(argv[3]);

    LogP x;
    Boolean result = parseLogP(floatString, x);
    cout << "result = " << result 
	 << " float = " << x << endl;

    if (newp) {
	for (unsigned i = 0; i < repeats; i ++) {
    		parseLogP(floatString, x);
	}
    } else {
	for (unsigned i = 0; i < repeats; i ++) {
    		oldParseLogP(floatString, x);
	}
    }

    exit(0);
}
