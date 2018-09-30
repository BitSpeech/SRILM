/*
 * hoeffding.cc --
 *	Test program for Hoeffding bounds
 *
 * usage: hoeffding f1 n1 f2 n2
 * output: maximum significance level
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/hoeffding.cc,v 1.6 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;
#include <stdlib.h>
#include <math.h>

int
main(int argc, char **argv)
{
    if (argc < 5) {
	cerr << "usage: " << argv[0] << " f1 n2 f2 n2\n";
	exit(1);
    }

    int f1 = atoi(argv[1]);
    int n1 = atoi(argv[2]);
    int f2 = atoi(argv[3]);
    int n2 = atoi(argv[4]);

    double delta = fabs(f1/(double)n1 - (double)f2/(double)n2);
    double dev = 1.0 / (1.0/sqrt((double)n1) + 1.0/sqrt((double)n2));
		 
    double val = 2.0 * exp(-2.0 * (delta * delta) * (dev * dev));

    cout << val << endl;

    exit(0);
}

