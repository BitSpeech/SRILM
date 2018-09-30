/*
 * wmatrix.cc
 *
 * Jeff Bilmes <bilmes@ee.washington.edu>
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2003-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/flm/src/RCS/wmatrix.cc,v 1.6 2006/01/09 18:33:31 stolcke Exp $";
#endif

#ifndef EXCLUDE_CONTRIB

#include <iostream>
using namespace std;
#include <string.h>
#include <ctype.h>

#include "FNgramSpecs.h"

#include "Trie.cc"
#include "Array.cc"
#include "FactoredVocab.h"
#include "Debug.h"
#include "FDiscount.h"
#include "hexdec.h"


WidMatrix::WidMatrix()
{
  for (int i=0;i<maxNumFactors;i++) {
    wid_factors[i] = new VocabIndex[maxNumParentsPerChild+1];
  }
}

WidMatrix::~WidMatrix()
{
  for (int i=0;i<maxNumFactors;i++) {
    delete [] wid_factors[i];
  }
}

WordMatrix::WordMatrix()
{
  for (int i=0;i<maxNumFactors;i++) {
    word_factors[i] = new VocabString[maxNumParentsPerChild+1];
  }
}

WordMatrix::~WordMatrix()
{
  for (int i=0;i<maxNumFactors;i++) {
    delete [] word_factors[i];
  }
}

void
WordMatrix::print(FILE* f)
{
  for (int i=0;word_factors[i][FNGRAM_WORD_TAG_POS] != 0;i++) {
    fprintf(f,"%d ",i);
    for (int j=0;word_factors[i][j] != 0;j++) {
      fprintf(f,"%s ",word_factors[i][j]);
    }
    fprintf(f,"\n");
  }
}

#endif /* EXCLUDE_CONTRIB_END */

