/*
 * MemStats.cc --
 *	Memory statistics.
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/dstruct/src/RCS/MemStats.cc,v 1.4 1998/10/31 08:09:10 stolcke Exp $";
#endif


#include "MemStats.h"

const size_t MB = 1024 * 1024;

MemStats::MemStats()
    : total(0), wasted(0)
{
}

void
MemStats::clear()
{
    total = 0;
    wasted = 0;
}

ostream &
MemStats::print(ostream &stream)
{
    return stream << "total memory " << total
		  << " (" << ((float)total/MB) << "M)" 
		  << ", used " << (total - wasted)
		  << " (" << ((float)(total - wasted)/MB) << "M)"
		  << ", wasted " << wasted
		  << " (" << ((float)wasted/MB) << "M)"
		  << endl;
}

