/*
 * XCount.cc --
 *	Extended counts implementation
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/speech/stolcke/project/srilm/devel/lm/src/RCS/XCount.cc,v 1.2 1996/05/31 05:25:44 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "XCount.h"

#define XCOUNT_STARTSIZE 100
#define XCOUNT_GROWSIZE	 2

static unsigned int *xcountTable = 0;
static unsigned int maxEntries = 0;
static unsigned int numEntries = 0;

#define XCOUNT_XBIT		(1<<XCOUNT_MAXBITS)

unsigned int
XCount_Get(XCount xcount)
{
    if (!(xcount & XCOUNT_XBIT)) {
	return xcount;
    } else {
	return xcountTable[xcount & (~XCOUNT_XBIT)];
    }
}

unsigned int
XCountNew()
{
    assert(numEntries < XCOUNT_MAXINLINE);

    if (numEntries == maxEntries) {
	if (maxEntries == 0) {
	    maxEntries = XCOUNT_STARTSIZE;
	    xcountTable = (unsigned int *)malloc(sizeof(unsigned int) *
							maxEntries);
	} else {
	    maxEntries = (unsigned int)(maxEntries * XCOUNT_GROWSIZE);
	    xcountTable = (unsigned int *)
			    realloc(xcountTable, sizeof(unsigned int) *
							maxEntries);
	}

	assert(xcountTable != 0);
    }
    return numEntries++ ;
}

/*
 * Store a new value, regardless of previous contents
 */
unsigned int
XCount_Set(XCount &xcount, unsigned int value)
{
    if (value <= XCOUNT_MAXINLINE) {
	xcount = value;
    } else {
	unsigned int index = XCountNew();

	xcountTable[index] = value;
	xcount = XCOUNT_XBIT | value;
    }
    return value;
}

/*
 * Store a new value, replacing previous one.
 */
unsigned int
XCount_Reset(XCount &xcount, unsigned int value)
{
    if (value <= XCOUNT_MAXINLINE) {
	/*
	 * XXX: we lose the space if an extended count was stored here
	 * perviously.
	 */
	xcount = value;
    } else {
	/*
	 * Reuse old entry if possible
	 */
	if (xcount & XCOUNT_XBIT) {
	    xcountTable[xcount & (~XCOUNT_XBIT)] = value;
	} else {
	    unsigned int index = XCountNew();

	    xcountTable[index] = value;
	    xcount = XCOUNT_XBIT | value;
	}
    }
    return value;
}

