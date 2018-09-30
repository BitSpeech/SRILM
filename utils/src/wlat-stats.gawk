#!/usr/local/bin/gawk -f
#
# wlat-stats --
#	Compute statistics of word posterior lattices
#
# $Header: /home/srilm/devel/utils/src/RCS/wlat-stats.gawk,v 1.1 2001/06/10 22:25:50 stolcke Exp $
#
BEGIN {
	name = "";
	nhyps = 0;
	entropy = 0;
	nwords = 0;

	M_LN10 = 2.30258509299404568402;
}

#
# word lattice format:
#	node 46 them 11 0.011827 45 0.0111445 13 0.000682478 ...
#
$1 == "node" {
	word = $3;
	posterior = $5;

	if (word != "NULL") {
	    nhyps ++;
	}

	if (posterior > 0) {
	    for (i = 6; i <= NF; i += 2) {
		prob = $(i + 1);

		if (prob > 0) {
		    entropy -= prob * log(prob/posterior);
		}
	    }
	}
}

#
# confusion network format:
#	align 4 okay 0.998848 ok 0.00113834 i 1.06794e-08 a 4.48887e-08 ...
#
$1 == "align" {
	for (i = 3; i <= NF; i += 2) {
	    word = $i;

	    if (word != "*DELETE*") {
		nhyps ++;
	    }

	    prob = $(i + 1);
	    if (prob > 0) {
		    entropy -= prob * log(prob);
	    }
	}
}
$1 == "reference" {
	if ($3 != "*DELETE*") {
	    nwords ++;
	}
}

END {
	printf name (name != "" ? " " : "") \
	       nhyps " hypotheses " \
	       entropy/M_LN10 " entropy";
	if (nwords > 0) {
	    printf " " nwords " words " nhyps/nwords " hyps/word " \
		  entropy/M_LN10/nwords " entropy/word";
	}
	printf "\n";
}

