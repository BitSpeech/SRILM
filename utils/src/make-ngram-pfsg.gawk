#!/usr/local/bin/gawk -f
#
# make-ngram-pfsg --
#	Create a Decipher PFSG from a trigram language model
#
# usage: make-ngram-pfsg [debug=1] [check_bows=1] [maxorder=N] backoff-lm > pfsg
#
# $Header: /home/srilm/devel/utils/src/RCS/make-ngram-pfsg.gawk,v 1.20 2001/01/22 20:14:29 stolcke Exp $
#

#########################################
#
# Output format specific code
#

BEGIN {
	logscale = 2.30258509299404568402 * 10000.5;
	round = 0.5;
	start_tag = "<s>";
	end_tag = "</s>";
	null = "NULL";

	getline pid < "/dev/pid";
	close("/dev/pid");
	tmpfile = "tmp.pfsg.trans." pid;
	debug = 0;
}

function rint(x) {
	if (x < 0) {
	    return int(x - round);
	} else {
	    return int(x + round);
	}
}

function scale_log(x) {
	return rint(x * logscale);
}

function output_for_node(name) {
	if (name == start_tag || name == end_tag) {
	    return null;
	} else if (name == bo_name || match(name, bo_name " ") == 1) {
	    return null;
	} else if (k = index(name, " ")) {
	    name2 = substr(name, k+1);
	    if (name2 == end_tag) {
		return null;
	    } else {
		return name2;
	    }
	} else {
	    return name;
	}
}

function node_exists(name) {
	return (name in node_num);
}

function node_index(name) {
	i = node_num[name];
	if (i == "") {
	    i = num_nodes ++;
	    node_num[name] = i;
	    node_string[i] = output_for_node(name);

	    if (debug) {
		print "node " i " = " name ", output = " node_string[i] \
				> "/dev/stderr";
	    }
	}
	return  i;
}

function start_grammar(name) {
	num_trans = 0;
	num_nodes = 0;
	return;
}

function end_grammar(name) {
	if (!node_exists(start_tag)) {
		print start_tag " tag undefined in LM" > "/dev/stderr";
		exit(1);
	} else if (!node_exists(end_tag)) {
		print end_tag " tag undefined in LM" > "/dev/stderr";
		exit(1);
	}

	printf "%d pfsg nodes\n", num_nodes > "/dev/stderr";
	printf "%d pfsg transitions\n", num_trans > "/dev/stderr";

	print "name " name;
	printf "nodes %s", num_nodes;
	for (i = 0; i < num_nodes; i ++) {
		printf " %s", node_string[i];
	}
	printf "\n";
	
	print "initial " node_index(start_tag);
	print "final " node_index(end_tag);
	print "transitions " num_trans;
	fflush();

	close(tmpfile);
	system("/bin/cat " tmpfile "; /bin/rm -f " tmpfile);
}

function add_trans(from, to, prob) {
	num_trans ++;
	print node_index(from), node_index(to), scale_log(prob) > tmpfile;
}

#########################################
#
# Generic code for parsing backoff file
#

BEGIN {
	maxorder = 3;
	grammar_name = "PFSG";
	bo_name = "BO";
	check_bows = 0;
	epsilon = 1e-5;		# tolerance for lowprob detection
}

NR == 1 {
	start_grammar(grammar_name);
}

NF == 0 {
	next;
}

/^ngram *[0-9][0-9]*=/ {
	num_grams = substr($2,index($2,"=")+1);
	if (num_grams > 0) {
	    order = substr($2,1,index($2,"=")-1);
	    if (order > maxorder) {
		print "ngram order exceeds maximum (" maxorder ")" \
				> "/dev/stderr";
		exit(1);
	    }
	    if (order == 2) {
		grammar_name = "BIGRAM_PFSG";
	    } if (order == 3) {
		grammar_name = "TRIGRAM_PFSG";
	    }
	}
	next;
}

/^\\[0-9]-grams:/ {
	currorder=substr($0,2,1);
	next;
}
/^\\/ {
	next;
}

#
# unigrams
#
currorder == 1 {
	uniprob = $1;
	word = $2;
	bow = $3;

	uni_prob[word] = uniprob;

	if (word != end_tag) {
	    #
	    # we always need a transition out of a unigram node
	    # (implicit bow = 1)
	    #
	    if (bow == "") {
		bow = 0;
	    } else {
		uni_bows[word] = bow;
	    }

	    if (order > 2) {
		add_trans(bo_name " " word, bo_name, bow);
	        add_trans(word, bo_name " " word, 0);
	    } else {
		add_trans(word, bo_name, bow);
	    }
	}

	if (word != start_tag) {
	    add_trans(bo_name, word, uniprob);
	}
}

#
# bigrams
#
currorder == 2 {
	biprob = $1;
	word1 = $2;
	word2 = $3;
	bow = $4;

	if (word2 == start_tag) {
	    printf "warning: ignoring bigram into start tag %s -> %s\n", \
			word1, word2 > "/dev/stderr";
	    next;
	}

	word12 = $2 " " $3;

	if (check_bows && order > 2) {
	    bi_prob[word12] = biprob;
	}

	if (bow != "" && order > 2) {
	    bi_bows[word12] = bow;

	    add_trans(word12, bo_name " " word2, bow);
	    add_trans(bo_name " " word1, word12, biprob);
	} else {
	    if (order > 2) {
		add_trans(bo_name " " word1, word2, biprob);
	    } else {
		add_trans(word1, word2, biprob);
	    }
	}

	if (check_bows && \
	    word2 in uni_prob && \
	    uni_prob[word2] + uni_bows[word1] - biprob > epsilon)
	{
	    printf "warning: bigram loses to backoff %s -> %s\n", \
			    word1, word2 > "/dev/stderr";
	}
}

#
# trigrams
#
currorder == 3 {
	triprob = $1;
	word1 = $2;
	word2 = $3;
	word3 = $4;

	if (word3 == start_tag) {
	    printf "warning: ignoring trigram into start tag %s %s -> %s\n", \
			word1, word2, word3 > "/dev/stderr";
	    next;
	}

	word12 = word1 " " word2;
	word23 = word2 " " word3;

	if (word12 in bi_bows) {
	    if (word23 in bi_bows) {
		add_trans(word12, word23, triprob);
	    } else {
		add_trans(word12, word3, triprob);
	    } 

	    if (check_bows && \
		word23 in bi_prob && \
		bi_prob[word23] + bi_bows[word12] - triprob > epsilon)
	    {
		printf "warning: trigram loses to backoff %s %s -> %s\n", \
				word1, word2, word3 > "/dev/stderr";
	    }
	}
}

END {
	end_grammar(grammar_name);
}
