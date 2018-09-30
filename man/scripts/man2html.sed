#!/bin/sed -f
#
# $Header: /home/srilm/devel/man/scripts/RCS/man2html.sed,v 1.4 2000/01/20 02:05:11 stolcke Exp $
#

s,\\-,-,g
s,\\&,,g
s,\\\\,/BS/,g

# replace < ... >
s,&,\&amp;,g
s,<,\&lt;,g
s,>,\&gt;,g

# font changes
s,\\fB\([^\\]*\)\\fP,<B>\1</B>,g
s,\\fI\([^\\]*\)\\fP,<I>\1</I>,g

s,/BS/,\\,g

# crossrefs
s,\([A-Za-z][^ ]*\)([1-8]),<A HREF="\1.html">&</A>,g
s,^\.BR  *\([A-Za-z][^ ]*\)  *\(([1-8])\),<A HREF="\1.html">\1\2</A>,g

