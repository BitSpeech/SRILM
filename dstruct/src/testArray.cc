//
// Testi for Array datastructure
//
// Copyright (c) 1995-2010 SRI International.  All Rights Reserved.
//
// $Header: /home/srilm/devel/dstruct/src/RCS/testArray.cc,v 1.13 2010/06/02 04:52:43 stolcke Exp $
//

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif

#define ZERO_INITIALIZE
#include "Array.cc"

#define BASE 10

void
printArray(unsigned *a, unsigned start, unsigned end)
{
    for (unsigned i = start; i < end; i++) {
	cout << "i = " << i << " myarray[i] = " << a[i] << endl;
    }
}

int
main()
{
    Array<unsigned int> myarray(BASE);
    Array<char *> array2;
    Array<const char *> array3;
    Array<float> array4;
    Array<double> array5;
    Array<char> array6;
    unsigned i;

    for (i = BASE+1; i <= 20; i++) {
	myarray[i] = i * i;
    }

    printArray(myarray, BASE, BASE + myarray.size());

    cout << "size = " << myarray.size() << endl;

    cout << myarray.data()[BASE+3] << endl;

    cout << "*** testing copy constructor ***\n";

    Array<unsigned int> myarray2(myarray);

    for (i = BASE; i < BASE + myarray.size(); i++) {
	cout << "i = " << i << " myarray2[i] = " << myarray2[i] << endl;
    }

    cout << "*** runtime-sized array ***\n";

    unsigned dsize = 10;
    makeArray(unsigned, darray, dsize);

    for (i = 0; i < dsize; i++) {
	darray[i] = i * i * i;
    }

    printArray(darray, 0, dsize);

    exit(0);
}
