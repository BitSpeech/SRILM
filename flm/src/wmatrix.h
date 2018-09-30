/*
 * wmatrix.h
 *
 * Jeff Bilmes  <bilmes@ee.washington.edu>
 * 
 * @(#)$Header: /home/srilm/devel/flm/src/RCS/wmatrix.h,v 1.5 2006/01/05 20:21:27 stolcke Exp $
 *
 */

#ifndef _WMatrix_h_
#define _WMatrix_h_

#ifndef EXCLUDE_CONTRIB

#include <iostream>
using namespace std;
#include <stdio.h>

#include "Trie.h"
#include "Array.h"

#include "LMStats.h"
#include "FactoredVocab.h"
#include "FNgramSpecs.h"

const unsigned maxNumFactors = maxWordsPerLine + maxExtraWordsPerLine;

// matrices for storing factors in parsed string format and wid format
class WidMatrix {
public:
  WidMatrix();
  ~WidMatrix();
  VocabIndex* wid_factors[maxNumFactors];
  VocabIndex* operator[](int i) { return wid_factors[i]; }
};


class WordMatrix {
public:
  WordMatrix();
  ~WordMatrix();
  VocabString* word_factors[maxNumFactors];
  VocabString* operator[](int i) { return word_factors[i]; }
  void print(FILE* f);
};

#endif /* EXCLUDE_CONTRIB_END */

#endif
