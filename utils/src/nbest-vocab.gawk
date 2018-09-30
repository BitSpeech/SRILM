#!/usr/local/bin/gawk -f
#
# nbest-vocab --
#	extract vocabulary used in nbest lists
#
# usage: nbest-vocab NBEST-FILE ... > VOCAB
#
# $Header: /home/srilm/devel/utils/src/RCS/nbest-vocab.gawk,v 1.1 2003/02/16 16:07:46 stolcke Exp $
#

BEGIN {
	nbestformat = 0;
}

$1 ~ /^NBestList1\.0/ {
	nbestformat = 1;
	next;
}

$1 ~ /^NBestList2\.0/ {
	nbestformat = 2;
	next;
}

NF > 1 {
	if (nbestformat == 1) {
	    # for Decipher nbest format 1 we use the aggregate score only
	    for (i = 2; i <= NF; i ++) {
		is_word[$i] = 1;
	    }
	} else if (nbestformat == 2) {
	    prev_end_time = -1;
	    for (i = 2; i <= NF; i += 11) {
		start_time = $(i + 3);
		end_time = $(i + 5);

		# skip tokens that are subsumed by the previous word
		# (this eliminates phone and state symbols)
		if (start_time > prev_end_time) {
		    is_word[$i] = 1;

		    prev_end_time = end_time;
		}
	    }
	} else {
	    for (i = 4; i <= NF; i ++) {
		is_word[$i] = 1;
	    }
	}
}

END {
	for (word in is_word) {
		print word;
	}
}

