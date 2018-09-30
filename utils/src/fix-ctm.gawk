#!/usr/local/bin/gawk -f
#
# Post-process CTM files output by lattice-tool -output-ctm to
# use global conversation-relative time marks and channel ids.
# (This requires that the waveform names conform to our standard
# formats, the same as in sentid-to-ctm.)
#
# $Header: /home/srilm/devel/utils/src/RCS/fix-ctm.gawk,v 1.6 2009/11/19 00:10:02 stolcke Exp $
#
BEGIN {
        # time to add to word start times (should be about half FE window size)
        phase_shift = 0.01;

	tag_pat = "^<.*>$";
	noise_pat = "^\\[.*\\]$";
        fragment_pat = "-$";
	pause = "-pau-";

	channel_letters = 0;

        # hesitations (best deleted for NIST scoring;
        # should be kept in sync with GLM filter file)
        hesitation["uh"] = 1;
        hesitation["um"] = 1;
        hesitation["eh"] = 1;
        hesitation["mm"] = 1;
        hesitation["hm"] = 1;
        hesitation["ah"] = 1;
        hesitation["huh"] = 1;
        hesitation["ha"] = 1;
        hesitation["er"] = 1;
        hesitation["oof"] = 1;
        hesitation["hee"] = 1;
        hesitation["ach"] = 1;
        hesitation["eee"] = 1;
        hesitation["ew"] = 1;

	sort_cmd = "sort -b -k 1,1 -k 2,2 -k 3,3n";
}
{
	sentid = $1;
	start_time = $3;
	duration = $4;
	word = $5;
	confidence = $6;

	if (sentid == last_sentid && start_time == "?") {
		start_time = last_end_time;
		duration = 0;
	}

        if (match(sentid, "_[12]_[-0-9][0-9]*_[0-9][0-9]*$")) {
           # waveforms with [12] channel id, timemarks 1/1000s
           # NOTE: this form is used by the segmenter
           conv = substr(sentid, 1, RSTART-1);
           split(substr(sentid, RSTART+1), sentid_parts, "_");
           channel = sentid_parts[1];
           start_offset = sentid_parts[2] / 1000;
           end_offset = sentid_parts[3] / 1000;
        } else if (match(sentid, "_[AB]_[-0-9][0-9]*_[0-9][0-9]*$")) {
           conv = substr(sentid, 1, RSTART-1);
           split(substr(sentid, RSTART+1), sentid_parts, "_");
           channel = sentid_parts[1];
           start_offset = sentid_parts[2] / 100;
           end_offset = sentid_parts[3] / 100;
	} else {
           print "cannot parse sentid " sentid >> "/dev/stderr";
           conv = sentid;
           channel = 1;
           start_offset = 0;
           end_offset = 10000;
        }

	if (channel_letters && channel ~ /^[0-9]/) {
		channel = sprintf("%c", 64+channel);
	}

	# exclude sentence start/end tags
	if (word ~ tag_pat) next;

	speaker_id = conv "_" channel;

	ncomps = split(word, word_comps, "_");

	for (j = 1; j <= ncomps; j ++) {
		this_word = word_comps[j];

		if (this_word == pause) {
		    next;
		} else if (this_word in hesitation) {
		    word_type = "fp";
		} else if (this_word ~ fragment_pat) {
		    word_type = "frag";
		} else if (this_word ~ noise_pat) {
		    word_type = "non-lex";
		} else {
		    word_type = "lex";
		}

		print conv, channel, \
			start_offset + start_time + phase_shift + \
				(j - 1) * duration/ncomps,\
			duration/ncomps, \
			this_word, \
			confidence, \
			word_type, \
			(word_type == "non-lex" ? \
				"null" : speaker_id) \
				 | sort_cmd;
	}

	last_end_time = start_time + duration;
	last_sentid = sentid;
}

