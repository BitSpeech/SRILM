/*
 * ngram-merge --
 *	Merge sorted ngram counts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/ngram-merge.cc,v 1.8 1999/08/01 09:33:14 stolcke Exp $";
#endif

#include <stdlib.h>
#include <iostream.h>
#include <locale.h>
#include <assert.h>
#include <new.h>

#include "option.h"

#include "File.h"
#include "Vocab.h"
#include "NgramStats.h"

static char *outName = "-";

static Option options[] = {
    { OPT_STRING, "write", &outName, "counts file to write" },
    { OPT_DOC, 0, 0, "following option, specify 2 or more files to merge" }
};

typedef struct {
    VocabString ngram[maxNgramOrder + 1];
    unsigned int howmany;
    NgramCount count;
    Boolean isNext;
} PerFile;

// Get a new line from all files that are 'next'.
// Return true if at least one of the files got more data.
Boolean
getInput(unsigned int nfiles, PerFile *perfile, File *files)
{
    Boolean haveInput = false;

    for (unsigned int i = 0; i < nfiles; i++) {
	if (perfile[i].isNext) {
	    perfile[i].isNext = false;

	    perfile[i].howmany = 
		NgramStats::readNgram(files[i],
			perfile[i].ngram, maxNgramOrder + 1, 
			perfile[i].count);
	}
	if (perfile[i].howmany > 0) {
	    haveInput = true;
	}
    }
    return haveInput;
}

// Go through all current entries and mark those that compare equal
// and come before all others as 'next'.
// Return the next (minimal) Ngram, and its aggregate count.
VocabString *
findNext(unsigned int nfiles, PerFile *perfile, NgramCount &count)
{
    VocabString *minNgram = 0;
    NgramCount minCount = 0;

    for (unsigned int i = 0; i < nfiles; i++) {
	if (perfile[i].howmany == 0) {
	    /*
	     * Data in this file is exhausted -- skip it.
	     */
	    continue;
	} else if (minNgram == 0) {
	    /*
	     * The first non-empty file -- it is our first candidate
	     * for next ngram.
	     */
	    minNgram = perfile[i].ngram;
	    perfile[i].isNext = true;
	    minCount = perfile[i].count;
	} else {
	    int comp = Vocab::compare(minNgram, perfile[i].ngram);

	    if (comp == 0) {
		/*
		 * Another file that shares the minimal ngram
		 */
		perfile[i].isNext = true;
	        minCount += perfile[i].count;
	    } else if (comp > 0) {
		/*
		 * A new minimal ngram.  Invalidate all the previous
		 * 'next' candidates.
		 */
	        minNgram = perfile[i].ngram;
		perfile[i].isNext = true;
		for (unsigned j = 0; j < i; j++) {
		   perfile[j].isNext = false;
		}
	        minCount = perfile[i].count;
	    }
	}
    }
    count = minCount;
    return minNgram;
}

static
void
merge_counts(unsigned int nfiles, File *files, File &out)
{
    PerFile *perfile = new PerFile[nfiles];
    assert(perfile != 0);

    /*
     * Get input from all files
     */
    for (int i = 0; i < nfiles; i++) {
	perfile[i].isNext = true;
    }

    while (getInput(nfiles, perfile, files)) {
	NgramCount count;
	VocabString *nextNgram = findNext(nfiles, perfile, count);

	NgramStats::writeNgram(out, nextNgram, count);
    }

    delete [] perfile;
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    argc = Opt_Parse(argc, argv, options, Opt_Number(options), 0);
    int i;

    if (argc < 3) {
	cerr << "need at least two input files\n";
	exit(1);
    }

    int nfiles = argc - 1;
    File *files = new File[nfiles];
    assert(files != 0);

    for (i = 0; i < nfiles; i++) {
	new (&files[i]) File(argv[i + 1], "r");
    }

    File outfile(outName, "w");
    merge_counts(nfiles, files, outfile);

    delete [] files;

    exit(0);
}

