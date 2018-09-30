/*
 * Map.cc --
 *	Map implementation
 *
 */

#ifndef lint
static char Map_Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char Map_RcsId[] = "@(#)$Header: /home/speech/stolcke/project/srilm/devel/dstruct/src/RCS/Map.cc,v 1.4 1996/05/30 18:53:03 stolcke Exp $";
#endif

#include "Map.h"

unsigned int _Map::initialSize = 10;
float _Map::growSize = 1.5;
Boolean _Map::foundP = false;	/* default last get() argument */

