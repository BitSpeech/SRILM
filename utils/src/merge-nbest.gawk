#!/usr/local/bin/gawk -f
#
# merge-nbest --
#	merge hyps from multiple N-best lists into a single list
#
# $Header: /home/srilm/devel/utils/src/RCS/merge-nbest.gawk,v 1.2 2001/06/22 00:59:42 stolcke Exp $
#

BEGIN {
	bytelogscale = 2.30258509299404568402 * 10000.5 / 1024.0;

	use_orig_hyps = 1;
	last_nbestformat = -1;

	nbestmagic1 = "NBestList1.0";
	nbestmagic2 = "NBestList2.0";
	pause = "-pau-";

	max_nbest = 0;
	multiwords = 0;
	nopauses = 0;
}

function process_nbest(file) {
	if (file ~ /.*\.gz$|.*\.Z/) {
	    input = "exec gunzip -c " file;
	} else {
	    input = "exec cat " file;
	}

	nbestformat = 0;
	num_hyps = 0;

	while ((status = (input | getline)) > 0) {
	    if ($1 == nbestmagic1) {
		nbestformat = 1;
	    } else if ($1 == nbestmagic2) {
		nbestformat = 2;
	    } else {
		words = "";
		num_hyps ++;

		if (max_nbest > 0 && num_hyps > max_nbest) {
		    break;
		}

		if (nbestformat == 1) {
		    for (i = 2; i <= NF; i++) {
			words = words " " $i;
		    }
		    score = $1;
		} else if (nbestformat == 2) {
		    prev_end_time = -1;
		    for (i = 2; i <= NF; i += 11) {
			start_time = $(i + 3);
			end_time = $(i + 5);

			# skip tokens that are subsumed by the previous word
			# (this eliminates phone and state symbols)
			if (start_time > prev_end_time) {
			    words = words " " $i;

			    ac_score += $(i + 9);
			    lm_score += $(i + 7);

			    prev_end_time = end_time;
			}
		    }
		    score = $1;
		} else {
		    for (i = 4; i <= NF; i++) {
			words = words " " $i;
		    }
		    score = "(" (($1 + 8 * $2) * bytelogscale) ")";

		}

		# resolve multiwords and eliminate pauses if so desired
		if (multiwords) {
			gsub("_", " ", words);
		}
		if (nopauses) {
			gsub(" " pause, " ", words);
		}

		# if word sequence is new, record it
		if (!(words in scores)) {
		    scores[words] = score;
		    hyps[words] = $0;
		}

	        if (last_nbestformat < 0) {
		    last_nbestformat = nbestformat;
		} else if (nbestformat != last_nbestformat) {
		    use_orig_hyps = 0;
		    last_nbestformat = nbestformat;
		}
	    }
	}
	if (status < 0) {
		print "error opening " file > "/dev/stderr";
	}

	close(input);
}

function output_nbest() {
	if (!use_orig_hyps || use_orig_hyps && last_nbestformat == 1) {
		print nbestmagic1;
	} else if (use_orig_hyps && last_nbestformat == 2) {
		print nbestmagic2;
	}

	for (words in scores) {
	    if (use_orig_hyps) {
		print hyps[words];
	    } else {
		print score words;
	    }
	}
}

BEGIN {
	if (ARGC < 2) {
	    print "usage: " ARGV[0] " N-BEST1 N-BEST2 ..." \
			    > "/dev/stderr";
	    exit(2);
	}

	for (arg = 1; arg < ARGC; arg ++) {
	    if (equals = index(ARGV[arg], "=")) {
		var = substr(ARGV[arg], 1, equals - 1);
		val = substr(ARGV[arg], equals + 1);

	        if (var == "multiwords") {
		    multiwords = val + 0;
		} else if (var == "max_nbest") {
		    max_nbest = val + 0;
		} else if (var == "nopauses") {
		    nopauses = val + 0;
		} else if (var == "use_orig_hyps") {
		    use_orig_hyps = val + 0;
		} 
	    } else {
	        process_nbest(ARGV[arg]);
	    }
	}

	output_nbest();
}

