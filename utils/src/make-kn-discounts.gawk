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
# $Header: /home/srilm/devel/utils/src/RCS/make-kn-discounts.gawk,v 1.1 2002/07/17 00:39:45 stolcke Exp $
#
# usage: make-kn-discounts min=<mincount> countfile
#
BEGIN {
    min=1;
}
/^#/ {
    # skip comments
    next;
}
{
    countOfCounts[$1] = $2;
}
END {
    # Code below is essentially identical to ModKneserNey::estimate()
    # (Discount.cc).

    if (countOfCounts[1] == 0 || \
	countOfCounts[2] == 0 || \
	countOfCounts[3] == 0 || \
	countOfCounts[4] == 0) \
    {
	printf "error: one of required counts of counts is zero\n" \
	       						> "/dev/stderr";
	exit(2);
    }

    Y = countOfCounts[1]/(countOfCounts[1] + 2 * countOfCounts[2]);

    print "mincount", min;
    print "discount1", 1 - 2 * Y * countOfCounts[2] / countOfCounts[1];
    print "discount2", 2 - 3 * Y * countOfCounts[3] / countOfCounts[2];
    print "discount3+", 3 - 4 * Y * countOfCounts[4] / countOfCounts[3];
}
