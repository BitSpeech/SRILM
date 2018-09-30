/*
 * ngram-merge --
 *	Merge sorted ngram counts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/ngram-merge.cc,v 1.10 2000/01/13 04:06:34 stolcke Exp $";
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

typedef double FloatCount;	// fractional count type
typedef union {
	NgramCount i;
	FloatCount f;
} MergeCount;			// integer or fractional count

static char *outName = "-";
static int floatCounts = 0;
static int optRest;

static Option options[] = {
    { OPT_TRUE, "float-counts", &floatCounts, "use fractional counts" },
    { OPT_STRING, "write", &outName, "counts file to write" },
    { OPT_REST, "-", &optRest, "indicate end of option list" },
    { OPT_DOC, 0, 0, "following options, specify 2 or more files to merge" },
};

typedef struct {
    VocabString ngram[maxNgramOrder + 1];
    unsigned int howmany;
    MergeCount count;
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
		floatCounts ?
		    NgramCounts<FloatCount>::readNgram(files[i],
			    perfile[i].ngram, maxNgramOrder + 1, 
			    perfile[i].count.f) : 
		    NgramCounts<NgramCount>::readNgram(files[i],
			    perfile[i].ngram, maxNgramOrder + 1, 
			    perfile[i].count.i);
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
findNext(unsigned int nfiles, PerFile *perfile, MergeCount &count)
{
    VocabString *minNgram = 0;
    MergeCount minCount;
    
    if (floatCounts) {
	minCount.f = 0.0;
    } else {
	minCount.i = 0;
    }

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
			
		if (floatCounts) {
		    minCount.f += perfile[i].count.f;
		} else {
		    minCount.i += perfile[i].count.i;
		}
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
    PerFile perfile[nfiles];

    /*
     * Get input from all files
     */
    for (int i = 0; i < nfiles; i++) {
	perfile[i].isNext = true;
    }

    while (getInput(nfiles, perfile, files)) {
	MergeCount count;
	VocabString *nextNgram = findNext(nfiles, perfile, count);

	if (floatCounts) {
	    NgramCounts<FloatCount>::writeNgram(out, nextNgram, count.f);
	} else {
	    NgramCounts<NgramCount>::writeNgram(out, nextNgram, count.i);
	}
    }
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    argc = Opt_Parse(argc, argv, options, Opt_Number(options),
							OPT_OPTIONS_FIRST);
    int i;

    if (argc < 3) {
	cerr << "need at least two input files\n";
	exit(1);
    }

    int nfiles = argc - 1;
    File files[nfiles];

    for (i = 0; i < nfiles; i++) {
	new (&files[i]) File(argv[i + 1], "r");
    }

    File outfile(outName, "w");
    merge_counts(nfiles, files, outfile);

    exit(0);
}

