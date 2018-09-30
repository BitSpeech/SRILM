#!/usr/local/bin/gawk -f
#
# compare-ppls --
#	Compare two LMs for significant differences in probabilities
#	The probabilities calculated for the test set words are ranked
#	pairwise, as appropriate for submitting the result a sign test.
#
# usage: compare-ppls [mindelta=d] pplout1 pplout2
#
# where pplout1, pplout2 is the output of ngram -debug 2 -ppl for the two
# models.  d is the minimum difference of logprobs for two probs to 
# be considered different.
#
# $Header: /home/srilm/devel/utils/src/RCS/compare-ppls.gawk,v 1.4 2004/11/02 02:00:35 stolcke Exp $
#
function abs(x) {
	return (x < 0) ? -x : x;
}
BEGIN {
	sampleA_no = 0;
	sampleB_no = 0;
	mindelta = 0;
	verbose = 0;

	logINF = -100000;
}
FNR == 1 {
	if (!readingA) {
		readingA = 1;
	} else {
		readingA = 0;
	}
}
readingA && $1 == "p(" {
	if ($0 ~ /\[ -[Ii]nf/) prob = logINF;
	else prob = $10;

	sampleA[sampleA_no ++] = prob;
}
!readingA && $1 == "p(" {
	if ($0 ~ /\[ -[Ii]nf/) prob = logINF;
	else prob = $10;

	if (sampleB_no > sampleA_no) {
		printf "sample B contains more data than sample A" >> "/dev/stderr";
		exit(1);
	}
	
	if (abs(sampleA[sampleB_no] - prob) <= mindelta) {
		equal ++;
	} else if (sampleA[sampleB_no] < prob) {
		if (verbose) {
			print;
		}
		greater ++;
	}

	sampleB_no ++;
}
END {
	if (sampleB_no < sampleA_no) {
		printf "sample B contains less data than sample A" >> "/dev/stderr";
	print sampleB_no, sampleA_no;
		exit(1);
	}

	printf "total %d, equal %d, different %d, greater %d\n", \
			sampleB_no, equal, sampleB_no - equal, greater;
}
