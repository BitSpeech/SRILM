/*
 * testLattice --
 *	Test for WordLattice class
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testLattice.cc,v 1.3 1999/08/01 09:22:47 stolcke Exp $";
#endif

#include <stdio.h>

#include "Vocab.h"
#include "WordLattice.h"

int
main (int argc, char *argv[])
{
    Vocab vocab;
    WordLattice lat(vocab);

    if (argc == 2) {
	File file(argv[1], "r");

	lat.read(file);
    }

    {
	char *line;
	File input(stdin);
	unsigned num = 0;

	while (line = input.getline()) {
	    VocabString sentence[maxWordsPerLine + 1];
	    VocabIndex words[maxWordsPerLine + 1];

	    (void)vocab.parseWords(line, sentence, maxWordsPerLine + 1);
	    vocab.addWords(sentence, words, maxWordsPerLine);

	    if (num++ == 0) {
		lat.addWords(words, 1.0);
	    } else {
		lat.alignWords(words, 1.0);
	    }
	}
    }
    
    {
	File file(stdout);
	lat.write(file);
    }

    {
	unsigned *sorted = new unsigned[lat.numNodes];
	unsigned reachable = lat.sortNodes(sorted);
	cerr << "sorted nodes: ";
	for (unsigned i = 0; i < reachable; i ++) {
	    cerr << " " << sorted[i];
	}
	cerr << endl;
	delete [] sorted;
    }

    exit(0);
}
