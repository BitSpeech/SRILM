#!/usr/local/bin/gawk -f
#
# get-unigram-probs --
#	extract unigram probabilities from backoff LM file
#
# usage: get-unigram-probs bo-file
#
# $Header: /home/srilm/devel/utils/src/RCS/get-unigram-probs.gawk,v 1.2 2007/07/13 23:25:02 stolcke Exp $
#

BEGIN {
	linear = 0;

	currorder = 0;
	logzero = -99;
}

/^\\[0-9]-grams:/ {
	currorder = substr($0,2,1);
	next;
}

/^\\/ {
	currorder = 0;
	next;
}

currorder == 1 && NF > 1 {
	if (linear) {
	    print $2, $1 == logzero ? 0 : 10^$1;
	} else {
	    print $2, $1 == logzero ? "-infinity" : $1;
	}
	next;
}

