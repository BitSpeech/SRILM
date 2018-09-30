/*
 * testMix --
 *	Test for probability interpolation
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testMix.cc,v 1.4 1999/08/01 09:31:35 stolcke Exp $";
#endif

#include <stdio.h>
#include <unistd.h>

#include "Prob.h"

extern "C" {
    extern void exit(int);
}

int
main (int argc, char *argv[])
{
    double lambda = 0.4;
    LogP a = -1.67604;
    LogP b = -2.14148;

    LogP r1 = ProbToLogP(lambda * LogPtoProb(a) + (1 - lambda) * LogPtoProb(b));
    printf("method1 %g\n", r1);

    LogP r2 = MixLogP(a, b, lambda);
    printf("method2 %g\n", r2);

    exit(0);
}
