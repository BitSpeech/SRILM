/*
 * testXCount --
 *	tests for XCount
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2005, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testXCount.cc,v 1.3 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;

#include "XCount.h"

int main()
{
    XCount a(1), b(1);

    cerr << "sizeof(XCcount) = " << sizeof(a) << endl;

    unsigned x = a + 10;

    a += 1;

    a = a + a;

    cerr << "x = " << x << " a = " << a << " b = " << b << endl;

    a = 40000;
    for (unsigned i = 0; i < 40000; i ++) {
    	a += 1;
	b += (XCount)1;
    }

    cerr << "a = " << a << " b = " << b << endl;

    exit(0);
}

