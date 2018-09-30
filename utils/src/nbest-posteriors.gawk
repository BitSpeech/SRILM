#!/usr/local/bin/gawk -f
#
# nbest-posteriors --
#	rescale the scores in an nbest list to reflect weighted posterior
#	probabilities
#
# usage: nbest-posteriors [ weight=W amw=AMW lmw=LMW wtw=WTW postscale=S max_nbest=M ] NBEST-FILE
#
# The output is the same input NBEST-FILE with acoustic scores set to
# the log10 of the posterior hyp proabilities and LM scores set to zero.
# postscale=S attenuates the posterior distribution by dividing combined log 
# scores by S (the default is S=LMW).
#
# If weight=W is specified the posteriors are multiplied by W.
# (This is useful to combine multiple nbest lists in a weighted fashion).
# The input should be in SRILM nbest-format.
#
# $Header: /home/srilm/devel/utils/src/RCS/nbest-posteriors.gawk,v 1.6 2001/10/17 23:03:45 stolcke Exp $
#

BEGIN {
	M_LN10 = 2.30258509299404568402;

	weight = 1.0;
	amw = 1.0;
	lmw = 8.0;
	wtw = 0.0;
	postscale = 0;
	max_nbest = 0;

	logINF = -320;		# log10 of smallest representable number
	log_total_numerator = logINF;
}

function log10(x) {
        return log(x)/M_LN10;
}
function exp10(x) {
	if (x <= logINF) {
		return 0;
	} else {
		return exp(x * M_LN10);
	}
}
function addlogs(x,y) {
    if (x<y) {
	temp = x; x = y; y = temp;
    }
    return x + log10(1 + exp10(y - x));
}

# by default, use posterior scale = lmw
NR == 1 {
	if (!postscale) {
		postscale = lmw;
	}
}

NF >= 3 {
	if (max_nbest && num_hyps == max_nbest) exit;

	num_hyps ++;

	total_score = (amw * $1 + lmw * $2 + wtw * $3)/postscale;

	if (num_hyps == 1) {
		score_offset = total_score;
	}

	total_score -= score_offset;

	#
	# store posteriors and hyp words
	#
	log_posteriors[num_hyps] = total_score;
	log_total_numerator = addlogs(log_total_numerator, total_score);

	num_words[num_hyps] = $3;

	$1 = $2 = $3 = "";
	hyps[num_hyps] = $0;
}

END {
	for (i = 1; i <= num_hyps; i ++) {
		print log10(weight) + log_posteriors[i] - log_total_numerator, \
			0, num_words[i], hyps[i];
	}
}

