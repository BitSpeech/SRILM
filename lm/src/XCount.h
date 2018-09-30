/*
 * XCount.h --
 *	Sparse integer counts stored in 2 bytes.
 *
 * Copyright (c) 1995,2005 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/XCount.h,v 1.4 2006/01/05 20:21:27 stolcke Exp $
 *
 */

#ifndef _XCount_h_
#define _XCount_h_

#include <iostream>
using namespace std;

#include "Boolean.h"
#include "Array.h"

#define XCOUNT_MAXBITS		(15)
#define XCOUNT_MAXINLINE	((1 << XCOUNT_MAXBITS)-1)

class XCount {
public:
    XCount(unsigned value = 0);
    ~XCount();

    XCount & operator= (const XCount &other);
    XCount & operator+= (unsigned value)
	{ *this = (unsigned)*this + value; return *this; };
    XCount & operator+= (XCount &value)
	{ *this = (unsigned)*this + (unsigned)value; return *this; };

    operator unsigned() const;

    void write(ostream &str) const;
	
private:
    unsigned short count:XCOUNT_MAXBITS;
    Boolean indirect:1;

    static void freeXCountTableIndex(unsigned short);
    static unsigned short getXCountTableIndex();

    static Array<unsigned> xcountTable;
    static unsigned short freeList;
};

ostream &operator<<(ostream &str, const XCount &count);

#endif /* _XCount_h_ */

