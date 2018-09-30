#!/usr/local/bin/gawk -f
#
# compute-oov-rate --
#	Compute OOV word rate from a vocabulary and a unigram count file
#
# usage: compute-oov-rate vocab countfile ...
#
# Assumes unigram counts do not have repeated words.
#
# $Header: /home/srilm/devel/utils/src/RCS/compute-oov-rate,v 1.1 1995/11/13 22:49:57 stolcke Exp $
#

# Read vocab
#
ARGIND == 1 {
	vocab[$1] = 1;
}
#
# Read counts
#
ARGIND > 1 {
	if ($1 == "<s>" || $1 == "</s>") {
		next;
	}

	total_count += $2;
	total_types ++;

	if (!vocab[$1]) {
		oov_count += $2;
		oov_types ++; 
	}
}
END {
	printf "OOV tokens: %d out of %d (%.2f\%)\n", \
			oov_count, total_count, 100 * oov_count/total_count;
	printf "OOV types: %d out of %d (%.2f\%)\n", \
			oov_types, total_types, 100 * oov_types/total_types;
}
