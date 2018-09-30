
#include <iostream.h>

#define ZERO_INITIALIZE
#include "Array.cc"

#define BASE 10

int
main()
{
    Array<unsigned int> myarray(BASE);
    Array<char *> array2;
    Array<const char *> array3;
    Array<float> array4;
    Array<double> array5;
    Array<char> array6;
    int i;

    for (i = BASE+1; i <= 20; i++) {
	myarray[i] = i * i;
    }

    for (i = BASE; i < BASE + myarray.size(); i++) {
	cout << "i = " << i << " myarray[i] = " << myarray[i] << endl;
    }

    cout << "size = " << myarray.size() << endl;

    cout << myarray.data()[BASE+3] << endl;

    exit(0);
}
