#!/usr/local/bin/gawk -f
#
# wlat-to-dot --
#	Generate dot(1) graph description from word lattice generates by
#	nbest-lattice(1)
#
# usage: wlat-to-dot [show_probs=1] file.wlat > file.dot
#
# $Header: /home/srilm/devel/utils/src/RCS/wlat-to-dot,v 1.2 1998/07/10 05:55:33 stolcke Exp $
#
BEGIN {
	name = "WLAT"; 
	show_probs = 0;

	version = 1;

	print "digraph \"" name "\" {";
	print "rankdir = LR";
}
$1 == "initial" {
	i = $2;
}
$1 == "final" {
	i = $2;
}
$1 == "version" {
	version = $2;
}
$1 == "node" && version == 1 {
	from = $2;
	word = $3;
	post = $4;

	print "\tnode" from " [label=\"" word \
		(!show_probs ? "" : "\\n" post ) "\"]";

	for (i = 5; i <= NF; i ++) {
	    to = $i;
	    print "\tnode" from " -> node" to ";"
	}
}
$1 == "node" && version == 2 {
	from = $2;
	word = $3;
	align = $4;
	post = $5;

	print "\tnode" from " [label=\"" word "\\n" align \
		(!show_probs ? "" : "/" post ) "\"]";

	for (i = 6; i <= NF; i += 2) {
	    to = $i;
	    print "\tnode" from " -> node" to \
		(!show_probs ? "" : " [label=\"" $(i + 1) "\"]") ";"
	}
}
END {
	print "}"
}
