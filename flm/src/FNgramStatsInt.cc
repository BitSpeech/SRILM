/*
 * FNgramStatsUnsigned.cc --
 *	Instantiation of FNgramCounts<unsigned>
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1996, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/flm/src/RCS/FNgramStatsInt.cc,v 1.5 2005/09/24 00:21:55 stolcke Exp $";
#endif

#ifndef EXCLUDE_CONTRIB

#include "FNgramStats.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_FNGRAMCOUNTS(FNgramCount);
#endif

#endif /* EXCLUDE_CONTRIB_END */
