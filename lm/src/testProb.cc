/*
 * testProb --
 *	Test arithmetic with log probabilities
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2000, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testProb.cc,v 1.1 2000/07/13 06:16:45 stolcke Exp $";
#endif


#include <iostream.h>
#include <stdio.h>

#include "Prob.h"

int
main(int argc, char **argv)
{
    if (argc < 2) {
    	cerr << "usage: testProb p1 [p2]\n";
	exit(2);
    }

    if (argc < 3) {
    	Prob p;

	sscanf(argv[1], "%lf", &p);
	LogP lp = ProbToLogP(p);

    	cout << "log(p) = " << lp << " " << LogPtoProb(lp) << endl;
    } else {
    	Prob p, q;

	sscanf(argv[1], "%lf", &p);
	sscanf(argv[2], "%lf", &q);

	LogP lp = ProbToLogP(p);
	LogP lq = ProbToLogP(q);
	LogP lpq = AddLogP(lp,lq);

    	cout << "log(p + q) = " << lpq << " " << LogPtoProb(lpq) << endl;

	if (lp >= lq) {
	    lpq = SubLogP(lp,lq);

	    cout << "log(p - q) = " << lpq << " " << LogPtoProb(lpq) << endl;
	}
    }

    exit(0);
}
