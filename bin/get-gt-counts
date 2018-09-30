#!/usr/local/bin/gawk -f
#
# get-gt-counts --
#	generate the counts-of-counts required for Good-Turing discounting
#	assumes the ngrams in the input contain no repetitions
#
# usage: get-gt-counts max=<number> out=<name> file ...
#
# $Header: /home/srilm/devel/utils/src/RCS/get-gt-counts,v 1.3 1999/08/06 22:22:43 stolcke Exp $
#
BEGIN {
	max = 10
	maxorder = 6;
}
{
	total[NF - 1] ++;
}
$NF <= max {
	if (NF == 2)
	    counts1[$NF] ++;
	else if (NF == 3)
	    counts2[$NF] ++;
	else if (NF == 4)
	    counts3[$NF] ++;
	else if (NF == 5)
	    counts4[$NF] ++;
	else if (NF == 6)
	    counts5[$NF] ++;
	else if (NF == 7)
	    counts6[$NF] ++;
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
			c = (order == 1 ? counts1[i] :
			     order == 2 ? counts2[i] :
			     order == 3 ? counts3[i] :
			     order == 4 ? counts4[i] :
			     order == 5 ? counts5[i] :
			                  counts6[i]);
			print i, c ? c : "0" > outfile;
		}
		print "total", total[order] > outfile;

		if (out) close(outfile);
	    }
	}
}
