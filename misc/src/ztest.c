/*
 * ztest --
 *	test for zio.
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1997, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/ztest.c,v 1.1 1997/01/23 20:05:17 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>

#include "zio.h"
#include "option.h"

char *inFile = "-";
char *outFile = "-";
int numLines = 0;

static Option options[] = {
    { OPT_STRING, "read", &inFile, "input file" },
    { OPT_STRING, "write", &outFile, "output file" },
    { OPT_INT, "lines", &numLines, "number of lines to copy" },
};

int
main(int argc, char **argv)
{
    char buffer[1024];
    FILE *in, *out;
    int result;
    int lineno;

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

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

