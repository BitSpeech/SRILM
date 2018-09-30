/*
 * hidden-ngram --
 *	Recover hidden word-level events using a hidden n-gram model
 *	(derived from disambig.cc)
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Id: hidden-ngram.cc,v 1.45 2006/01/05 20:21:27 stolcke Exp $";
#endif

#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>

#include "option.h"
#include "version.h"
#include "File.h"
#include "Vocab.h"
#include "VocabMap.h"
#include "SubVocab.h"
#include "NullLM.h"
#include "Ngram.h"
#include "ClassNgram.h"
#include "SimpleClassNgram.h"
#include "ProductNgram.h"
#include "BayesMix.h"
#include "NgramStats.h"
#include "Trellis.cc"
#include "LHash.cc"
#include "Array.cc"

typedef const VocabIndex *VocabContext;

#define DEBUG_ZEROPROBS		1
#define DEBUG_TRANSITIONS	2
#define DEBUG_TRELLIS	        3

static int version = 0;
static unsigned numNbest = 1;
static unsigned order = 3;
static unsigned debug = 0;
static char *lmFile = 0;
static char *classesFile = 0;
static int simpleClasses = 0;
static int factored = 0;
static char *mixFile  = 0;
static char *mixFile2 = 0;
static char *mixFile3 = 0;
static char *mixFile4 = 0;
static char *mixFile5 = 0;
static char *mixFile6 = 0;
static char *mixFile7 = 0;
static char *mixFile8 = 0;
static char *mixFile9 = 0;
static double mixLambda = 0.5;
static double mixLambda2 = 0.0;
static double mixLambda3 = 0.0;
static double mixLambda4 = 0.0;
static double mixLambda5 = 0.0;
static double mixLambda6 = 0.0;
static double mixLambda7 = 0.0;
static double mixLambda8 = 0.0;
static double mixLambda9 = 0.0;
static char *hiddenVocabFile = 0;
static char *textFile = 0;
static char *textMapFile = 0;
static char *escape = 0;
static int keepUnk = 0;
static int keepnull = 1;
static int toLower = 0;
static double lmw = 1.0;
static double mapw = 1.0;
static int logMap = 0;
static int fb = 0;
static int fwOnly = 0;
static int posteriors = 0;
static int totals = 0;
static int continuous = 0;
static int forceEvent = 0;
static char *countsFile = 0;

typedef double NgramFractCount;			/* type for posterior counts */

const LogP LogP_PseudoZero = -100;

static double unkProb = LogP_PseudoZero;

const VocabString noHiddenEvent = "*noevent*";
VocabIndex noEventIndex;

static Option options[] = {
    { OPT_TRUE, "version", &version, "print version information" },
    { OPT_STRING, "lm", &lmFile, "hidden token sequence model" },
    { OPT_STRING, "classes", &classesFile, "class definitions" },
    { OPT_TRUE, "simple-classes", &simpleClasses, "use unique class model" },
    { OPT_TRUE, "factored", &factored, "use a factored LM" },
    { OPT_STRING, "mix-lm", &mixFile, "LM to mix in" },
    { OPT_FLOAT, "lambda", &mixLambda, "mixture weight for -lm" },
    { OPT_STRING, "mix-lm2", &mixFile2, "second LM to mix in" },
    { OPT_FLOAT, "mix-lambda2", &mixLambda2, "mixture weight for -mix-lm2" },
    { OPT_STRING, "mix-lm3", &mixFile3, "third LM to mix in" },
    { OPT_FLOAT, "mix-lambda3", &mixLambda3, "mixture weight for -mix-lm3" },
    { OPT_STRING, "mix-lm4", &mixFile4, "fourth LM to mix in" },
    { OPT_FLOAT, "mix-lambda4", &mixLambda4, "mixture weight for -mix-lm4" },
    { OPT_STRING, "mix-lm5", &mixFile5, "fifth LM to mix in" },
    { OPT_FLOAT, "mix-lambda5", &mixLambda5, "mixture weight for -mix-lm5" },
    { OPT_STRING, "mix-lm6", &mixFile6, "sixth LM to mix in" },
    { OPT_FLOAT, "mix-lambda6", &mixLambda6, "mixture weight for -mix-lm6" },
    { OPT_STRING, "mix-lm7", &mixFile7, "seventh LM to mix in" },
    { OPT_FLOAT, "mix-lambda7", &mixLambda7, "mixture weight for -mix-lm7" },
    { OPT_STRING, "mix-lm8", &mixFile8, "eighth LM to mix in" },
    { OPT_FLOAT, "mix-lambda8", &mixLambda8, "mixture weight for -mix-lm8" },
    { OPT_STRING, "mix-lm9", &mixFile9, "ninth LM to mix in" },
    { OPT_FLOAT, "mix-lambda9", &mixLambda9, "mixture weight for -mix-lm9" },
    { OPT_UINT, "nbest", &numNbest, "number of nbest hypotheses to generate in Viterbi search" },
    { OPT_UINT, "order", &order, "ngram order to use for lm" },
    { OPT_STRING, "hidden-vocab", &hiddenVocabFile, "hidden vocabulary" },
    { OPT_TRUE, "keep-unk", &keepUnk, "preserve unknown words" },
    { OPT_FALSE, "nonull", &keepnull, "remove <NULL> in factored LM" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_STRING, "text", &textFile, "text file to disambiguate" },
    { OPT_STRING, "text-map", &textMapFile, "text+map file to disambiguate" },
    { OPT_FLOAT, "lmw", &lmw, "language model weight" },
    { OPT_FLOAT, "mapw", &mapw, "map log likelihood weight" },
    { OPT_TRUE, "logmap", &logMap, "map file contains log probabilities" },
    { OPT_STRING, "escape", &escape, "escape prefix to pass data through -text" },
    { OPT_TRUE, "fb", &fb, "use forward-backward algorithm" },
    { OPT_TRUE, "fw-only", &fwOnly, "use only forward probabilities" },
    { OPT_TRUE, "posteriors", &posteriors, "output posterior event probabilities" },
    { OPT_TRUE, "totals", &totals, "output total string probabilities" },
    { OPT_TRUE, "continuous", &continuous, "read input without line breaks" },
    { OPT_FLOAT, "unk-prob", &unkProb, "log probability assigned to <unk>" },
    { OPT_TRUE, "force-event", &forceEvent, "disallow default event" },
    { OPT_STRING, "write-counts", &countsFile, "write posterior counts to file" },
    { OPT_UINT, "debug", &debug, "debugging level for lm" },
};

/*
 * Increment posterior counts for all prefixes of a context
 * Parameters:
 *		wid	current word
 *		context preceding N-gram
 *		prevWid	word preceding context (Vocab_None if not available)
 */
void
incrementCounts(NgramCounts<NgramFractCount> &counts, VocabIndex wid,
			    VocabIndex prevWid,
			    const VocabIndex *context, NgramFractCount factor)
{
    unsigned clen = Vocab::length(context);

    VocabIndex reversed[maxNgramOrder + 2];
    assert(clen < maxNgramOrder);

    unsigned i;
    reversed[0] = prevWid;
    for (i = 0; i < clen; i ++) {
	reversed[i + 1] = context[clen - 1 - i];

	/*
	 * Don't count contexts ending in *noevent*
	 */
	if (reversed[i + 1] == noEventIndex) {
		break;
	}
    }

    /*
     * Increment counts of context and its suffixes.
     * Note: this is skipped if last element of context is *noevent*.
     */
    if (i == clen) {
	reversed[i + 1] = Vocab_None;

	for (unsigned j = (prevWid == Vocab_None ? 1 : 0); j <= clen; j ++) {
	    *counts.insertCount(&reversed[j]) += factor;
	}
    }

    if (wid != Vocab_None) {
	reversed[i + 1] = wid;
	reversed[i + 2] = Vocab_None;

	for (unsigned j = 0; j <= clen; j ++) {
	    *counts.insertCount(&reversed[j + 1]) += factor;
	}
    }
}

/*
 * Find the word preceding context
 */
VocabIndex
findPrevWord(unsigned pos, const VocabIndex *wids, const VocabIndex *context)
{
    unsigned i;
    unsigned j;

    /*
     * Match words in context against wids starting at current position
     */
    for (i = pos, j = 0; context[j] != Vocab_None; j ++) {
	if (i > 0 && wids[i - 1] == context[j]) {
	    i --;
	}
    }

    if (i > 0 && j > 0 && context[j - 1] != wids[i]) {
	return wids[i - 1];
    } else {
	return Vocab_None;
    }
}

/*
 * Disambiguate a sentence by finding the hidden tag sequence compatible with
 * it that has the highest probability
 */
unsigned
disambiguateSentence(VocabIndex *wids, VocabIndex *hiddenWids[],
			LogP totalProb[], PosVocabMap &map, LM &lm,
			NgramCounts<NgramFractCount> *hiddenCounts,
			unsigned numNbest, Boolean positionMapped = false)
{

    unsigned len = Vocab::length(wids);

    Trellis<VocabContext> trellis(len + 1, numNbest);

    /*
     * Prime the trellis with empty context as the start state
     */
    static VocabIndex emptyContext[] = { Vocab_None };
    trellis.setProb(emptyContext, LogP_One);

    unsigned pos = 0;
    while (wids[pos] != Vocab_None) {

	trellis.step();

	/*
	 * Set up new context to transition to
	 * (allow enough room for one hidden event per word)
	 */
	VocabIndex newContext[2 * maxWordsPerLine + 3];

	/*
	 * Iterate over all contexts for the previous position in trellis
	 */
	TrellisIter<VocabContext> prevIter(trellis, pos);
	VocabContext prevContext;
	LogP prevProb;

	while (prevIter.next(prevContext, prevProb)) {

	    /*
	     * A non-event token in the previous context is skipped for
	     * purposes of LM lookup
	     */
	    VocabContext usedPrevContext =
					(prevContext[0] == noEventIndex) ?
						&prevContext[1] : prevContext;

	    /*
	     * transition prob out of previous context to current word;
	     * skip probability of first word, since it's a constant offset
	     * for all states and usually infinity if the first word is <s>
	     */
	    LogP wordProb = (pos == 0) ?
			LogP_One : lm.wordProb(wids[pos], usedPrevContext);

	    if (wordProb == LogP_Zero) {
		/*
		 * Zero probability due to an OOV
		 * would equalize the probabilities of all paths
		 * and make the Viterbi backtrace useless.
		 */
		wordProb = unkProb;
	    }

	    /*
	     * Set up the extended context.  Allow room for adding 
	     * the current word and a new hidden event.
	     */

	    unsigned i;
	    for (i = 0; i < 2 * maxWordsPerLine && 
				    usedPrevContext[i] != Vocab_None; i ++)
	    {
		newContext[i + 2] = usedPrevContext[i];
	    }

	    newContext[i + 2] = Vocab_None;
	    newContext[1] = wids[pos];	/* prepend current word */

	    /*
	     * Iterate over all hidden events
	     */
	    VocabMapIter currIter(map, positionMapped ? pos : 0);
	    VocabIndex currEvent;
	    Prob currProb;

	    while (currIter.next(currEvent, currProb)) {
		LogP localProb = logMap ? currProb : ProbToLogP(currProb);

		/*
		 * Prepend current event to context.
		 * Note: noEvent is represented here (but no earlier in 
		 * the context).
		 */ 
		newContext[0] = currEvent;	

		/*
		 * Event probability
		 */
		LogP eventProb = (currEvent == noEventIndex) ? LogP_One
				    : lm.wordProb(currEvent, &newContext[1]);

		/*
		 * Truncate context to what is actually used by LM,
		 * but keep at least one word so we can recover words later.
		 */
		VocabIndex *usedNewContext = 
			    (currEvent == noEventIndex) ? &newContext[1]
					: newContext;

		unsigned usedLength;
		lm.contextID(usedNewContext, usedLength);
		if (usedLength == 0) {
		    usedLength = 1;
		}

		VocabIndex truncatedContextWord = usedNewContext[usedLength];
		usedNewContext[usedLength] = Vocab_None;

		if (debug >= DEBUG_TRANSITIONS) {
		    cerr << "POSITION = " << pos
			 << " FROM: " << (lm.vocab.use(), prevContext)
			 << " TO: " << (lm.vocab.use(), newContext)
		         << " WORD = " << lm.vocab.getWord(wids[pos])
			 << " WORDPROB = " << wordProb
			 << " EVENTPROB = " << eventProb
			 << " LOCALPROB = " << localProb
			 << endl;
		}

		trellis.update(prevContext, newContext,
			       weightLogP(lmw, wordProb + eventProb) +
			       weightLogP(mapw, localProb));

		/*
		 * Restore newContext
		 */
		usedNewContext[usedLength] = truncatedContextWord;
	    }
	}

	pos ++;
    }

    if (debug >= DEBUG_TRELLIS) {
	cerr << "Trellis:\n" << trellis << endl;
    }

    /*
     * Check whether viterbi-nbest (default) is asked for.  If so,
     * do it and return. Otherwise do forward-backward.
     */
    if (!fb && !fwOnly && !posteriors && !hiddenCounts) {
	//VocabContext hiddenContexts[numNbest][len + 2];	  
	makeArray(VocabContext *, hiddenContexts, numNbest);

	unsigned n;
	for (n = 0; n < numNbest; n ++) {
	    hiddenContexts[n] = new VocabContext[len + 2];
						// One extra state for initial.
	    assert(hiddenContexts[n]);

	    if (trellis.nbest_viterbi(hiddenContexts[n], len + 1,
						    n, totalProb[n]) != len + 1)
	    {
		return n;
	    }
	      
	    /*
	     * Transfer the first word of each state (word context) into the
	     * word index string
	     */
	    for (unsigned i = 0; i < len; i ++) {
		hiddenWids[n][i] = hiddenContexts[n][i + 1][0];
	    }
	    hiddenWids[n][len] = Vocab_None;
	}

	for (n = 0; n < numNbest; n ++) {
	    delete [] hiddenContexts[n];
	}

	return numNbest;
    } else {
	/*
	 * Viterbi-nbest wasn't asked for.  We run the Forward-backward
	 * algorithm: compute backward scores, but only get the 1st best.
	 */
	trellis.initBack(pos);

	/* 
	 * Initialize backward probabilities
	 */
	{
	    TrellisIter<VocabContext> iter(trellis, pos);
	    VocabContext context;
	    LogP prob;
	    while (iter.next(context, prob)) {
		trellis.setBackProb(context, LogP_One);
	    }
	}

	while (pos > 0) {
	    trellis.stepBack();

	    /*
	     * Set up new context to transition to
	     * (allow enough room for one hidden event per word)
	     */
	    VocabIndex newContext[2 * maxWordsPerLine + 3];

	    /*
	     * Iterate over all contexts for the previous position in trellis
	     */
	    TrellisIter<VocabContext> prevIter(trellis, pos - 1);
	    VocabContext prevContext;
	    LogP prevProb;

	    while (prevIter.next(prevContext, prevProb)) {

		/*
		 * A non-event token in the previous context is skipped for
		 * purposes of LM lookup
		 */
		VocabContext usedPrevContext =
					(prevContext[0] == noEventIndex) ?
						&prevContext[1] : prevContext;

		/*
		 * transition prob out of previous context to current word;
		 * skip probability of first word, since it's a constant offset
		 * for all states and usually infinity if the first word is <s>
		 */
		LogP wordProb = (pos == 1) ?
			LogP_One : lm.wordProb(wids[pos-1], usedPrevContext);

		if (wordProb == LogP_Zero) {
		    /*
		     * Zero probability due to an OOV
		     * would equalize the probabilities of all paths
		     * and make the Viterbi backtrace useless.
		     */
		    wordProb = unkProb;
		}

		/*
		 * Set up the extended context.  Allow room for adding 
		 * the current word and a new hidden event.
		 */
		unsigned i;
		for (i = 0; i < 2 * maxWordsPerLine && 
					usedPrevContext[i] != Vocab_None; i ++)
		{
		    newContext[i + 2] = usedPrevContext[i];
		}
		newContext[i + 2] = Vocab_None;
		newContext[1] = wids[pos-1];	/* prepend current word */

		/*
		 * Iterate over all hidden events
		 */
		VocabMapIter currIter(map, positionMapped ? pos - 1 : 0);
		VocabIndex currEvent;
		Prob currProb;

		while (currIter.next(currEvent, currProb)) {
		    LogP localProb = logMap ? currProb : ProbToLogP(currProb);

		    /*
		     * Prepend current event to context.
		     * Note: noEvent is represented here (but no earlier in 
		     * the context).
		     */ 
		    newContext[0] = currEvent;	

		    /*
		     * Event probability
		     */
		    LogP eventProb = (currEvent == noEventIndex) ? LogP_One
				    : lm.wordProb(currEvent, &newContext[1]);

		    /*
		     * Truncate context to what is actually used by LM,
		     * but keep at least one word so we can recover words later.
		     */
		    VocabIndex *usedNewContext = 
				(currEvent == noEventIndex) ? &newContext[1]
					    : newContext;

		    unsigned usedLength;
		    lm.contextID(usedNewContext, usedLength);
		    if (usedLength == 0) {
			usedLength = 1;
		    }
		    VocabIndex truncatedContextWord =
						usedNewContext[usedLength];
		    usedNewContext[usedLength] = Vocab_None;

		    if (debug >= DEBUG_TRANSITIONS) {
			cerr << "BACKWARD POSITION = " << pos
			     << " FROM: " << (lm.vocab.use(), prevContext)
			     << " TO: " << (lm.vocab.use(), newContext)
			     << " WORD = " << lm.vocab.getWord(wids[pos - 1])
			     << " WORDPROB = " << wordProb
			     << " EVENTPROB = " << eventProb
			     << " LOCALPROB = " << localProb
			     << endl;
		    }

		    trellis.updateBack(prevContext, newContext,
				       weightLogP(lmw, wordProb + eventProb) +
				       weightLogP(mapw, localProb));

		    /*
		     * Restore newContext
		     */
		    usedNewContext[usedLength] = truncatedContextWord;
		}
	    }
	    pos --;
	}

	if (hiddenCounts) {
	    incrementCounts(*hiddenCounts, wids[0], Vocab_None,
							emptyContext, 1.0);
	}

	/*
	 * Compute posterior symbol probabilities and extract most
	 * probable symbol for each position
	 */
	for (pos = 1; pos <= len; pos ++) {
	    /*
	     * Compute symbol probabilities by summing posterior probs
	     * of all states corresponding to the same symbol
	     */
	    LHash<VocabIndex,LogP2> symbolProbs;

	    TrellisIter<VocabContext> iter(trellis, pos);
	    VocabContext context;
	    LogP forwProb;
	    while (iter.next(context, forwProb)) {
		LogP2 posterior;
		
		if (fwOnly) {
		    posterior = forwProb;
		} else if (fb) {
		    posterior = forwProb + trellis.getBackLogP(context, pos);
		} else {
		    LogP backmax;
		    posterior = trellis.getMax(context, pos, backmax);
		    posterior += backmax;
		}

		Boolean foundP;
		LogP2 *symbolProb = symbolProbs.insert(context[0], foundP);
		if (!foundP) {
		    *symbolProb = posterior;
		} else {
		    *symbolProb = AddLogP(*symbolProb, posterior);
		}
	    }

	    /*
	     * Find symbol with highest posterior
	     */
	    LogP2 totalPosterior = LogP_Zero;
	    LogP2 maxPosterior = LogP_Zero;
	    VocabIndex bestSymbol = Vocab_None;

	    LHashIter<VocabIndex,LogP2> symbolIter(symbolProbs);
	    LogP2 *symbolProb;
	    VocabIndex symbol;
	    while (symbolProb = symbolIter.next(symbol)) {
		if (bestSymbol == Vocab_None || *symbolProb > maxPosterior) {
		    bestSymbol = symbol;
		    maxPosterior = *symbolProb;
		}
		totalPosterior = AddLogP(totalPosterior, *symbolProb);
	    }

	    if (bestSymbol == Vocab_None) {
		cerr << "no forward-backward state for position "
		     << pos << endl;
		return 0;
	    }

	    hiddenWids[0][pos - 1] = bestSymbol;

	    /*
	     * Print posterior probabilities
	     */
	    if (posteriors) {
		cout << lm.vocab.getWord(wids[pos - 1]) << "\t";

		/*
		 * Print events in sorted order
		 */
		VocabIter symbolIter(map.vocab2, true);
		VocabString symbolName;
		while (symbolName = symbolIter.next(symbol)) {
		    LogP2 *symbolProb = symbolProbs.find(symbol);
		    if (symbolProb != 0) {
			LogP2 posterior = *symbolProb - totalPosterior;

			cout << " " << symbolName
			     << " " << LogPtoProb(posterior);
		    }
		}
		cout << endl;
	    }

	    /*
	     * Accumulate hidden posterior counts, if requested
	     */
	    if (hiddenCounts) {
		iter.init();

		while (iter.next(context, forwProb)) {
		    LogP2 posterior;
		    
		    if (fwOnly) {
			posterior = forwProb;
		    } else if (fb) {
			posterior = forwProb +
					trellis.getBackLogP(context, pos);
		    } else {
			LogP backmax;
			posterior = trellis.getMax(context, pos, backmax);
			posterior += backmax;
		    }

		    posterior -= totalPosterior;	/* normalize */

		    incrementCounts(*hiddenCounts, wids[pos],
					findPrevWord(pos, wids, context),
					context, LogPtoProb(posterior));
		}
	    }
	}

	/*
	 * Return total string probability summing over all paths
	 */
	totalProb[0] = trellis.sumLogP(len);
	hiddenWids[0][len] = Vocab_None;
	return 1;
    }
}

/*
 * create dummy PosVocabMap to enumerate hiddenVocab
 */
void
makeDummyMap(PosVocabMap &map, Vocab &hiddenVocab)
{
    VocabIter viter(hiddenVocab);
    VocabIndex event;
    while (viter.next(event)) {
	map.put(0, event, logMap ? LogP_One : 1.0);
    }
}

/*
 * Get one input sentences at a time, map it to wids, 
 * disambiguate it, and print out the result
 */
void
disambiguateFile(File &file, SubVocab &hiddenVocab, LM &lm,
		 NgramCounts<NgramFractCount> *hiddenCounts, unsigned numNbest)
{
    PosVocabMap dummyMap(hiddenVocab);
    makeDummyMap(dummyMap, hiddenVocab);

    char *line;
    VocabString sentence[maxWordsPerLine];
    unsigned escapeLen = escape ? strlen(escape) : 0;

    while (line = file.getline()) {

	/*
	 * Pass escaped lines through unprocessed
	 */
        if (escape && strncmp(line, escape, escapeLen) == 0) {
	    cout << line;
	    continue;
	}

	unsigned numWords = Vocab::parseWords(line, sentence, maxWordsPerLine);
	if (numWords == maxWordsPerLine) {
	    file.position() << "too many words per sentence\n";
	} else {
	    makeArray(VocabIndex, wids, maxWordsPerLine + 1);
	    makeArray(VocabIndex *, hiddenWids, numNbest);

	    for (unsigned n = 0; n < numNbest; n++) {
		hiddenWids[n] = new VocabIndex[maxWordsPerLine + 1];
		assert(hiddenWids[n] != 0);
	    }

	    lm.vocab.getIndices(sentence, wids, maxWordsPerLine,
							  lm.vocab.unkIndex());

	    makeArray(LogP, totalProb, numNbest);
	    unsigned numHyps = disambiguateSentence(wids, hiddenWids, totalProb,
						    dummyMap, lm, hiddenCounts,
						    numNbest);
	    if (!numHyps) {
		file.position() << "Disambiguation failed\n";
	    } else if (totals) {
		cout << totalProb[0] << endl;
	    } else if (!posteriors) {
		for (unsigned n = 0; n < numHyps; n++) {
		    if (numNbest > 1) {
			cout << "NBEST_" << n << " " << totalProb[n] << " ";
		    }
		    for (unsigned i = 0; hiddenWids[n][i] != Vocab_None; i ++) {
			cout << (i > 0 ? " " : "")
			     << (keepUnk ? sentence[i] :
						lm.vocab.getWord(wids[i]));

			if (hiddenWids[n][i] != noEventIndex) {
			    cout << " " << lm.vocab.getWord(hiddenWids[n][i]);
			}
		    }
		    cout << endl;
		}
	    }
	    for (unsigned n = 0; n < numNbest; n++) {
		delete [] hiddenWids[n];
	    }
	}
    }
}

/*
 * Read entire file ignoring line breaks, map it to wids, 
 * disambiguate it, and print out the result
 */
void
disambiguateFileContinuous(File &file, SubVocab &hiddenVocab, LM &lm,
			   NgramCounts<NgramFractCount> *hiddenCounts,
			   unsigned numNbest)
{
    PosVocabMap dummyMap(hiddenVocab);
    makeDummyMap(dummyMap, hiddenVocab);

    char *line;
    Array<VocabIndex> wids;

    unsigned escapeLen = escape ? strlen(escape) : 0;
    unsigned lineStart = 0; // index into the above to mark the offset for the 
			    // current line's data


    while (line = file.getline()) {
	/*
	 * Pass escaped lines through unprocessed
	 * (although this is pretty useless in continuous mode)
	 */
        if (escape && strncmp(line, escape, escapeLen) == 0) {
	    cout << line;
	    continue;
	}

	VocabString words[maxWordsPerLine];
	unsigned numWords =
		Vocab::parseWords(line, words, maxWordsPerLine);

	if (numWords == maxWordsPerLine) {
	    file.position() << "too many words per line\n";
	} else {
	    // This effectively allocates more space
	    wids[lineStart + numWords] = Vocab_None;

	    lm.vocab.getIndices(words, &wids[lineStart], numWords,
							lm.vocab.unkIndex());
	    lineStart += numWords;
	}
    }

    if (lineStart == 0) {
	// empty input -- nothing to do
	return;
    }

    makeArray(VocabIndex *, hiddenWids, numNbest);
    makeArray(LogP, totalProb, numNbest);

    for (unsigned n = 0; n < numNbest; n++) {
	hiddenWids[n] = new VocabIndex[lineStart + 1];
	assert(hiddenWids[n] != 0);
    }

    unsigned numHyps =
		disambiguateSentence(&wids[0], hiddenWids, totalProb,
				     dummyMap, lm, hiddenCounts, numNbest);

    if (!numHyps) {
	file.position() << "Disambiguation failed\n";
    } else if (totals) {
	cout << totalProb << endl;
    } else if (!posteriors) {
	for (unsigned n = 0; n < numHyps; n++) {
	    if (numNbest > 1) {
		cout << "NBEST_" << n << " " << totalProb[n] << " ";
	    }
	    for (unsigned i = 0; hiddenWids[n][i] != Vocab_None; i ++) {
		// XXX: keepUnk not implemented yet.
		cout << lm.vocab.getWord(wids[i]) << " ";

		if (hiddenWids[n][i] != noEventIndex) {
		    cout << lm.vocab.getWord(hiddenWids[n][i]) << " ";
		}
	    }
	    cout << endl;
	}
    }
    for (unsigned n = 0; n < numNbest; n++) {
	delete [] hiddenWids[n];
    }
}

/*
 * Read a combined text+map file,
 * disambiguate it, and print out the result
 */
void
disambiguateTextMap(File &file, SubVocab &hiddenVocab, LM &lm,
		    NgramCounts<NgramFractCount> *hiddenCounts,
		    unsigned numNbest)
{
    char *line;

    unsigned escapeLen = escape ? strlen(escape) : 0;

    while (line = file.getline()) {

	/*
	 * Hack alert! We pass the map entries associated with the word
	 * instances in a VocabMap, but we encode the word position (not
	 * its identity) as the first VocabIndex.
	 */
	PosVocabMap map(hiddenVocab);

	unsigned numWords = 0;
	Array<VocabIndex> wids;

	/*
	 * Process one sentence at a time
	 */
	Boolean haveEscape = false;

	do {
	    /*
	     * Pass escaped lines through unprocessed
	     * We also terminate an "sentence" whenever an escape line is found,
	     * but printing the escaped line has to be deferred until we're
	     * done processing the sentence.
	     */
	    if (escape && strncmp(line, escape, escapeLen) == 0) {
		haveEscape = true;
		break;
	    }

	    /*
	     * Read map line
	     */
	    VocabString mapFields[maxWordsPerLine];

	    unsigned howmany =
			Vocab::parseWords(line, mapFields, maxWordsPerLine);

	    if (howmany == maxWordsPerLine) {
		file.position() << "text map line has too many fields\n";
		return;
	    }

	    /*
	     * First field is the observed word
	     */
	    wids[numWords] =
			lm.vocab.getIndex(mapFields[0], lm.vocab.unkIndex());
	    
	    /*
	     * Parse the remaining words as either probs or hidden events
	     */
	    unsigned i = 1;

	    while (i < howmany) {
		double prob;

		/*
		 * Use addWord here so new event names are added as needed
		 * (this means the -hidden-vocab option become optional).
		 */
		VocabIndex w2 = hiddenVocab.addWord(mapFields[i++]);

		if (i < howmany && sscanf(mapFields[i], "%lf", &prob)) {
		    i ++;
		} else {
		    prob = logMap ? LogP_One : 1.0;
		}

		map.put((VocabIndex)numWords, w2, prob);
	    }
	} while (wids[numWords ++] != lm.vocab.seIndex() &&
		 (line = file.getline()));

	if (numWords > 0) {
	    wids[numWords] = Vocab_None;

	    makeArray(VocabIndex *, hiddenWids, numNbest);
	    makeArray(LogP, totalProb, numNbest);

	    for (unsigned n = 0; n < numNbest; n++) {
		hiddenWids[n] = new VocabIndex[numWords + 1];
		assert(hiddenWids[n] != 0);
	    }

	    unsigned numHyps =
		    disambiguateSentence(&wids[0], hiddenWids, totalProb,
					 map, lm, hiddenCounts, numNbest, true);
	    if (!numHyps) {
		file.position() << "Disambiguation failed\n";
	    } else if (totals) {
		cout << totalProb[0] << endl;
	    } else if (!posteriors) {
		for (unsigned n = 0; n < numHyps; n++) {
		    if (numNbest > 1) {
		      cout << "NBEST_" << n << " " << totalProb[n] << " ";
		    }
		    for (unsigned i = 0; hiddenWids[n][i] != Vocab_None; i ++) {
			cout << lm.vocab.getWord(wids[i]) << " ";
			if (hiddenWids[n][i] != noEventIndex) {
			    cout << lm.vocab.getWord(hiddenWids[n][i]) << " ";
			}
		    }
		    cout << endl;
		}
	    }

	    for (unsigned n = 0; n < numNbest; n++) {
		delete [] hiddenWids[n];
	    }
	}

	if (haveEscape) {
	    cout << line;
	}
    }
}

LM *
makeMixLM(const char *filename, Vocab &vocab, SubVocab *classVocab,
		    unsigned order, LM *oldLM, double lambda1, double lambda2)
{
    File file(filename, "r");

    /*
     * create class-ngram if -classes were specified, otherwise a regular ngram
     */
    Ngram *lm = factored ? 
		  new ProductNgram((ProductVocab &)vocab, order) :
		  (classVocab != 0) ?
		    (simpleClasses ?
			new SimpleClassNgram(vocab, *classVocab, order) :
		        new ClassNgram(vocab, *classVocab, order)) :
		    new Ngram(vocab, order);
    assert(lm != 0);

    lm->debugme(debug);

    if (!lm->read(file)) {
	cerr << "format error in mix-lm file " << filename << endl;
	exit(1);
    }

    /*
     * Each class LM needs to read the class definitions
     */
    if (classesFile != 0) {
	File file(classesFile, "r");
	((ClassNgram *)lm)->readClasses(file);
    }

    if (oldLM) {
	/*
	 * Compute mixture lambda (make sure 0/0 = 0)
	 */
	Prob lambda = (lambda1 == 0.0) ? 0.0 : lambda1/lambda2;

	LM *newLM = new BayesMix(vocab, *lm, *oldLM, 0, lambda);
	assert(newLM != 0);

	newLM->debugme(debug);

	return newLM;
    } else {
	return lm;
    }
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (version) {
	printVersion(RcsId);
	exit(0);
    }

    if (factored && classesFile) {
	cerr << "factored and class N-gram models are mutually exclusive\n";
	exit(2);
    }

    if (numNbest <= 0) numNbest = 1;		  
				// Silent fix.  Ought to say something here.

    /*
     * Construct language model
     */
    Vocab *vocab;

    vocab = factored ? new ProductVocab : new Vocab;
    assert(vocab != 0);

    vocab->unkIsWord() = keepUnk ? true : false;
    vocab->toLower() = toLower ? true : false;

    if (factored) {
	((ProductVocab *)vocab)->nullIsWord() = keepnull ? true : false;
    }

    SubVocab hiddenVocab(*vocab);
    SubVocab *classVocab = 0;

    LM    *hiddenLM = 0;
    NgramCounts<NgramFractCount> *hiddenCounts = 0;

    if (lmFile) {
	File file(lmFile, "r");

	/*
	 * create based N-gram model (either factored,  word or class-based)
	 */
	if (factored) {
	    hiddenLM = new ProductNgram(*(ProductVocab *)vocab, order);
	} else if (classesFile) {
	    classVocab = new SubVocab(*vocab);
	    assert(classVocab != 0);

	    if (simpleClasses) {
		hiddenLM = new SimpleClassNgram(*vocab, *classVocab, order);
	    } else {
		cerr << "warning: state space will get very large; consider using -simple-classes\n";
		hiddenLM = new ClassNgram(*vocab, *classVocab, order);
	    }
	} else {
	    hiddenLM = new Ngram(*vocab, order);
	}
	assert(hiddenLM != 0);

	hiddenLM->debugme(debug);
	hiddenLM->read(file);

	if (classesFile) {
	    File file(classesFile, "r");
	    ((ClassNgram *)hiddenLM)->readClasses(file);
	}
    } else {
	hiddenLM = new NullLM(*vocab);
	assert(hiddenLM != 0);
	hiddenLM->debugme(debug);
    }

    /*
     * Build the full LM used for hidden event decoding
     */
    LM *useLM = hiddenLM;

    if (mixFile) {
	/*
	 * create a Bayes mixture LM 
	 */
	double mixLambda1 = 1.0 - mixLambda - mixLambda2 - mixLambda3 -
				mixLambda4 - mixLambda5 - mixLambda6 -
				mixLambda7 - mixLambda8 - mixLambda9;

	useLM = makeMixLM(mixFile, *vocab, classVocab, order, useLM,
				mixLambda1,
				mixLambda + mixLambda1);

	if (mixFile2) {
	    useLM = makeMixLM(mixFile2, *vocab, classVocab, order, useLM,
				mixLambda2,
				mixLambda + mixLambda1 + mixLambda2);
	}
	if (mixFile3) {
	    useLM = makeMixLM(mixFile3, *vocab, classVocab, order, useLM,
				mixLambda3,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3);
	}
	if (mixFile4) {
	    useLM = makeMixLM(mixFile4, *vocab, classVocab, order, useLM,
				mixLambda4,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4);
	}
	if (mixFile5) {
	    useLM = makeMixLM(mixFile5, *vocab, classVocab, order, useLM,
				mixLambda5,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5);
	}
	if (mixFile6) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda6,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6);
	}
	if (mixFile7) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda7,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6 + mixLambda7);
	}
	if (mixFile8) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda8,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6 + mixLambda7 + mixLambda8);
	}
	if (mixFile9) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda9, 1.0);
	}
    }

    /*
     * Make sure noevent token is not used in LM
     */
    if (hiddenVocab.getIndex(noHiddenEvent) != Vocab_None) {
	cerr << "LM must not contain " << noHiddenEvent << endl;
	exit(1);
    }

    /*
     * Allocate fractional counts tree
     */
    if (countsFile) {
	hiddenCounts = new NgramCounts<NgramFractCount>(*vocab, order);
	assert(hiddenCounts);
	hiddenCounts->debugme(debug);
    }

    /*
     * Read event vocabulary
     */
    if (hiddenVocabFile) {
	File file(hiddenVocabFile, "r");

	hiddenVocab.read(file);
    }

    if (forceEvent) {
	/*
	 * Omit the noevent token from hidden vocabulary.
	 * We still have to assign an index to it, so just use the regular
	 * vocabulary.
	 */
	noEventIndex = vocab->addWord(noHiddenEvent);
    } else {
	/*
	 * Add noevent token to hidden vocabulary
	 */
	noEventIndex = hiddenVocab.addWord(noHiddenEvent);
    }

    if (textFile) {
	File file(textFile, "r");

	if (continuous) {
	    disambiguateFileContinuous(file, hiddenVocab, *useLM,
							hiddenCounts, numNbest);
	} else {
	    disambiguateFile(file, hiddenVocab, *useLM, hiddenCounts, numNbest);
	}
    }

    if (textMapFile) {
	File file(textMapFile, "r");

	disambiguateTextMap(file, hiddenVocab, *useLM, hiddenCounts, numNbest);
    }

    if (countsFile) {
	File file(countsFile, "w");

	hiddenCounts->write(file, 0, true);
    }

#ifdef DEBUG
    delete hiddenLM;
    delete hiddenCounts;

    return 0;
#endif /* DEBUG */

    exit(0);
}

