#!/usr/local/bin/gawk -f
#
# add-pauses-to-pfsg --
#	Modify Decipher PFSG to allow pauses between words
#
# $Header: /home/srilm/devel/utils/src/RCS/add-pauses-to-pfsg.gawk,v 1.8 2001/01/22 20:19:49 stolcke Exp $
#
BEGIN {
	pause = "-pau-";
	top_level_name = "TOP_LEVEL";
	pause_filler_name = "PAUSE_FILLER";
	null = "NULL";

	wordwrap = 1;		# wrap pause filler around words
	pauselast = 0;		# make pauses follow wrapped words
	version = 0;		# no "version" line by default
}

#
# output the TOP_LEVEL model
#	oldname is the name of the original pfsg
function print_top_level(oldname) {
	if (version) {
		print "version " version "\n";
	}
	print "name " top_level_name;
	if (pauselast) {
	    print "nodes 4 " null " " pause_filler_name " " oldname " " null;
	} else {
	    print "nodes 4 " null " " oldname " " pause_filler_name " " null;
	}
	print "initial 0"
	print "final 3"
	print "transitions 4"
	print "0 1 0"
	print "1 2 0"
	if (pauselast) {
	    print "0 2 0"
	} else {
	    print "1 3 0"
	}
	print "2 3 0"
	print "";
}

#
# output a pause wrapper for word
#
function print_word_wrapper(word) {
	print "name " toupper(word) "_P";
	if (pauselast) {
	    print "nodes 3 " word " " pause_filler_name " " null;
	} else {
	    print "nodes 3 " null " " pause_filler_name " " word;
	}
	print "initial 0";
	print "final 2";
	print "transitions 3";
	print "0 1 0";
	print "1 2 0";
	print "0 2 0";
	print "";
}

#
# output the pause filler
#
function print_pause_filler() {
	print "name " pause_filler_name;
	print "nodes 4 " null " " null " " pause " " null;
	print "initial 0";
	print "final 3";
	print "transitions 4";
	print "0 1 0";
	print "1 2 0";
	print "2 3 0";
	print "2 1 0";
}

NF == 0 {
	print;
	next;
}

#
# first time we see a pfsg name, issue a top-level wrapper for it.
#
$1 == "name" && !have_top_level {
	print_top_level($2);
	print;
	have_top_level = 1;
	next;
}

#
# check that a node name is word
# word nodes contain at least one lowercase character and are not
# surrounded by "*...*" (which indicates a class name).
#
function is_word(w) {
	return w !~ /^\*.*\*$/ && w ~ /[a-z]/;
}

#
# maps word nodes to wrapper nodes
#
$1 == "nodes" {
	numnodes = $2;
	printf "nodes %d", numnodes;

	for (i = 0; i < numnodes; i ++) {
	    node_name = $(i + 3);

	    # if it contains lowercase characters it's a word and
	    # needs to wrapped
	    if (wordwrap && is_word(node_name)) {
		if (!(node_name in all_words)) {
		    all_words[node_name] = 1;
		    words[++num_words] = node_name;
		}
		printf " %s", toupper(node_name) "_P";
	    } else {
		printf " %s", node_name;
	    }
	}
	printf "\n";
	next;
}

{
	print;
}

END {
	#
	# output the word wrappers
	#
	if (wordwrap) {
	    for (i = 1; i <= num_words; i ++) {
		print_word_wrapper(words[i]);
	    }
	}

	print_pause_filler();
}
