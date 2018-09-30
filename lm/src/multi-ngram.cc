/*
 * multi-ngram --
 *	Assign probabilities of multiword N-gram models
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2000 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/multi-ngram.cc,v 1.3 2000/07/30 03:22:45 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>
#include <locale.h>
#include <assert.h>

#include "option.h"
#include "File.h"
#include "Vocab.h"
#include "Ngram.h"

#include "Array.cc"
#include "LHash.cc"

static unsigned order = defaultNgramOrder;
static unsigned multiOrder = defaultNgramOrder;
static unsigned debug = 0;
static char *vocabFile = 0;
static char *lmFile  = 0;
static char *multiLMfile  = 0;
static char *writeLM  = "-";
static char *multiChar = "_";

static Option options[] = {
    { OPT_UINT, "order", &order, "max ngram order" },
    { OPT_UINT, "multi-order", &multiOrder, "max multiword ngram order" },
    { OPT_UINT, "debug", &debug, "debugging level for lm" },
    { OPT_STRING, "vocab", &vocabFile, "(multiword) vocabulary to be added" },
    { OPT_STRING, "lm", &lmFile, "ngram LM to model" },
    { OPT_STRING, "multi-lm", &multiLMfile, "multiword ngram LM" },
    { OPT_STRING, "write-lm", &writeLM, "multiword ngram output file" },
    { OPT_STRING, "multi-char", &multiChar, "multiword component delimiter" },
};

/*
 * construct a mapping from multiword vocab indices to strings of indices
 * corresponding to their components
 */
LHash< VocabIndex, VocabIndex * > *
makeMultiwordMap(Vocab &vocab, Ngram &lm)
{
    LHash< VocabIndex, VocabIndex * > *map =
			new LHash< VocabIndex, VocabIndex * >;
    assert(map != 0);

    VocabIter viter(vocab);
    VocabString word;
    VocabIndex wid;


    while (word = viter.next(wid)) {
	static VocabIndex emptyContext[1] = { Vocab_None };

	if (strchr(word, *multiChar) && !lm.findProb(wid, emptyContext)) {
	    /*
	     * split multiword
	     */
	    char wordString[strlen(word) + 1];
	    VocabIndex widString[maxWordsPerLine + 1];

	    strcpy(wordString, word);

	    char *cp = strtok(wordString, multiChar);
	    unsigned numWords = 0;
	    do {
		assert(numWords <= maxWordsPerLine);

		widString[numWords] = vocab.getIndex(cp);

		if (widString[numWords] == Vocab_None) {
		    cerr << "warning: multiword component " << cp
			 << " not in vocab\n";
		    widString[numWords] = vocab.unkIndex;
		}

		numWords ++;
	    } while (cp = strtok((char *)0, multiChar));

	    widString[numWords] = Vocab_None;

	    VocabIndex *wids = new VocabIndex[numWords + 1];
	    assert(wids != 0);

	    Vocab::copy(wids, widString);

	    *(map->insert(wid)) = wids;
	}
    }
    return map;
}

/*
 * Expand a string of multiwords into components
 *	return length of expanded string
 */
unsigned
expandMultiwords(VocabIndex *words, VocabIndex *expanded,
	     LHash< VocabIndex, VocabIndex * > &map, Boolean reverse = false)
{
    unsigned j = 0;

    for (unsigned i = 0; words[i] != Vocab_None; i ++) {

	VocabIndex **comps = map.find(words[i]);

	if (comps == 0) {
	    assert(j <= maxWordsPerLine);
	    expanded[j] = words[i];
	    j ++;
	} else {
	    unsigned compsLength = Vocab::length(*comps);

	    for (unsigned k = 0; k < compsLength; k ++) {
		assert(j <= maxWordsPerLine);
		if (reverse) {
		    expanded[j] = (*comps)[compsLength - 1 - k];
		} else {
		    expanded[j] = (*comps)[k];
		}
		j ++;
	    }
	}
    }

    assert(j <= maxWordsPerLine);
    expanded[j] = Vocab_None;
    return j;
}

void
assignMultiProbs(Ngram &multiLM, Ngram &ngramLM,
					LHash< VocabIndex, VocabIndex * > &map)
{
    Vocab &vocab = multiLM.vocab;
    unsigned order = multiLM.setorder();

    for (unsigned i = 0; i < order; i++) {
	BOnode *node;
        VocabIndex context[order + 1];
	NgramBOsIter iter(multiLM, context, i);
	
	while (node = iter.next()) {

	    /*
	     * buffer holding expanded context, with room to prepend expanded
	     * word
	     */
	    VocabIndex expandedBuffer[2 * maxWordsPerLine + 1];

	    /*
	     * Expand the backoff context with all multiwords
	     */
	    VocabIndex *expandedContext = &expandedBuffer[maxWordsPerLine];
	    unsigned exandedContextLength =
		    expandMultiwords(context, expandedContext, map, true);

	    /*
	     * Find the corresponding context in the old LM
	     */
	    unsigned usedLength;
	    ngramLM.contextID(expandedContext, usedLength);
	    expandedContext[usedLength] = Vocab_None;

	    LogP *bow = ngramLM.findBOW(expandedContext);
	    assert(bow != 0);

	    /*
	     * Assign BOW from old LM to new LM context
	     */
	    node->bow = *bow;

	    NgramProbsIter piter(*node);
	    VocabIndex word;
	    LogP *multiProb;
		
	    while (multiProb = piter.next(word)) {

		VocabIndex multiWord[2];
		multiWord[0] = word;
		multiWord[1] = Vocab_None;

		VocabIndex expandedWord[maxWordsPerLine + 1];
		unsigned expandedWordLength =
			expandMultiwords(multiWord, expandedWord, map);

		LogP prob = LogP_One;
		for (unsigned j = 0; j < expandedWordLength; j ++) {
		    prob += ngramLM.wordProb(expandedWord[j],
					&expandedBuffer[maxWordsPerLine - j]);

		    expandedBuffer[maxWordsPerLine - 1 - j] = expandedWord[j];
		}

		/*
		 * Set new LM prob to aggregate old LM prob
		 */
		*multiProb = prob;
	    }
	}
    }
}

/* 
 * Populate multi-ngram LM with a superset of original ngrams.
 */
void
populateMultiNgrams(Ngram &multiLM, Ngram &ngramLM,
					LHash< VocabIndex, VocabIndex * > &map)
{
    Vocab &vocab = multiLM.vocab;
    unsigned order = ngramLM.setorder();
    unsigned multiOrder = multiLM.setorder();

    /*
     * don't exceed the multi-ngram order
     */
    if (order > multiOrder) {
	order = multiOrder;
    }

    /*
     * construct mapping to multiwords that start/end with given words
     */
    LHash<VocabIndex, Array<VocabIndex> > startMultiwords;
    LHash<VocabIndex, Array<VocabIndex> > endMultiwords;

    VocabIndex word;
    VocabIndex **expansion;
    LHashIter< VocabIndex, VocabIndex * > mapIter(map);
    while (expansion = mapIter.next(word)) {
	VocabIndex startWord = (*expansion)[0];
	VocabIndex endWord = (*expansion)[Vocab::length(*expansion) - 1];

	Array<VocabIndex> &startIndex = *startMultiwords.insert(startWord);
	Array<VocabIndex> &endIndex = *endMultiwords.insert(endWord);

	startIndex[startIndex.size()] = word;
	endIndex[endIndex.size()] = word;
    }

    /*
     * Populate multi-ngram LM
     */
    for (unsigned i = 0; i < order; i++) {
	BOnode *node;
	VocabIndex context[order + 1];
	NgramBOsIter iter(ngramLM, context, i);
	
	while (node = iter.next()) {
	    /*
	     * copy BOW to multi ngram
	     */
	    *multiLM.insertBOW(context) = node->bow;
		
	    /*
	     * copy probs to multi ngram
	     */
	    NgramProbsIter piter(*node);
	    VocabIndex word;
	    LogP *prob;

	    while (prob = piter.next(word)) {
		*multiLM.insertProb(word, context) = *prob;

		Array<VocabIndex> *startIndex = startMultiwords.find(word);
		if (startIndex) {
		    for (unsigned k = 0; k < startIndex->size(); k ++) {
			/*
			 * don't worry, the prob value will be reset later
			 */
			*multiLM.insertProb((*startIndex)[k], context) = *prob;
		    }
		}
	    }

	    Array<VocabIndex> *endIndex;
	    if (i > 0 && (endIndex = endMultiwords.find(context[i - 1]))) {
		VocabIndex oldWord = context[i - 1];

		for (unsigned j = 0; j < endIndex->size(); j ++) {
		    context[i - 1] = (*endIndex)[j];

		    /*
		     * repeat the procedure above for the new context
		     */
		    *multiLM.insertBOW(context) = node->bow;
			
		    /*
		     * copy probs to multi ngram
		     */
		    NgramProbsIter piter(*node);
		    VocabIndex word;
		    LogP *prob;

		    while (prob = piter.next(word)) {
			*multiLM.insertProb(word, context) = *prob;

			Array<VocabIndex> *startIndex =
						startMultiwords.find(word);
			if (startIndex) {
			    for (unsigned k = 0; k < startIndex->size(); k ++) {
				*multiLM.insertProb((*startIndex)[k], context) =
									*prob;
			    }
			}
		    }
		}

		/*
		 * restore old context
		 */
		context[i - 1] = oldWord;
	    }
	}
    }

    /*
     * Remove ngrams made redundant by multiwords
     */
    mapIter.init();
    while (expansion = mapIter.next(word)) {
	Vocab::reverse(*expansion);
	multiLM.removeProb((*expansion)[0], &(*expansion)[1]);
	Vocab::reverse(*expansion);
    }
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (!lmFile) {
	cerr << "-lm must be specified\n";
	exit(2);
    }
    
    /*
     * Construct language models
     */
    Vocab vocab;
    Ngram ngramLM(vocab, order);
    Ngram multiNgramLM(vocab, multiOrder);

    ngramLM.debugme(debug);
    multiNgramLM.debugme(debug);

    if (vocabFile) {
	File file(vocabFile, "r");

	if (!vocab.read(file)) {
	    cerr << "format error in vocab file\n";
	    exit(1);
	}
    }

    /*
     * Read LMs
     */
    {
	File file(lmFile, "r");

	if (!ngramLM.read(file)) {
	    cerr << "format error in lm file\n";
	    exit(1);
	}
    }

    if (multiLMfile) {
	File file(multiLMfile, "r");

	if (!multiNgramLM.read(file)) {
	    cerr << "format error in multi-lm file\n";
	    exit(1);
	}
    }

    /*
     * Construct mapping from multiwords to component words
     */
    LHash< VocabIndex, VocabIndex * > *map =
				makeMultiwordMap(vocab, ngramLM);

    /*
     * If a vocabulary was specified assume that we want to add new ngrams
     * containing multiwords.
     */
    if (vocabFile) {
	populateMultiNgrams(multiNgramLM, ngramLM, *map);
    }

    assignMultiProbs(multiNgramLM, ngramLM, *map);

    {
	File file(writeLM, "w");
	multiNgramLM.write(file);
    }

    exit(0);
}
