#!/usr/local/bin/gawk -f
#
# bytelog-to-log10 --
#	convert bytelog scores to log-base-10
#
# $Header: /home/spot71/srilm/devel/utils/src/RCS/bytelog-to-log10,v 1.1 1997/04/22 20:20:41 stolcke Exp $
#
BEGIN {
	logscale = 2.30258509299404568402 * 10000.5 / 1024.0;
	scale = 1;
}
{
	for (i = 1; i <= NF; i ++) {
	    if ($i ~ /^[-+0-9][0-9]*$/) {
		    $i = $i / scale / logscale;
	    }
	}
	print;
}
