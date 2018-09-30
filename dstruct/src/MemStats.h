/*
 * MemStats.h --
 *	Memory statistics.
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/dstruct/src/RCS/MemStats.h,v 1.3 2006/08/12 06:31:24 stolcke Exp $
 *
 */

#ifndef _MemStats_h_
#define _MemStats_h_

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <stddef.h>

/*
 * The MemStats structure is used to return memory accounting 
 * information from the memstats() methods of various data types.
 */
class MemStats
{
public:
	MemStats();
	void clear();			/* reset memory stats */
	ostream &print(ostream &stream = cerr);
					/* print to cerr */

	size_t	total;			/* total allocated memory */
	size_t	wasted;			/* unused allocated memory */
};

#endif /* _MemStats_h_ */
