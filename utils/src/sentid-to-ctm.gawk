#!/usr/local/bin/gawk -f
#
# sentid-to-ctm --
#	Format a sentid transcript file into CTM format, faking time marks
#	by spacing words evenly across the duration of the segment
#
#	Note: this script makes assumptions about the structure of sentence
#	ID, specifically, how they encode speakers and timemarks.
#
# $Header: /home/srilm/devel/utils/src/RCS/sentid-to-ctm.gawk,v 1.5 2001/04/06 17:22:19 stolcke Exp $
#

BEGIN {
	# time to leave at edges of segments
	delta = 0.07;

	pause = "-pau-";
	reject = "@reject@";

	sort_cmd = "sort -u +0 -1 +1 -2 +2nb -3";
}

# read confidences if given
NR == 1 {
	if (confidences) {
		while ((getline line < confidences) > 0) {
			nvalues = split(line, a);
			if (nvalues > 0) {
				conf_lines[a[1]] = line;
			}
		}
	}
}

function is_nonspeech(w) {
	return w == pause || w == reject || w ~/^\[.*\]$/;
}

{
	orig_sentid = sentid = $1;

	# strip speaker diacritics
	sub("_s[1-9]$", "", sentid);

	# derive channel and time information from sentids
	# look for a pattern that encodes channel and 
	# start/end times
	if (match(sentid, "_[AB]_[-0-9][0-9]*_[0-9][0-9]*$")) {
	   conv = substr(sentid, 1, RSTART-1);
	   split(substr(sentid, RSTART+1), sentid_parts, "_");
	   channel = sentid_parts[1];
	   start_offset = sentid_parts[2] / 100;
	   end_offset = sentid_parts[3] / 100;
	# new sentids used by Ramana for SPINE segmentations
	} else if (match(sentid, "_[AB]_[-0-9][0-9]*_[0-9][0-9]*_[-0-9][0-9]*_[0-9][0-9]*$")) {
	   conv = substr(sentid, 1, RSTART-1);
	   split(substr(sentid, RSTART+1), sentid_parts, "_");
	   channel = sentid_parts[1];
	   start_offset = (sentid_parts[2]+sentid_parts[4]) / 100;
	   end_offset = (sentid_parts[2]+sentid_parts[5]) / 100;
	} else {
	   print "cannot parse sentid " sentid > "/dev/stderr";
	   conv = sentid;
	   channel = "?";
	   start_offset = 0;
	   end_offset = 10000;
	}

	$1 = "";
	$0 = $0;

	numwords = NF;

	if (numwords > 0) {
	    word_dur = (end_offset - start_offset - 2 * delta)/numwords;
	} else {
	    word_dur = 0;
	}

	# find confidence values for this sentid
	if (confidences) {
		if (!(orig_sentid in conf_lines)) {
		    print "no confidences for " orig_sentid > "/dev/stderr";
		} else {
		    delete conf_values;
		    n_conf_values = \
			split(conf_lines[orig_sentid], conf_values);
		}
	}

	for (i = 1; i <= numwords; i ++) {
		if (is_nonspeech($i)) continue;

		start_time = start_offset + delta + (i - 1) * word_dur;

		if (i + 1 in conf_values) {
			conf_value = conf_values[i + 1];
		} else {
			conf_value = 0;
		}

		# split multiwords
		ncomps = split($i, word_comps, "_");

		for (j = 1; j <= ncomps; j ++) {
			print conv, channel, \
				start_time + (j - 1) * word_dur/ncomps,\
				word_dur/ncomps, \
				toupper(word_comps[j]), \
				conf_value | sort_cmd;
		}
	}

	if (orig_sentid in conf_lines && numwords != n_conf_values - 1) {
	    print "mismatched number of confidences for " orig_sentid \
						> "/dev/stderr";
	}
}
