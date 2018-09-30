/*
 * disambig --
 *	Disambiguate text tokens using a hidden ngram model
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-8 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/disambig.cc,v 1.23 1999/08/01 09:33:14 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>

#include "option.h"
#include "File.h"
#include "Vocab.h"
#include "VocabMap.h"
#include "NullLM.h"
#include "Ngram.h"
#include "Trellis.cc"
#include "LHash.cc"
#include "Array.cc"

/* 
 * We use strings over VocabIndex as keys into the trellis.
 * Define the necessary support functions (see Map.h and LHash.cc).
 */

typedef const VocabIndex *VocabContext;

static inline unsigned
LHash_hashKey(VocabContext key, unsigned maxBits)
{
    unsigned i = 0;

    for (; *key != Vocab_None; key ++) {
	i += *key;
    }
    return LHash_hashKey(i, maxBits);
}

static inline VocabContext
Map_copyKey(VocabContext key)
{
    VocabIndex *copy = new VocabIndex[Vocab::length(key) + 1];
    assert(copy != 0);

    unsigned i;
    for (i = 0; key[i] != Vocab_None; i ++) {
	copy[i] = key[i];
    }
    copy[i] = Vocab_None;

    return copy;
}

static inline void
Map_freeKey(VocabContext key)
{
    delete [] (VocabIndex *)key;
}

static inline Boolean
LHash_equalKey(VocabContext key1, VocabContext key2)
{
    unsigned i;
    for (i = 0; key1[i] != Vocab_None && key2[i] != Vocab_None; i ++) {
	if (key1[i] != key2[i]) {
	    return false;
	}
    }
    if (key1[i] == Vocab_None && key2[i] == Vocab_None) {
	return true;
    } else {
	return false;
    }
}
     
#define DEBUG_ZEROPROBS		1
#define DEBUG_TRANSITIONS	2

static unsigned order = 2;
static unsigned debug = 0;
static int scale = 0;
static char *lmFile = 0;
static char *vocab1File = 0;
static char *vocab2File = 0;
static char *mapFile = 0;
static char *mapWriteFile = 0;
static char *textFile = 0;
static char *textMapFile = 0;
static int keepUnk = 0;
static int tolower1 = 0;
static int tolower2 = 0;
static double lmw = 1.0;
static double mapw = 1.0;
static int fb = 0;
static int posteriors = 0;
static int logMap = 0;
static int noEOS = 0;
static int continuous = 0;

const LogP LogP_PseudoZero = -100;

static Option options[] = {
    { OPT_STRING, "lm", &lmFile, "hidden token sequence model" },
    { OPT_INT, "order", &order, "ngram order to use for lm" },
    { OPT_STRING, "write-vocab1", &vocab1File, "output observable vocabulary" },
    { OPT_STRING, "write-vocab2", &vocab2File, "output hidden vocabulary" },
    { OPT_STRING, "map", &mapFile, "mapping from observable to hidden tokens" },
    { OPT_TRUE, "logmap", &logMap, "map file contains log probabilities" },
    { OPT_STRING, "write-map", &mapWriteFile, "output map file (for validation)" },
    { OPT_TRUE, "scale", &scale, "scale map probabilities by unigram probs" },
    { OPT_TRUE, "keep-unk", &keepUnk, "preserve unknown words" },
    { OPT_TRUE, "tolower1", &tolower1, "map observable vocabulary to lowercase" },
    { OPT_TRUE, "tolower2", &tolower2, "map hidden vocabulary to lowercase" },
    { OPT_STRING, "text", &textFile, "text file to disambiguate" },
    { OPT_STRING, "text-map", &textMapFile, "text+map file to disambiguate" },
    { OPT_FLOAT, "lmw", &lmw, "language model weight" },
    { OPT_FLOAT, "mapw", &mapw, "map log likelihood weight" },
    { OPT_TRUE, "fb", &fb, "use forward-backward algorithm" },
    { OPT_TRUE, "posteriors", &posteriors, "output posterior probabilities" },
    { OPT_TRUE, "no-eos", &noEOS, "don't assume end-of-sentence token" },
    { OPT_TRUE, "continuous", &continuous, "read input without line breaks" },
    { OPT_INT, "debug", &debug, "debugging level for lm" },
};

/*
 * Disambiguate a sentence by finding the hidden tag sequence compatible with
 * it that has the highest probability
 */
Boolean
disambiguateSentence(Vocab &vocab, VocabIndex *wids, VocabIndex *hiddenWids,
			VocabMap &map, LM &lm, Boolean positionMapped = false)
{
    static VocabIndex emptyContext[] = { Vocab_None };
    unsigned len = Vocab::length(wids);

    Trellis<VocabContext> trellis(len);

    /*
     * Prime the trellis with the tag likelihoods of the first position
     * (No transition probs for this one.)
     */
    {
	VocabMapIter iter(map, positionMapped ? (VocabIndex)0 : wids[0]);

	VocabIndex context[2];
	Prob prob;

	context[1] = Vocab_None;
	while (iter.next(context[0], prob)) {
	    LogP localProb = logMap ? prob : ProbToLogP(prob);
	    trellis.setProb(context, localProb);
	}
    }

    unsigned pos = 1;
    while (wids[pos] != Vocab_None) {

	trellis.step();

	/*
	 * Iterate over all combinations of hidden tags for the current
	 * position and contexts for the previous position.
	 */
	VocabMapIter currIter(map,
				positionMapped ? (VocabIndex)pos : wids[pos]);
	VocabIndex currWid;
	Prob currProb;

	while (currIter.next(currWid, currProb)) {
	    LogP localProb = logMap ? currProb : ProbToLogP(currProb);
	    LogP unigramProb = lm.wordProb(currWid, emptyContext);
	    
	    if (debug >= DEBUG_ZEROPROBS &&
		unigramProb == LogP_Zero &&
		currWid != map.vocab2.unkIndex)
	    {
		cerr << "warning: map token " << map.vocab2.getWord(currWid)
		     << " has prob zero\n";
	    }

	    if (scale && unigramProb != LogP_Zero) {
		/*
		 * Scale the map probability p(tag|word)
		 * by the tag prior probability p(tag), so the
		 * local distance is a scaled version of the
		 * likelihood p(word|tag)
		 */
		localProb -= unigramProb;
	    }

	    /*
	     * Set up new context to transition to
	     */
	    VocabIndex newContext[maxWordsPerLine + 2];
	    newContext[0] = currWid;

	    TrellisIter<VocabContext> prevIter(trellis, pos - 1);
	    VocabContext prevContext;
	    LogP prevProb;

	    while (prevIter.next(prevContext, prevProb)) {
		LogP transProb = lm.wordProb(currWid, prevContext);

		if (transProb == LogP_Zero && unigramProb == LogP_Zero) {
		    /*
		     * Zero probability due to an OOV
		     * would equalize the probabilities of all paths
		     * and make the Viterbi backtrace useless.
		     */
		    transProb = LogP_PseudoZero;
		}

		/*
		 * Determined new context
		 */
		unsigned i;
		for (i = 0; i < maxWordsPerLine && 
					prevContext[i] != Vocab_None; i ++)
		{
		    newContext[i + 1] = prevContext[i];
		}
		newContext[i + 1] = Vocab_None;

		/*
		 * Truncate context to what is actually used by LM,
		 * but keep at least one word so we can recover words later.
		 */
		unsigned usedLength;
		lm.contextID(newContext, usedLength);
		newContext[usedLength > 0 ? usedLength : 1] = Vocab_None;

		if (debug >= DEBUG_TRANSITIONS) {
		    cerr << "position = " << pos
			 << " from: " << (map.vocab2.use(), prevContext)
			 << " to: " << (map.vocab2.use(), newContext)
			 << " trans = " << transProb
			 << " local = " << localProb
			 << endl;
		}

		trellis.update(prevContext, newContext,
					    lmw * transProb + mapw * localProb);
	    }
	}

	pos ++;
    }

    if (fb || posteriors) {
	/*
	 * Forward-backward algorithm: compute backward scores
	 */
	pos --;
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
	     * Iterate over all combinations of hidden tags for the current
	     * position and contexts for the previous position.
	     */
	    VocabMapIter currIter(map,
				positionMapped ? (VocabIndex)pos : wids[pos]);
	    VocabIndex currWid;
	    Prob currProb;

	    while (currIter.next(currWid, currProb)) {
		LogP localProb = logMap ? currProb : ProbToLogP(currProb);
		LogP unigramProb = lm.wordProb(currWid, emptyContext);
		
		if (debug >= DEBUG_ZEROPROBS &&
		    unigramProb == LogP_Zero &&
		    currWid != map.vocab2.unkIndex)
		{
		    cerr << "warning: map token " << map.vocab2.getWord(currWid)
			 << " has prob zero\n";
		}

		if (scale && unigramProb != LogP_Zero) {
		    /*
		     * Scale the map probability p(tag|word)
		     * by the tag prior probability p(tag), so the
		     * local distance is a scaled version of the
		     * likelihood p(word|tag)
		     */
		    localProb -= unigramProb;
		}

		/*
		 * Set up new context to transition to
		 */
		VocabIndex newContext[maxWordsPerLine + 2];
		newContext[0] = currWid;

		TrellisIter<VocabContext> prevIter(trellis, pos - 1);
		VocabContext prevContext;
		LogP prevProb;

		while (prevIter.next(prevContext, prevProb)) {
		    LogP transProb = lm.wordProb(currWid, prevContext);

		    if (transProb == LogP_Zero && unigramProb == LogP_Zero) {
			/*
			 * Zero probability due to an OOV
			 * would equalize the probabilities of all paths
			 * and make the Viterbi backtrace useless.
			 */
			transProb = LogP_PseudoZero;
		    }

		    /*
		     * Determined new context
		     */
		    unsigned i;
		    for (i = 0; i < maxWordsPerLine && 
					    prevContext[i] != Vocab_None; i ++)
		    {
			newContext[i + 1] = prevContext[i];
		    }
		    newContext[i + 1] = Vocab_None;

		    /*
		     * Truncate context to what is actually used by LM,
		     * but keep at least one word so we can recover words later.
		     */
		    unsigned usedLength;
		    lm.contextID(newContext, usedLength);
		    newContext[usedLength > 0 ? usedLength : 1] = Vocab_None;

		    trellis.updateBack(prevContext, newContext,
					    lmw * transProb + mapw * localProb);

		    if (debug >= DEBUG_TRANSITIONS) {
			cerr << "backward position = " << pos
			     << " from: " << (map.vocab2.use(), prevContext)
			     << " to: " << (map.vocab2.use(), newContext)
			     << " trans = " << transProb
			     << " local = " << localProb
			     << endl;
		    }
		}
	    }
	    pos --;
	}

	/*
	 * Compute posterior symbol probabilities and extract most
	 * probable symbol for each position
	 */
	for (pos = 0; pos < len; pos ++) {
	    /*
	     * Compute symbol probabilities by summing posterior probs
	     * of all states corresponding to the same symbol
	     */
	    LHash<VocabIndex,LogP> symbolProbs;

	    TrellisIter<VocabContext> iter(trellis, pos);
	    VocabContext context;
	    LogP forwProb;
	    while (iter.next(context, forwProb)) {
		LogP posterior;
		
		if (fb) {
		    posterior = forwProb + trellis.getBackLogP(context, pos);
		} else {
		    LogP backmax;
		    posterior = trellis.getMax(context, pos, backmax);
		    posterior += backmax;
		}

		Boolean foundP;
		LogP *symbolProb = symbolProbs.insert(context[0], foundP);
		if (!foundP) {
		    *symbolProb = posterior;
		} else {
		    *symbolProb = AddLogP(*symbolProb, posterior);
		}
	    }

	    /*
	     * Find symbol with highest posterior
	     */
	    LogP totalPosterior = LogP_Zero;
	    LogP maxPosterior = LogP_Zero;
	    VocabIndex bestSymbol = Vocab_None;

	    LHashIter<VocabIndex,LogP> symbolIter(symbolProbs);
	    LogP *symbolProb;
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
		return false;
	    }

	    hiddenWids[pos] = bestSymbol;

	    /*
	     * Print posterior probabilities
	     */
	    if (posteriors) {
		cout << vocab.getWord(wids[pos]) << "\t";

		symbolIter.init();
		while (symbolProb = symbolIter.next(symbol)) {
		    LogP posterior = *symbolProb - totalPosterior;

		    cout << " " << map.vocab2.getWord(symbol)
			 << " " << (logMap ? posterior : LogPtoProb(posterior));
		}
		cout << endl;
	    }
	}
    } else {
	VocabContext *hiddenContexts = new VocabContext[len + 1];
	assert(hiddenContexts != 0);

	/*
	 * Run Viterbi to get most likely state sequence
	 */
	if (trellis.viterbi(hiddenContexts, len) != len) {
	    delete [] hiddenContexts;
	    return false;
	}

	/*
	 * Transfer the first word of each state (word context) into the
	 * word index string
	 */
	for (unsigned i = 0; i < len; i ++) {
	    hiddenWids[i] = hiddenContexts[i][0];
	}

	delete [] hiddenContexts;
    }

    hiddenWids[len] = Vocab_None;
    return true;
}

/*
 * Get one input sentences at a time, map it to wids, 
 * disambiguate it, and print out the result
 */
void
disambiguateFile(File &file, VocabMap &map, LM &lm)
{
    char *line;
    VocabString sentence[maxWordsPerLine];

    while (line = file.getline()) {

	unsigned numWords = Vocab::parseWords(line, sentence, maxWordsPerLine);
	if (numWords == maxWordsPerLine) {
	    file.position() << "too many words per sentence\n";
	} else {
	    VocabIndex wids[maxWordsPerLine + 2];

	    VocabIndex hiddenWids[maxWordsPerLine + 2];
	    VocabString hiddenWords[maxWordsPerLine + 2];

	    map.vocab1.getIndices(sentence, &wids[1], maxWordsPerLine,
						    map.vocab1.unkIndex);

	    wids[0] = map.vocab1.ssIndex;
	    if (noEOS) {
		wids[numWords + 1] = Vocab_None;
	    } else {
		wids[numWords + 1] = map.vocab1.seIndex;
		wids[numWords + 2] = Vocab_None;
	    }

	    if (!disambiguateSentence(map.vocab1, wids, hiddenWids, map, lm)) {
		file.position() << "viterbi failed\n";
	    } else {
		map.vocab2.getWords(hiddenWids, hiddenWords,
						maxWordsPerLine + 2);
		if (keepUnk) {
		    /*
		     * Look for <unk> symbols in the output and replace
		     * them with the corresponding input tokens
		     */
		    for (unsigned i = 0; hiddenWids[i] != Vocab_None; i++) {
			if (i > 0 && hiddenWids[i] == map.vocab2.unkIndex) {
			    hiddenWords[i] = sentence[i - 1];
			}
		    }
		}
		if (!posteriors) {
		    cout << (map.vocab2.use(), hiddenWords) << endl;
		}
	    }
	}
    }
}

/*
 * Read entire file ignoring line breaks, map it to wids, 
 * disambiguate it, and print out the result
 */
void
disambiguateFileContinuous(File &file, VocabMap &map, LM &lm)
{
    char *line;
    Array<VocabIndex> wids;

    unsigned lineStart = 0; // index into the above to mark the offset for the 
			    // current line's data

    while (line = file.getline()) {
	VocabString words[maxWordsPerLine];
	unsigned numWords =
		Vocab::parseWords(line, words, maxWordsPerLine);

	if (numWords == maxWordsPerLine) {
	    file.position() << "too many words per line\n";
	} else {
	    // This effectively allocates more space
	    wids[lineStart + numWords] = Vocab_None;

	    map.vocab1.getIndices(words, &wids[lineStart], numWords,
							map.vocab1.unkIndex);
	    lineStart += numWords;
	}
    }

    if (lineStart == 0) {
	// empty input -- nothing to do
	return;
    }

    Array<VocabIndex> hiddenWids;
    // This implicitly allocates enough space for the posterior vector
    hiddenWids[lineStart] = Vocab_None;

    if (!disambiguateSentence(map.vocab1, &wids[0], &hiddenWids[0], map, lm)) {
	file.position() << "viterbi failed\n";
    } else {
	if (!posteriors) {
	    for (unsigned i = 0; hiddenWids[i] != Vocab_None; i++) {
		// XXX: keepUnk not implemented yet.
		cout << map.vocab2.getWord(hiddenWids[i]) << endl;
	    }
	}
    }
}

/*
 * Read a "text-map" file, containing one word per line, followed by
 * map entries;
 * disambiguate it, and print out the result
 */
void
disambiguateTextMap(File &file, Vocab &vocab, LM &lm)
{
    char *line;

    while (line = file.getline()) {
	/*
	 * Hack alert! We pass the map entries associated with the word
	 * instances in a VocabMap, but we encode the word position (not
	 * its identity) as the first VocabIndex.
	 */
	PosVocabMap map(lm.vocab);

	unsigned numWords = 0;
	Array<VocabIndex> wids;

	/*
	 * Process one sentences
	 */
	do {
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
	     * First field is the V1 word
	     * Note we use addWord() since V1 words are by definition
	     * only found in the textmap, so there are no OOVs here.
	     */
	    wids[numWords] = vocab.addWord(mapFields[0]);
	    
	    /*
	     * Parse the remaining words as either probs or V2 words
	     */
	    unsigned i = 1;

	    while (i < howmany) {
		double prob;

		VocabIndex w2 = lm.vocab.addWord(mapFields[i++]);

		if (i < howmany && sscanf(mapFields[i], "%lf", &prob)) {
		    i ++;
		} else {
		    prob = logMap ? LogP_One : 1.0;
		}

		map.put((VocabIndex)numWords, w2, prob);
	    }
	} while (wids[numWords ++] != vocab.seIndex && (line = file.getline()));

	if (numWords == 0) {
	    // empty input -- nothing to do
	   break;
	}

	wids[numWords] = Vocab_None;

	Array<VocabIndex> hiddenWids;
	// This implicitly allocates enough space for the posterior vector
	hiddenWids[numWords] = Vocab_None;

	if (!disambiguateSentence(vocab, &wids[0], &hiddenWids[0], map, lm,
									true))
	{
	    file.position() << "viterbi failed\n";
	} else {
	    if (!posteriors) {
		for (unsigned i = 0; hiddenWids[i] != Vocab_None; i++) {
		    cout << map.vocab2.getWord(hiddenWids[i]) << endl;
		}
	    }
	}
    }
}

int
main(int argc, char **argv)
{
    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    /*
     * Construct language model
     */

    Vocab hiddenVocab;
    Vocab vocab;
    LM    *hiddenLM;

    VocabMap map(vocab, hiddenVocab, logMap);

    vocab.toLower = tolower1? true : false;
    hiddenVocab.toLower = tolower2 ? true : false;

    if (mapFile) {
	File file(mapFile, "r");

	if (!map.read(file)) {
	    cerr << "format error in map file\n";
	    exit(1);
	}
    }

    if (lmFile) {
	File file(lmFile, "r");

	hiddenLM = new Ngram(hiddenVocab, order);
	assert(hiddenLM != 0);

	hiddenLM->debugme(debug);
	hiddenLM->read(file);
    } else {
	hiddenLM = new NullLM(hiddenVocab);
	assert(hiddenLM != 0);
	hiddenLM->debugme(debug);
    }

    if (textFile) {
	File file(textFile, "r");

	if (continuous) {
	    disambiguateFileContinuous(file, map, *hiddenLM);
	} else {
	    disambiguateFile(file, map, *hiddenLM);
	}
    }

    if (textMapFile) {
	File file(textMapFile, "r");

	disambiguateTextMap(file, vocab, *hiddenLM);
    }

    if (mapWriteFile) {
	File file(mapWriteFile, "w");
	map.write(file);
    }

    if (vocab1File) {
	File file(vocab1File, "w");
	hiddenVocab.write(file);
    }
    if (vocab2File) {
	File file(vocab2File, "w");
	vocab.write(file);
    }

#ifdef DEBUG
    delete hiddenLM;
    return 0;
#endif /* DEBUG */

    exit(0);
}

