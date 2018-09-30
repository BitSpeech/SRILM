#!/usr/local/bin/gawk -f
#
# combine acoustic scores in nbest lists with additional acoustic score files
# (used by rescore-acoustic)
#
# Setting the "max_nbest" limits the number of hyps retrieved from each
# input score list.
# If max_nbest is set and an additional score file contains less values
# than the nbest list is long, missing values are filled in with the
# minimal score found in that file.
#
# $Header: /home/srilm/devel/utils/src/RCS/combine-acoustic-scores,v 1.1 1998/09/03 07:27:09 stolcke Exp $
#
function get_from_file(i) {
	if (ARGV[i] ~ /\.gz$/) {
		status = ("exec gunzip -c " ARGV[i] | getline);
	} else {
		status = (getline < ARGV[i]);
	}
	if (status < 0) {
		print "error reading from " ARGV[i] > "/dev/stderr";
		exit 1;
	}
	return status;
}

BEGIN {
	hypno = 0;

	sentid = ARGV[1];
	sub(".*/", "", sentid);
	sub("\\.gz$", "", sentid);
	sub("\\.score$", "", sentid);

	bytelogscale = 1024.0 / 10000.5 / 2.30258509299404568402;

	nweights = split(weights, weight);
	if (nweights != ARGC - 1) {
		print "number of weights doesn't match number of score files" \
					> "/dev/stderr";
		exit 1;
	}

	while ((max_nbest == 0 || hypno < max_nbest) && get_from_file(1)) {

		old_ac = $1; $1 = "";
		hyp = $0;

		total_ac = weight[1] * old_ac;
		for (i = 2; i < ARGC; i ++) {
			if (!get_from_file(i)) {
				if (max_nbest == 0) {
					print "missing score in " ARGV[i] \
						 > "/dev/stderr";
					exit 2
				} else {
					new_ac = min_score[i];
				}
			} else {
				# skip nbest header
				if ($1 ~ /NBestList/) {
					i --; 
					continue;
				}

				new_ac = $1;

				# handle decipher-style scores
				if (new_ac ~ /\(.*\)/) {
					gsub("[()]", "", new_ac);
					new_ac *= bytelogscale;
				}

				# replace minimum score if needed
				if (!(i in min_score) || $1 < min_score[i]) {
					min_score[i] = new_ac;
				}
			}
			total_ac += weight[i] * new_ac;
		}

		print total_ac hyp;

		hypno ++;
	}
}
