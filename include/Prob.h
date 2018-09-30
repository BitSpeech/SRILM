/*
 * Prob.h --
 *	Probabilities and stuff
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/Prob.h,v 1.18 2001/06/28 23:02:32 stolcke Exp $
 *
 */

#ifndef _Prob_h_
#define _Prob_h_

#include <math.h>
#include <assert.h>

#include "Boolean.h"

extern "C" {
    double atof(const char *)
#ifdef __THROW
    __THROW __attribute_pure__
#endif
    ;	/* might be missing from math.h or stdlib.h */
}

/*
 * Types
 */
typedef float LogP;		/* A log-base-10 probability */
typedef double LogP2;		/* A log-base-10 probability, double-size */
typedef double Prob;		/* a straight probability */

/*
 * Constants
 */
extern const LogP LogP_Zero;		/* log(0) = -Infinity */
extern const LogP LogP_Inf;		/* log(Inf) = Infinity */
extern const LogP LogP_One;		/* log(1) = 0 */

extern const int LogP_Precision;	/* number of significant decimals
				 	 * in a LogP */

extern const Prob Prob_Epsilon;		/* probability sum considered not
					 * significantly different from 0 */

/*
 * Convenience functions for handling LogPs
 *	Many of these are time critical, so we use inline definitions.
 */

Boolean parseLogP(const char *string, LogP &prob);

inline Prob LogPtoProb(LogP prob)
{
    if (prob == LogP_Zero) {
    	return 0;
    } else {
	return exp(prob * M_LN10);
    }
}

inline Prob LogPtoPPL(LogP prob)
{
    return exp(- prob * M_LN10);
}

inline LogP ProbToLogP(Prob prob)
{
    return log10(prob);
}

inline LogP MixLogP(LogP prob1, LogP prob2, double lambda)
{
    return ProbToLogP(lambda * LogPtoProb(prob1) +
			(1 - lambda) * LogPtoProb(prob2));
}

inline LogP2 AddLogP(LogP2 x, LogP2 y)
{
    if (x<y) {
	LogP2 temp = x; x = y; y = temp;
    }
    if (y == LogP_Zero) {
	return x;
    } else {
	LogP2 diff = y - x;
	return x + log10(1.0 + exp(diff * M_LN10));
    }
}

inline LogP2 SubLogP(LogP2 x, LogP2 y)
{
    assert(x >= y);
    if (x == y) {
	return LogP_Zero;
    } else if (y == LogP_Zero) {
    	return x;
    } else {
	LogP2 diff = y - x;
	return x + log10(1.0 - exp(diff * M_LN10));
    }
}

/*
 * Bytelogs and Intlogs are scaled log probabilities used in the SRI
 * DECIPHER(TM) recognizer. 
 * Note: DECIPHER actually flips the sign on bytelogs, we keep them negative.
 */

typedef int Bytelog;
typedef int Intlog;

inline Bytelog ProbToBytelog(Prob prob)
{
    return (int)rint(log(prob) * (10000.5 / 1024.0));
}

inline Intlog ProbToIntlog(Prob prob)
{
    return (int)rint(log(prob) * 10000.5);
}

inline Bytelog LogPtoBytelog(LogP prob)
{
    return (int)rint(prob * (M_LN10 * 10000.5 / 1024.0));
}

inline Intlog LogPtoIntlog(LogP prob)
{
    return (int)rint(prob * (M_LN10 * 10000.5));
}

inline LogP BytelogToLogP(Bytelog bytelog)
{
    return bytelog * (1024.0 / 10000.5 / M_LN10);
}

const int BytelogShift = 10;

inline  Bytelog IntlogToBytelog(Intlog intlog)
{
    int bytelog = ((-intlog) + (1 << (BytelogShift-1))) >> BytelogShift;

    if (bytelog > 255) {
	bytelog = 255;
    }
    return -bytelog;
}

inline Intlog BytelogToIntlog(Bytelog bytelog)
{
    return bytelog << BytelogShift;
}

#endif /* _Prob_h_ */

