#!/usr/local/bin/gawk -f
#
# wlat-stats --
#	Compute statistics of word posterior lattices
#
# $Header: /home/srilm/devel/utils/src/RCS/wlat-stats.gawk,v 1.2 2001/08/02 18:20:17 stolcke Exp $
#
BEGIN {
	name = "";
	nhyps = 0;
	entropy = 0;
	nwords = 0;

	nsub = nins = ndel = 0;

	M_LN10 = 2.30258509299404568402;

	empty_hyp = "*DELETE*";
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
	align_pos = $2;

	best_hyp = "";
	best_posterior = 0;
	for (i = 3; i <= NF; i += 2) {
	    word = $i;

	    if (word != "*DELETE*") {
		nhyps ++;
	    }

	    prob = $(i + 1);
	    if (prob > 0) {
		    entropy -= prob * log(prob);
	    }

	    if (prob > best_posterior) {
		best_posterior = prob;
		best_hyp = word;
	    }
	}
}
$1 == "reference" && $2 == align_pos {
	if ($3 != empty_hyp) {
	    nwords ++;

	    if (best_hyp == empty_hyp) {
		ndel ++;
	    } else if (best_hyp != $3) {
		nsub ++;
	    }
	} else {
	    if (best_hyp != empty_hyp) {
		nins ++;
	    }
	}

	align_pos = -1;
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
	if (nwords  > 0) {
	    nerrors = nsub + nins + ndel;
	    printf name (name != "" ? " " : "") \
		   nerrors " errors " nerrors*100/nwords " WER " \
		   nsub*100/nwords " SUB " nins*100/nwords " INS " \
		   ndel*100/nwords " DEL\n";
	}
}

