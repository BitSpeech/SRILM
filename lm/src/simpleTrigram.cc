/*
 * Simple C interface to a trigram backoff model
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/simpleTrigram.cc,v 1.5 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;

#include "File.h"
#include "Vocab.h"
#include "Ngram.h"

static Vocab *vocab;
static Ngram *lm;

extern "C" {

void
trigram_init(char *filename) {

    vocab = new Vocab;
    assert(vocab != 0);

    lm = new Ngram(*vocab, 3);
    assert(lm != 0);

    File file(filename, "r");

    lm->debugme(1);

    if (!lm->read(file)) {
	cerr << "format error in lm file " << filename << endl;
	exit(1);
    }
}

int
word_id(char *w) {
     return vocab->getIndex(w);
}

double
trigram_logprob(unsigned w1, unsigned w2, unsigned w3) {
    VocabIndex context[3];

    context[0] = w2;
    context[1] = w1;
    context[2] = Vocab_None;

    return lm->wordProb(w3, context);
}

double
trigram_prob(unsigned w1, unsigned w2, unsigned w3) {

    return LogPtoProb(trigram_logprob(w1, w2, w3));
}

}
