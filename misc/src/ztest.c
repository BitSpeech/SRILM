/*
 * ztest --
 *	test for zio.
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1997,2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/ztest.c,v 1.3 2008/12/21 23:19:52 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>

#include "zio.h"
#include "option.h"
#include "version.h"

char *inFile = "-";
char *outFile = "-";
int numLines = 0;
int version = 0;

static Option options[] = {
    { OPT_TRUE, "version", (void *)&version, "print version information" },
    { OPT_STRING, "read", (void *)&inFile, "input file" },
    { OPT_STRING, "write", (void *)&outFile, "output file" },
    { OPT_INT, "lines", (void *)&numLines, "number of lines to copy" },
};

int
main(int argc, char **argv)
{
    char buffer[1024];
    FILE *in, *out;
    int result;
    int lineno;

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (version) {
	printVersion(RcsId);
	exit(0);
    }

    in = zopen(inFile, "r");
    if (in == NULL) {
	perror(inFile);
	exit(1);
    }

    out = zopen(outFile, "w");
    if (out == NULL) {
	perror(outFile);
	exit(1);
    }

    lineno = 0;
    while ((numLines == 0 || lineno < numLines) &&
           fgets(buffer, sizeof(buffer), in))
    {
	fputs(buffer, out);
	lineno ++;
    }

    result = zclose(in);
    fprintf(stderr, "zclose(in) = %d\n", result);

    result = zclose(out);
    fprintf(stderr, "zclose(out) = %d\n", result);

    exit(0);
}

