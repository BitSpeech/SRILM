/*
 * testMix --
 *	Test for probability interpolation
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/testMix.cc,v 1.7 2008/01/21 22:53:51 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "Prob.h"

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
