/*
 * testNgram --
 *	Rudimentary test for Ngram LM class
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2005, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testNgram.cc,v 1.1 2005/07/17 22:54:13 stolcke Exp $";
#endif

#include <stdio.h>

#include "Ngram.h"

int
main (int argc, char *argv[])
{
    Vocab vocab;
    Ngram ng(vocab, 3);

    if (argc == 2) {
	File file(argv[1], "r");

	ng.read(file);
    }

    cerr << "*** Ngram LM ***\n";
    {
	File file(stderr);

	ng.write(file);
    }

    Ngram ng_copy(ng);

    ng.clear();

    cerr << "*** Ngram LM (copy) ***\n";
    {
	File file(stderr);

	ng_copy.write(file);
    }

    cerr << "*** Ngram LM (cleared) ***\n";
    {
	File file(stderr);

	ng.write(file);
    }

    exit(0);
}
