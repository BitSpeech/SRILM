
/*
 * Prob.cc --
 *	Functions for handling Probs
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,2003 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /tmp_mnt/home/srilm/devel/lm/src/RCS/Prob.cc,v 1.14 2003/03/04 04:58:30 stolcke Exp $";
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "Prob.h"

const LogP LogP_Zero = -HUGE_VAL;		/* log(0) */
const LogP LogP_Inf = HUGE_VAL;			/* log(Inf) */
const LogP LogP_One = 0.0;			/* log(1) */

const int LogP_Precision = 7;	/* number of significant decimals in a LogP */

const Prob Prob_Epsilon = 3e-06;/* probability sums less than this in
				 * magnitude are effectively considered 0
				 * (assuming they were obtained by summing
				 * LogP's) */

/*
 * parseLogP --
 *	Fast parsing of floats representing log probabilities
 *
 * Results:
 *	true if string can be parsed as a float, false otherwise.
 *
 * Side effects:
 *	Result is set to float value if successful.
 *
 */
Boolean
parseLogP(const char *str, LogP &result)
{
    const unsigned maxDigits = 8;	// number of decimals in an integer

    const char *cp = str;
    const char *cp0;
    Boolean minus = false;

    /*
     * Log probabilties are typically negative values of magnitude > 0.0001,
     * and thus are usually formatted without exponential notation.
     * We parse this type of format using integer arithmetic for speed,
     * and fall back onto scanf() in all other cases.
     * We also use scanf() when there are too many digits to handle with
     * integers.
     * Finally, we also parse +/- infinity values as they are printed by 
     * printf().  These are "[Ii]nf" or "[Ii]nfinity".
     */

    /*
     * Parse optional sign
     */
    if (*cp == '-') {
	minus = true;
	cp++;
    } else if (*cp == '+') {
	cp++;
    }
    cp0 = cp;

    unsigned digits = 0;		// total value of parsed digits
    unsigned decimals = 1;		// scaling factor from decimal point
    unsigned precision = 0;		// total number of parsed digits

    /*
     * Parse digits before decimal point
     */
    while (isdigit(*cp)) {
	digits = digits * 10 + (*(cp++) - '0');
	precision ++;
    }

    if (*cp == '.') {
	cp++;

	/*
	 * Parse digits after decimal point
	 */
	while (isdigit(*cp)) {
	    digits = digits * 10 + (*(cp++) - '0');
    	    precision ++;
	    decimals *= 10;
	}
    }

    /*
     * If we're at the end of the string then we're done.
     * Otherwise there was either an error or some format we can't
     * handle, so fall back on scanf(), after checking for infinity
     * values.
     */
    if (*cp == '\0' && precision <= maxDigits) {
	result = (minus ? - (LogP)digits : (LogP)digits) / (LogP)decimals;
	return true;
    } else if ((*cp0 == 'i' || *cp0 == 'I') &&
		(strncmp(cp0, "Inf", 3) == 0 || strncmp(cp0, "inf", 3) == 0))
    {
	result = (minus ? LogP_Zero : LogP_Inf);
	return true;
    } else {
	return (sscanf(str, "%f", &result) == 1);
    }
}

