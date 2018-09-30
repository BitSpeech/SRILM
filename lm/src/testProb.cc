/*
 * testProb --
 *	Test arithmetic with log probabilities
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2000-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testProb.cc,v 1.7 2006/08/12 06:46:11 stolcke Exp $";
#endif


#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <stdlib.h>
#include <stdio.h>

#include "Prob.h"

/*
 * Simulate the rounding going on from the original LM LogP scores to the
 * bytelogs in the recognizer:
 * - PFSGs encode LogP as intlogs
 * - Nodearray compiler maps intlogs to bytelogs
 */
#define RoundToBytelog(x)	BytelogToLogP(IntlogToBytelog(LogPtoIntlog(x)))

int
main(int argc, char **argv)
{
    if (argc < 2) {
    	cerr << "usage: testProb p1 [p2]\n";
	exit(2);
    }

    cout << "log(0) = " << LogP_Zero << endl;
    cout << "log(inf) = " << LogP_Inf << endl;

    if (argc < 3) {
    	Prob p;

	sscanf(argv[1], "%lf", &p);
	LogP lp = ProbToLogP(p);

    	cout << "log(p) = " << lp << " " << LogPtoProb(lp) << endl;
    	cout << "Decipher log(p) = " << RoundToBytelog(lp)
		<< " " << LogPtoProb(RoundToBytelog(lp))
		<< " " << LogPtoIntlog(lp)
		<< " " << IntlogToBytelog(LogPtoIntlog(lp)) << endl;
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
