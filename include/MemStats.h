/*
 * MemStats.h --
 *	Memory statistics.
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/speech/stolcke/project/lm/src/dstruct/RCS/MemStats.h,v 1.1 1995/06/22 07:35:01 stolcke Exp $
 *
 */

#ifndef _MemStats_h_
#define _MemStats_h_

#include <stddef.h>
#include <iostream.h>

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
