#!/usr/local/bin/gawk -f
#
# reverse-ngram-counts --
#	Reverse the word order in N-gram count files
#
# $Header: /home/srilm/devel/utils/src/RCS/reverse-ngram-counts,v 1.1 2000/06/22 20:38:41 stolcke Exp $
#
{
	i = 1;
	j = NF - 1;
	while (i < j) {
		h = $i;
		$i = $j;
		$j = h; 
		i ++; j--;
	}
	print;
}
