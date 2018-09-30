#!/bin/sed -f
#
# $Header: /home/speech/stolcke/project/srilm/devel/man/scripts/RCS/man2html.sed,v 1.3 1996/07/13 01:34:30 stolcke Exp $
#

s,\\-,-,g
s,\\&,,g

# replace < ... >
s,&,\&amp;,g
s,<,\&lt;,g
s,>,\&gt;,g

# font changes
s,\\fB\([^\\]*\)\\fP,<B>\1</B>,g
s,\\fI\([^\\]*\)\\fP,<I>\1</I>,g

# crossrefs
s,\([A-Za-z][^ ]*\)([1-8]),<A HREF="\1.html">&</A>,g

