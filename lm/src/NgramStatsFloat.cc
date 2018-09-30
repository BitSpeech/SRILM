/*
 * NgramStatsFloat.cc --
 *	Instantiation of NgramCounts<double>
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1996, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/spot71/srilm/devel/lm/src/RCS/NgramStatsFloat.cc,v 1.3 1996/09/08 21:06:34 stolcke Exp $";
#endif

#include "NgramStats.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_NGRAMCOUNTS(double);
#endif

