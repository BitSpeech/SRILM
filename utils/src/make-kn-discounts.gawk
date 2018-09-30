#!/usr/local/bin/gawk -f
#
# make-kn-discounts --
#	generate modified Kneser-Ney discounting parameters from a
#	count-of-count file
#
#	The purpose of this script is to do the KN computation off-line,
#	without ngram-count having to read all counts into memory.
#	The output is compatible with the ngram-count -kn<n> options.
#
# $Header: /home/srilm/devel/utils/src/RCS/make-kn-discounts.gawk,v 1.4 2007/06/17 01:21:18 stolcke Exp $
#
# usage: make-kn-discounts min=<mincount> countfile
#
BEGIN {
    min = 1;
}

/^#/ {
    # skip comments
    next;
}

{
    countOfCounts[$1] = $2;
    if ($1 != "total" && $1 > maxCount && $2 > 0) {
	maxCount = $1;
    }
}

#
# Estimate missing counts-of-counts f(k) based on the empirical law
#
#	log f(k) - log f(k+1) = a / k
#
# for some constant a dependent on the distribution.
#
function handle_missing_counts() {

    #
    # compute average a value based on well-defined counts-of-counts
    #
    a_sum = 0;

    for (k = maxCount - 1; k > 0; k --) {
	if (countOfCounts[k] == 0) break;

	a =  k * (log(countOfCounts[k]) - log(countOfCounts[k + 1]));

	if (debug) {
		print "k = " k ", a = " a > "/dev/stderr";
	}

	a_sum += a;
    }

    if (maxCount - 1 == k) {
	# no data to estimate a, give up
	return;
    }

    avg_a = a_sum / (maxCount - k - 1);

    if (debug) {
	print "average a = " avg_a > "/dev/stderr";
    }

    ## print "avg_a", avg_a > "/dev/stderr";

    for ( ; k > 0; k --) {
	if (countOfCounts[k] == 0) {
	    countOfCounts[k] = exp(log(countOfCounts[k + 1]) + avg_a / k);

	    print "estimating missing count-of-count " k \
					" = " countOfCounts[k] > "/dev/stderr";
	}
    }
}

END {
    # Code below is essentially identical to ModKneserNey::estimate()
    # (Discount.cc).

    handle_missing_counts();

    if (countOfCounts[1] == 0 || \
	countOfCounts[2] == 0 || \
	countOfCounts[3] == 0 || \
	countOfCounts[4] == 0) \
    {
	printf "error: one of required counts of counts is zero\n" \
	       						>> "/dev/stderr";
	exit(2);
    }

    Y = countOfCounts[1]/(countOfCounts[1] + 2 * countOfCounts[2]);

    print "mincount", min;
    print "discount1", 1 - 2 * Y * countOfCounts[2] / countOfCounts[1];
    print "discount2", 2 - 3 * Y * countOfCounts[3] / countOfCounts[2];
    print "discount3+", 3 - 4 * Y * countOfCounts[4] / countOfCounts[3];
}
