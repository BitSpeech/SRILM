/*
 * XCount.cc --
 *	Sparse integer counts stored in 2 bytes.
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,2005 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/XCount.cc,v 1.4 2005/09/30 17:54:44 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "XCount.h"

#include "Array.cc"

#define XCOUNT_XBIT		(1<<XCOUNT_MAXBITS)

Array<unsigned> XCount::xcountTable;
unsigned short XCount::freeList = XCOUNT_MAXINLINE;

unsigned short
XCount::getXCountTableIndex()
{
    static Boolean initialized = false;

    if (!initialized) {
    	// populate xcountTable free list
	for (unsigned short i = 0; i < XCOUNT_MAXINLINE; i++) {
	    freeXCountTableIndex(i);
	}

    	initialized = true;
    }

    Boolean xcountTableEmpty = (freeList == XCOUNT_MAXINLINE);
    assert(!xcountTableEmpty);

    unsigned short result = freeList;
    freeList = xcountTable[freeList];
    return result;
}

void 
XCount::freeXCountTableIndex(unsigned short idx)
{
    xcountTable[idx] = freeList;
    freeList = idx;
}

XCount::XCount(unsigned value)
    : indirect(false)
{
    if (value <= XCOUNT_MAXINLINE) {
    	indirect = false;
	count = value;
    } else {
	indirect = true;
	count = getXCountTableIndex();

	xcountTable[count] = value;
    }
}

XCount::~XCount()
{
    if (indirect) {
	freeXCountTableIndex(count);
    }
}

XCount::operator unsigned() const
{
    if (!indirect) {
	return count;
    } else {
	return xcountTable[count];
    }
}

XCount &
XCount::operator= (const XCount &other)
{
    if (&other != this) {
    	if (indirect) {
	    if (other.indirect) {
	    	xcountTable[count] = xcountTable[other.count];
	    } else {
		freeXCountTableIndex(count);
		indirect = false;
		count = other.count;
	    }
	} else {
	    if (other.indirect) {
		indirect = true;
		count = getXCountTableIndex();

		xcountTable[count] = xcountTable[other.count];
	    } else {
	    	count = other.count;
	    }
	}
    }
    return *this;
}

void
XCount::write(ostream &str) const
{
#ifdef DEBUG
    if (indirect) {
    	str << "X" << xcountTable[count]
	        << "[" << count << "]";
    } else {
    	str << "X" << count;
    }
#else
    str << (unsigned)*this;
#endif
}

ostream &operator<<(ostream &str, const XCount &count)
{
    count.write(str);
    return str;
}

