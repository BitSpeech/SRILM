#!/usr/local/bin/gawk -f
#
# get-gt-counts --
#	generate the counts-of-counts required for Good-Turing discounting
#	assumes the ngrams in the input contain no repetitions
#
# usage: get-gt-counts max=<number> out=<name> file ...
#
# $Header: /home/srilm/devel/utils/src/RCS/get-gt-counts.gawk,v 1.4 2002/07/17 00:38:40 stolcke Exp $
#
BEGIN {
	max = 10
	maxorder = 9;
}
{
	total[NF - 1] ++;
}
NF > 1 && $NF <= max {
	counts[(NF - 1), $NF] ++;
}
END {
	for (order = 1; order <= maxorder; order++) {
	    if (total[order] > 0) {
		if (out) {
		    outfile = out ".gt" order "counts";
		} else {
		    outfile = "/dev/stderr";
		}

		for (i = 0; i <= max; i ++) {
			c = counts[order, i];
			print i, c ? c : "0" > outfile;
		}
		print "total", total[order] > outfile;

		if (out) close(outfile);
	    }
	}
}
