/*
 * testMix --
 *	Test for probability interpolation
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testMix.cc,v 1.6 2006/01/09 18:08:21 stolcke Exp $";
#endif

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "Prob.h"

extern "C" {
    extern void exit(int);
}

int
main (int argc, char *argv[])
{
    double lambda = 0.4;
    LogP a = -1.67604f;
    LogP b = -2.14148f;

    LogP r1 = ProbToLogP(lambda * LogPtoProb(a) + (1 - lambda) * LogPtoProb(b));
    printf("method1 %g\n", r1);

    LogP r2 = MixLogP(a, b, lambda);
    printf("method2 %g\n", r2);

    exit(0);
}
