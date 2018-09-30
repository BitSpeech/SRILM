#!/usr/local/bin/gawk -f
#
# make-hiddens-lm --
#	Create a hidden-sentence-boundary ngram LM from a standard one
#
# This script edits a ARPA backoff model file as follows:
#
# 1 - ngrams involving <s> and </s> are duplicated using the
#     hidden segment boundary token <#s>.
# 2 - ngram starting with <s> are eliminated.
# 3 - the backoff weights of <s> is set to 1.
#     this together with the previous change sets all probabilities conditioned
#     on <s> to the respective marignal probabilities without <s>.
# 4 - ngrams ending in </s> get probability 1.
#     this avoid an end-of-sentence penalty in rescoring.
#
# $Header: /home/spot71/srilm/devel/utils/src/RCS/make-hiddens-lm,v 1.3 1996/09/15 17:28:34 stolcke Exp $
#
BEGIN {
	sent_start = "<s>";
	sent_end = "</s>";
	hiddens = "<#s>";
}
NF==0 {
	print; next;
}
/^ngram *[0-9][0-9]*=/ {
	print;
	next;
}
/^.[0-9]-grams:/ {
	currorder=substr($0,2,1);
}
/^\\/ {
	print; next;
}
# 
currorder && currorder < highorder {
	if (NF < currorder + 2) {
		print $0 "\t0";
	} else {
		print;
	}
	next;
}
$0 ~ sent_start || $0 ~ sent_end {
	oldline = $0;

	# modify sentence initial/final ngrams
	if ($2 == sent_start && currorder == 1) {
	    if (no_s_start) {
		# set <s> backoff weight to 1
		$3 = 0;
	    }
	    print;
	} else if ($2 == sent_start) {
	    # suppress other ngram starting with <s>
	    if (!no_s_start) {
		print;
	    }
	} else if ($(currorder + 1) == sent_end) {
	    if (no_s_end) {
		# set </s> prob to 1
		$1 = 0;
	    }
	    print;
	}

	# replace <s> and </s> with <#s> and output result
	gsub(sent_start, hiddens, oldline);
	gsub(sent_end, hiddens, oldline);
	print oldline;
	next;
}
{ print }
