/*
 * XCount.cc --
 *	Sparse integer counts stored in 2 bytes.
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/XCount.cc,v 1.7 2006/07/31 17:36:55 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "XCount.h"

XCountValue XCount::xcountTable[XCount_TableSize];
unsigned XCount::refCounts[XCount_TableSize];
XCountIndex XCount::freeList = XCount_Maxinline;

XCountIndex
XCount::getXCountTableIndex()
{
    static Boolean initialized = false;

    if (!initialized) {
    	// populate xcountTable free list
	for (XCountIndex i = 0; i < XCount_TableSize; i++) {
	    xcountTable[i] = freeList;
	    freeList = i;
	}

    	initialized = true;
    }

    Boolean xcountTableEmpty = (freeList == XCount_Maxinline);
    assert(!xcountTableEmpty);

    XCountIndex result = freeList;
    freeList = xcountTable[freeList];

    refCounts[result] = 1;
    return result;
}

void 
XCount::freeXCountTableIndex(XCountIndex idx)
{
    refCounts[idx] --;
    if (refCounts[idx] == 0) {
	xcountTable[idx] = freeList;
	freeList = idx;
    }
}

XCount::XCount(XCountValue value)
    : indirect(false)
{
    if (value <= XCount_Maxinline) {
    	indirect = false;
	count = value;
    } else {
	indirect = true;
	count = getXCountTableIndex();

	xcountTable[count] = value;
    }
}

XCount::XCount(const XCount &other)
{
    indirect = other.indirect;
    count = other.count;
    if (indirect) {
	refCounts[count] ++;
    }
}

XCount::~XCount()
{
    if (indirect) {
	freeXCountTableIndex(count);
    }
}

XCount::operator XCountValue() const
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
	    freeXCountTableIndex(count);
	}

	count = other.count;
	indirect = other.indirect;

	if (other.indirect) {
	    refCounts[other.count] ++;
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
    str << (XCountValue)*this;
#endif
}

ostream &operator<<(ostream &str, const XCount &count)
{
    count.write(str);
    return str;
}

