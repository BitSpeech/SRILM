/*
 * ngram --
 *	Create and manipulate ngram (and related) models
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2011 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Id: ngram.cc,v 1.113 2011/01/14 01:02:18 stolcke Exp $";
#endif

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#ifndef _MSC_VER
#include <unistd.h>
#define GETPID getpid
#else
#include <process.h>
#define GETPID _getpid
#endif
#include <string.h>
#include <time.h>

#ifdef NEED_RAND48
extern "C" {
    void srand48(long);
}
#endif

#include "option.h"
#include "version.h"
#include "zio.h"
#include "File.h"
#include "Vocab.h"
#include "SubVocab.h"
#include "MultiwordVocab.h"
#include "MultiwordLM.h"
#include "NonzeroLM.h"
#include "NBest.h"
#include "TaggedVocab.h"
#include "Ngram.h"
#include "TaggedNgram.h"
#include "StopNgram.h"
#include "ClassNgram.h"
#include "SimpleClassNgram.h"
#include "DFNgram.h"
#include "SkipNgram.h"
#include "HiddenNgram.h"
#include "HiddenSNgram.h"
#include "NullLM.h"
#include "LMClient.h"
#include "BayesMix.h"
#include "LoglinearMix.h"
#include "AdaptiveMix.h"
#include "AdaptiveMarginals.h"
#include "NgramCountLM.h"
#include "CacheLM.h"
#include "DynamicLM.h"
#include "DecipherNgram.h"
#include "HMMofNgrams.h"
#include "RefList.h"
#include "ProductNgram.h"
#include "Array.cc"

static int version = 0;
static unsigned order = defaultNgramOrder;
static unsigned debug = 0;
static char *pplFile = 0;
static char *escape = 0;
static char *countFile = 0;
static int countEntropy = 0;
static char *useServer = 0;
static int cacheServedNgrams = 0;
static unsigned serverPort = 0;
static unsigned serverMaxClients = 0;
static unsigned countOrder = 0;
static int useFloatCounts = 0;
static char *vocabFile = 0;
static char *vocabAliasFile = 0;
static char *noneventFile = 0;
static int limitVocab = 0;
static char *lmFile  = 0;
static char *mixFile  = 0;
static char *mixFile2 = 0;
static char *mixFile3 = 0;
static char *mixFile4 = 0;
static char *mixFile5 = 0;
static char *mixFile6 = 0;
static char *mixFile7 = 0;
static char *mixFile8 = 0;
static char *mixFile9 = 0;
static int bayesLength = -1;	/* marks unset option */
static double bayesScale = 1.0;
static double mixLambda = 0.5;
static double mixLambda2 = 0.0;
static double mixLambda3 = 0.0;
static double mixLambda4 = 0.0;
static double mixLambda5 = 0.0;
static double mixLambda6 = 0.0;
static double mixLambda7 = 0.0;
static double mixLambda8 = 0.0;
static double mixLambda9 = 0.0;
static int loglinearMix = 0;
static int reverseSents = 0;
static int noSOS = 0;
static int noEOS = 0;
static char *writeLM  = 0;
static char *writeBinLM = 0;
static char *writeBinV1LM = 0;
static char *writeVocab  = 0;
static int memuse = 0;
static int renormalize = 0;
static double prune = 0.0;
static int pruneLowProbs = 0;
static char *pruneHistoryLM = 0;
static int minprune = 2;
static int skipOOVs = 0;
static unsigned generateSents = 0;
static char *generatePrefixFile = 0;
static int seed = 0;  /* default dynamically generated in main() */
static int df = 0;
static int skipNgram = 0;
static int hiddenS = 0;
static char *hiddenVocabFile = 0;
static int hiddenNot = 0;
static char *classesFile = 0;
static int simpleClasses = 0;
static int expandClasses = -1;
static unsigned expandExact = 0;
static int tagged = 0;
static int factored = 0;
static int toLower = 0;
static int multiwords = 0;
static int splitMultiwords = 0;
static const char *multiChar = MultiwordSeparator;
static int keepunk = 0;
static int keepnull = 1;
static char *mapUnknown = 0;
static char *zeroprobWord = 0;
static int null = 0;
static unsigned cache = 0;
static double cacheLambda = 0.05;
static int dynamic = 0;
static double dynamicLambda = 0.05;
static char *noiseTag = 0;
static char *noiseVocabFile = 0;
static char *stopWordFile = 0;
static int decipherHack = 0;
static int hmm = 0;
static int useCountLM = 0;
static int adaptMix = 0;
static double adaptDecay = 1.0;
static unsigned adaptIters = 2;
static char *adaptMarginals = 0;
static double adaptMarginalsBeta = 0.5;
static int adaptMarginalsRatios = 0;
static char *baseMarginals = 0;
static char *rescoreNgramFile = 0;

/*
 * N-Best related variables
 */

static char *nbestFile = 0;
static char *nbestFiles = 0;
static char *writeNbestDir = 0;
static int writeDecipherNbest = 0;
static int noReorder = 0;
static unsigned maxNbest = 0;
static char *rescoreFile = 0;
static char *decipherLM = 0;
static unsigned decipherOrder = 2;
static int decipherNoBackoff = 0;
static double decipherLMW = 8.0;
static double decipherWTW = 0.0;
static double rescoreLMW = 8.0;
static double rescoreWTW = 0.0;

static Option options[] = {
    { OPT_TRUE, "version", &version, "print version information" },
    { OPT_UINT, "order", &order, "max ngram order" },
    { OPT_UINT, "debug", &debug, "debugging level for lm" },
    { OPT_TRUE, "skipoovs", &skipOOVs, "skip n-gram contexts containing OOVs" },
    { OPT_TRUE, "df", &df, "use disfluency ngram model" },
    { OPT_TRUE, "tagged", &tagged, "use a tagged LM" },
    { OPT_TRUE, "factored", &factored, "use a factored LM" },
    { OPT_TRUE, "skip", &skipNgram, "use skip ngram model" },
    { OPT_TRUE, "hiddens", &hiddenS, "use hidden sentence ngram model" },
    { OPT_STRING, "hidden-vocab", &hiddenVocabFile, "hidden ngram vocabulary" },
    { OPT_TRUE, "hidden-not", &hiddenNot, "process overt hidden events" },
    { OPT_STRING, "classes", &classesFile, "class definitions" },
    { OPT_TRUE, "simple-classes", &simpleClasses, "use unique class model" },
    { OPT_INT, "expand-classes", &expandClasses, "expand class-model into word-model" },
    { OPT_UINT, "expand-exact", &expandExact, "compute expanded ngrams longer than this exactly" },
    { OPT_STRING, "stop-words", &stopWordFile, "stop-word vocabulary for stop-Ngram LM" },
    { OPT_TRUE, "decipher", &decipherHack, "use bigram model exactly as recognizer" },
    { OPT_TRUE, "unk", &keepunk, "vocabulary contains unknown word tag" },
    { OPT_FALSE, "nonull", &keepnull, "remove <NULL> in LM" },
    { OPT_STRING, "map-unk", &mapUnknown, "word to map unknown words to" },
    { OPT_STRING, "zeroprob-word", &zeroprobWord, "word to back off to for zero probs" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_TRUE, "multiwords", &multiwords, "split multiwords for LM evaluation" },
    { OPT_STRING, "ppl", &pplFile, "text file to compute perplexity from" },
    { OPT_STRING, "escape", &escape, "escape prefix to pass data through -ppl" },
    { OPT_STRING, "counts", &countFile, "count file to compute perplexity from" },
    { OPT_TRUE, "counts-entropy", &countEntropy, "compute entropy (not perplexity) from counts" },
    { OPT_UINT, "count-order", &countOrder, "max count order used by -counts" },
    { OPT_TRUE, "float-counts", &useFloatCounts, "use fractional -counts" },
    { OPT_STRING, "use-server", &useServer, "port@host to use as LM server" },
    { OPT_TRUE, "cache-served-ngrams", &cacheServedNgrams, "enable client side caching" },
    { OPT_UINT, "server-port", &serverPort, "port to listen on as probability server" },
    { OPT_UINT, "server-maxclients", &serverMaxClients, "maximum number of simultaneous server clients" },
    { OPT_UINT, "gen", &generateSents, "number of random sentences to generate" },
    { OPT_STRING, "gen-prefixes", &generatePrefixFile, "file of prefixes to generate sentences" },
    { OPT_INT, "seed", &seed, "seed for randomization" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_STRING, "vocab-aliases", &vocabAliasFile, "vocab alias file" },
    { OPT_STRING, "nonevents", &noneventFile, "non-event vocabulary" },
    { OPT_TRUE, "limit-vocab", &limitVocab, "limit LM reading to specified vocabulary" },
    { OPT_STRING, "lm", &lmFile, "file in ARPA LM format" },
    { OPT_UINT, "bayes", &bayesLength, "context length for Bayes mixture LM" },
    { OPT_FLOAT, "bayes-scale", &bayesScale, "log likelihood scale for -bayes" },
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
    { OPT_TRUE, "loglinear-mix", &loglinearMix, "use log-linear mixture LM" },
    { OPT_TRUE, "null", &null, "use a null language model" },
    { OPT_UINT, "cache", &cache, "history length for cache language model" },
    { OPT_FLOAT, "cache-lambda", &cacheLambda, "interpolation weight for -cache" },
    { OPT_TRUE, "dynamic", &dynamic, "interpolate with a dynamic lm" },
    { OPT_TRUE, "hmm", &hmm, "use HMM of n-grams model" },
    { OPT_TRUE, "count-lm", &useCountLM, "use a count-based LM" },
    { OPT_TRUE, "adapt-mix", &adaptMix, "use adaptive mixture of n-grams model" },
    { OPT_FLOAT, "adapt-decay", &adaptDecay, "history likelihood decay factor" },
    { OPT_UINT, "adapt-iters", &adaptIters, "EM iterations for adaptive mix" },
    { OPT_STRING, "adapt-marginals", &adaptMarginals, "unigram marginals to adapt base LM to" },
    { OPT_STRING, "base-marginals", &baseMarginals, "unigram marginals of base LM to" },
    { OPT_FLOAT, "adapt-marginals-beta", &adaptMarginalsBeta, "marginals adaptation weight" },
    { OPT_TRUE, "adapt-marginals-ratios", &adaptMarginalsRatios, "compute ratios between marginals-adapted and base probs" },
    { OPT_FLOAT, "dynamic-lambda", &dynamicLambda, "interpolation weight for -dynamic" },
    { OPT_TRUE, "reverse", &reverseSents, "reverse words" },
    { OPT_TRUE, "no-sos", &noSOS, "don't insert start-of-sentence tokens" },
    { OPT_TRUE, "no-eos", &noEOS, "don't insert end-of-sentence tokens" },
    { OPT_STRING, "rescore-ngram", &rescoreNgramFile, "recompute probs in N-gram LM" },
    { OPT_STRING, "write-lm", &writeLM, "re-write LM to file" },
    { OPT_STRING, "write-bin-lm", &writeBinLM, "write LM to file in binary format" },
    { OPT_STRING, "write-oldbin-lm", &writeBinV1LM, "write LM to file in old binary format" },
    { OPT_STRING, "write-vocab", &writeVocab, "write LM vocab to file" },
    { OPT_TRUE, "renorm", &renormalize, "renormalize backoff weights" },
    { OPT_FLOAT, "prune", &prune, "prune redundant probs" },
    { OPT_UINT, "minprune", &minprune, "prune only ngrams at least this long" },
    { OPT_TRUE, "prune-lowprobs", &pruneLowProbs, "low probability N-grams" },
    { OPT_STRING, "prune-history-lm", &pruneHistoryLM, "LM used for history probabilities in pruning" },
    { OPT_TRUE, "memuse", &memuse, "show memory usage" },

    { OPT_STRING, "nbest", &nbestFile, "nbest list file to rescore" },
    { OPT_STRING, "nbest-files", &nbestFiles, "list of N-best filenames" },
    { OPT_TRUE, "split-multiwords", &splitMultiwords, "split multiwords in N-best lists" },
    { OPT_STRING, "multi-char", &multiChar, "multiword component delimiter" },
    { OPT_STRING, "write-nbest-dir", &writeNbestDir, "output directory for N-best rescoring" },
    { OPT_TRUE, "decipher-nbest", &writeDecipherNbest, "output Decipher n-best format" },
    { OPT_UINT, "max-nbest", &maxNbest, "maximum number of hyps to consider" },
    { OPT_TRUE, "no-reorder", &noReorder, "don't reorder N-best hyps after rescoring" },
    { OPT_STRING, "rescore", &rescoreFile, "hyp stream input file to rescore" },
    { OPT_STRING, "decipher-lm", &decipherLM, "DECIPHER(TM) LM for nbest list generation" },
    { OPT_UINT, "decipher-order", &decipherOrder, "ngram order for -decipher-lm" },
    { OPT_TRUE, "decipher-nobackoff", &decipherNoBackoff, "disable backoff hack in recognizer LM" },
    { OPT_FLOAT, "decipher-lmw", &decipherLMW, "DECIPHER(TM) LM weight" },
    { OPT_FLOAT, "decipher-wtw", &decipherWTW, "DECIPHER(TM) word transition weight" },
    { OPT_FLOAT, "rescore-lmw", &rescoreLMW, "rescoring LM weight" },
    { OPT_FLOAT, "rescore-wtw", &rescoreWTW, "rescoring word transition weight" },
    { OPT_STRING, "noise", &noiseTag, "noise tag to skip" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary to skip" },
};

/*
 * Rescore N-best list
 */
void
rescoreNbest(LM &lm, const char *inFilename, const char *outFilename)
{
    NBestList nbList(lm.vocab, maxNbest, splitMultiwords ? multiChar : 0);

    File inlist(inFilename, "r");
    if (!nbList.read(inlist)) {
	cerr << "format error in nbest file\n";
	exit(1);
    }

    if (nbList.numHyps() == 0) {
	cerr << "warning: " << inFilename << " is empty, not rescored\n";
	return;
    }

    if (decipherLM) {
	/*
	 * decipherNoBackoff prevents the Decipher LM from simulating
	 * backoff paths when they score higher than direct probabilities.
	 */
	DecipherNgram oldLM(lm.vocab, decipherOrder, !decipherNoBackoff);

	File file(decipherLM, "r");

	if (!oldLM.read(file, limitVocab)) {
	    cerr << "format error in Decipher LM\n";
	    exit(1);
	}

	nbList.decipherFix(oldLM, decipherLMW, decipherWTW);
    }

    nbList.rescoreHyps(lm, rescoreLMW, rescoreWTW);

    if (!noReorder) {
	nbList.sortHyps();
    }

    if (outFilename) {
	File sout(outFilename, "w");
	nbList.write(sout, writeDecipherNbest);
    } else {
	File sout(stdout);
	nbList.write(sout, writeDecipherNbest);
    }
}

LM *
makeMixLM(const char *filename, Vocab &vocab, SubVocab *classVocab,
		    unsigned order, LM *oldLM, double lambda1, double lambda2)
{
    LM *lm;
    Boolean useLMClient = false;

    if (useServer && strchr(filename, '@') && !strchr(filename, '/')) {
	/*
	 * Filename looks like a network LM spec -- create LMClient object
	 */
	lm = new LMClient(vocab, filename, order,
					    cacheServedNgrams ? order : 0);
	assert(lm != 0);

	useLMClient = true;
    } else {
	File file(filename, "r");

	/*
	 * create factored LM if -factored was specified, 
	 * class-ngram if -classes were specified,
	 * and otherwise a regular ngram
	 */
	lm = factored ?
		      new ProductNgram((ProductVocab &)vocab, order) :
		      (classVocab != 0) ?
			(simpleClasses ?
			    new SimpleClassNgram(vocab, *classVocab, order) :
			    new ClassNgram(vocab, *classVocab, order)) :
			new Ngram(vocab, order);
	assert(lm != 0);

	((Ngram *)lm)->skipOOVs() = skipOOVs;

	if (!lm->read(file, limitVocab)) {
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
    }

    /*
     * Compute mixture lambda (make sure 0/0 = 0)
     */
    Prob lambda = (lambda1 == 0.0) ? 0.0 : lambda1/lambda2;

    if (oldLM == 0) {
	return lm;
    } else if (loglinearMix) {
	/*
	 * log-linear mixture
	 */
	LM *newLM = new LoglinearMix(vocab, *lm, *oldLM, lambda);
	assert(newLM != 0);

	return newLM;
    } else if (!useLMClient && bayesLength < 0) {
	/*
	 * static mixture -- only possible with Ngram LMs
	 */
	((Ngram *)oldLM)->mixProbs(*(Ngram *)lm, 1.0 - lambda);
	delete lm;

	return oldLM;
    } else {
	/*
	 * dymamic Bayesian mixture
	 */
	LM *newLM = new BayesMix(vocab, *lm, *oldLM,
					bayesLength, lambda, bayesScale);
	assert(newLM != 0);

	return newLM;
    }
}

LM *
makeLoglinearMixLM(Array<const char *> filenames, Vocab &vocab,
		   SubVocab *classVocab, unsigned order,
		   LM *oldLM, Array<double> lambdas)
{
    Array<LM *> allLMs;
    allLMs[0] = oldLM;

    for (unsigned i = 1; i < filenames.size(); i++) {
	const char *filename = filenames[i];
	File file(filename, "r");

	/*
	 * create factored LM if -factored was specified, 
	 * class-ngram if -classes were specified,
	 * and otherwise a regular ngram
	 */
	Ngram *lm = factored ?
		      new ProductNgram((ProductVocab &)vocab, order) :
		      (classVocab != 0) ?
			(simpleClasses ?
			    new SimpleClassNgram(vocab, *classVocab, order) :
			    new ClassNgram(vocab, *classVocab, order)) :
			new Ngram(vocab, order);
	assert(lm != 0);

	lm->skipOOVs() = skipOOVs;

	if (!lm->read(file, limitVocab)) {
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
	allLMs[i] = lm;
    }

    LM *newLM = new LoglinearMix(vocab, allLMs, lambdas);
    assert(newLM != 0);

    return newLM;
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    /* set default seed for randomization */
    seed = time(NULL) + GETPID();

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (version) {
	printVersion(RcsId);
	exit(0);
    }

    if (hmm + useCountLM + adaptMix + decipherHack + tagged +
	skipNgram + hiddenS + df + factored + (hiddenVocabFile != 0) +
	(classesFile != 0 || expandClasses >= 0) + (stopWordFile != 0) > 1)
    {
	cerr << "HMM, NgramCountLM, AdaptiveMix, Decipher, tagged, factored, DF, hidden N-gram, hidden-S, class N-gram, skip N-gram and stop-word N-gram models are mutually exclusive\n";
	exit(2);
    }

    /*
     * Set random seed
     */
    srand48((long)seed);

    /*
     * Construct language model
     */

    LM::initialDebugLevel = debug;

    Vocab *vocab;
    Ngram *ngramLM;
    LM *useLM;

    if (factored + tagged + multiwords > 1) {
	cerr << "factored, tagged, and multiword vocabularies are mutually exclusive\n";
	exit(2);
    }

    if (null + (lmFile != 0) + (useServer != 0) > 1) {
	cerr << "-null, -lm, and -use-server are mutually exclusive\n";
	exit(2);
    }

    vocab = tagged ? new TaggedVocab :
		multiwords ? new MultiwordVocab(multiChar) :
		      factored ? new ProductVocab :
   			          new Vocab;
    assert(vocab != 0);

    vocab->unkIsWord() = keepunk ? true : false;
    vocab->toLower() = toLower ? true : false;

    if (factored) {
	((ProductVocab *)vocab)->nullIsWord() = keepnull ? true : false;
    }

    /*
     * Change unknown word string if requested
     */
    if (mapUnknown) {
	vocab->remove(vocab->unkIndex());
	vocab->unkIndex() = vocab->addWord(mapUnknown);
    }

    if (vocabFile) {
	File file(vocabFile, "r");
	vocab->read(file);
    }

    if (vocabAliasFile) {
	File file(vocabAliasFile, "r");
	vocab->readAliases(file);
    }

    if (noneventFile) {
	/*
	 * create temporary sub-vocabulary for non-event words
	 */
	SubVocab nonEvents(*vocab);

	File file(noneventFile, "r");
	nonEvents.read(file);

	vocab->addNonEvents(nonEvents);
    }

    SubVocab *stopWords = 0;
    if (stopWordFile != 0) {
	stopWords = new SubVocab(*vocab);
	assert(stopWords);

	File file(stopWordFile, "r");
	stopWords->read(file);
    }

    SubVocab *hiddenEvents = 0;
    if (hiddenVocabFile != 0) {
	hiddenEvents = new SubVocab(*vocab);
	assert(hiddenEvents);

	File file(hiddenVocabFile, "r");
	hiddenEvents->read(file);
    }

    SubVocab *classVocab = 0;
    if (classesFile != 0 || expandClasses >= 0) {
	classVocab = new SubVocab(*vocab);
	assert(classVocab);

	/*
	 * limitVocab on class N-grams only works if the classes are 
	 * in the vocabulary at read time.  We ensure this by reading 
	 * the class names (the first column of the class definitions)
	 * into the vocabulary.
	 */
	if (limitVocab) {
	    File file(classesFile, "r");
	    classVocab->read(file);
	}
    }

    ngramLM =
       decipherHack ? new DecipherNgram(*vocab, order, !decipherNoBackoff) :
	 df ? new DFNgram(*vocab, order) :
	   skipNgram ? new SkipNgram(*vocab, order) :
	     hiddenS ? new HiddenSNgram(*vocab, order) :
	       tagged ? new TaggedNgram(*(TaggedVocab *)vocab, order) :
	        factored ? new ProductNgram(*(ProductVocab *)vocab, order) :
		 (stopWordFile != 0) ? new StopNgram(*vocab, *stopWords, order):
		   (hiddenVocabFile != 0) ? new HiddenNgram(*vocab, *hiddenEvents, order, hiddenNot) :
		     (classVocab != 0) ? 
			(simpleClasses ?
			    new SimpleClassNgram(*vocab, *classVocab, order) :
			    new ClassNgram(*vocab, *classVocab, order)) :
		        new Ngram(*vocab, order);
    assert(ngramLM != 0);

    if (skipOOVs) {
	ngramLM->skipOOVs() = true;
    }

    if (null) {
	useLM = new NullLM(*vocab);
	assert(useLM != 0);
    } else if (useServer) {
    	useLM = new LMClient(*vocab, useServer, order,
						cacheServedNgrams ? order : 0);
	assert(useLM != 0);
    } else if (lmFile) {
	if (hmm) {
	    /*
	     * Read an HMM of Ngrams
	     */
	    File file(lmFile, "r");
	    HMMofNgrams *hmm = new HMMofNgrams(*vocab, order);

	    if (!hmm->read(file, limitVocab)) {
		cerr << "format error in hmm-lm file\n";
		exit(1);
	    }

	    useLM = hmm;
	} else if (useCountLM) {
	    /*
	     * Read an Ngram-count LM
	     */
	    File file(lmFile, "r");
	    NgramCountLM *lm = new NgramCountLM(*vocab, order);

	    if (!lm->read(file, limitVocab)) {
		cerr << "format error in count-lm file\n";
		exit(1);
	    }

	    useLM = lm;
	} else if (adaptMix) {
	    /*
	     * Read an adaptive mixture of Ngrams
	     */
	    File file(lmFile, "r");
	    AdaptiveMix *lm = new AdaptiveMix(*vocab, adaptDecay,
							bayesScale, adaptIters);

	    if (!lm->read(file, limitVocab)) {
		cerr << "format error in lm file\n";
		exit(1);
	    }

	    useLM = lm;
	} else {
	    /*
	     * Read just a single LM
	     */
	    File file(lmFile, "r");

	    if (!ngramLM->read(file, limitVocab)) {
		cerr << "format error in lm file\n";
		exit(1);
	    }

	    if (mixFile && !loglinearMix && bayesLength < 0) {
		/*
		 * perform static interpolation (ngram merging)
		 */
		double mixLambda1 = 1.0 - mixLambda - mixLambda2 - mixLambda3 -
					mixLambda4 - mixLambda5 - mixLambda6 -
					mixLambda7 - mixLambda8 - mixLambda9;

		ngramLM = (Ngram *)makeMixLM(mixFile, *vocab, classVocab,
					order, ngramLM, mixLambda1,
					mixLambda + mixLambda1);

		if (mixFile2) {
		    ngramLM = (Ngram *)makeMixLM(mixFile2, *vocab, classVocab,
					order, ngramLM, mixLambda2,
					mixLambda + mixLambda1 + mixLambda2);
		}
		if (mixFile3) {
		    ngramLM = (Ngram *)makeMixLM(mixFile3, *vocab, classVocab,
					order, ngramLM, mixLambda3,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3);
		}
		if (mixFile4) {
		    ngramLM = (Ngram *)makeMixLM(mixFile4, *vocab, classVocab,
					order, ngramLM, mixLambda4,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4);
		}
		if (mixFile5) {
		    ngramLM = (Ngram *)makeMixLM(mixFile5, *vocab, classVocab,
					order, ngramLM, mixLambda5,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5);
		}
		if (mixFile6) {
		    ngramLM = (Ngram *)makeMixLM(mixFile6, *vocab, classVocab,
					order, ngramLM, mixLambda6,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5 +
					mixLambda6);
		}
		if (mixFile7) {
		    ngramLM = (Ngram *)makeMixLM(mixFile7, *vocab, classVocab,
					order, ngramLM, mixLambda7,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5 +
					mixLambda6 + mixLambda7);
		}
		if (mixFile8) {
		    ngramLM = (Ngram *)makeMixLM(mixFile8, *vocab, classVocab,
					order, ngramLM, mixLambda8,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5 +
					mixLambda6 + mixLambda7 + mixLambda8);
		}
		if (mixFile9) {
		    ngramLM = (Ngram *)makeMixLM(mixFile9, *vocab, classVocab,
					order, ngramLM, mixLambda9, 1.0);
		}
	    }

	    /*
	     * Renormalize before the optional steps below, in case input
	     * model needs it, and because class expansion and pruning already
	     * include normalization.
	     */
	    if (renormalize) {
		ngramLM->recomputeBOWs();
	    }

	    /*
	     * Read class definitions from command line AFTER the LM, so
	     * they can override embedded class definitions.
	     */
	    if (classesFile != 0) {
		File file(classesFile, "r");
		((ClassNgram *)ngramLM)->readClasses(file);
	    }

	    if (expandClasses >= 0) {
		/*
		 * Replace class ngram with equivalent word ngram
		 * expandClasses == 0 generates all ngrams
		 * expandClasses > 0 generates only ngrams up to given length
		 */
		Ngram *newLM =
		    ((ClassNgram *)ngramLM)->expand(expandClasses, expandExact);

		delete ngramLM;
		ngramLM = newLM;
	    }

	    if (prune != 0.0) {
		if (pruneHistoryLM != 0) {
		    File file(pruneHistoryLM, "r");

		    /*
		     * Note the history LM needs one less word of context
		     * than the LM being pruned.
		     */
		    Ngram historyLM(*vocab, order > 1 ? order - 1 : 1);

		    if (!historyLM.read(file, limitVocab)) {
			cerr << "format error in history LM file\n";
			exit(1);
		    }

		    ngramLM->pruneProbs(prune, minprune, &historyLM);
		} else {
		    ngramLM->pruneProbs(prune, minprune);
		}
	    }

	    if (pruneLowProbs) {
		ngramLM->pruneLowProbs(minprune);
	    }

	    useLM = ngramLM;
	}
    } else {
	cerr << "need at least an -lm file specified\n";
	exit(1);
    }

    if (mixFile && !loglinearMix && bayesLength >= 0) {
	/*
	 * create a Bayes (linear) mixture LM 
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
	    useLM = makeMixLM(mixFile7, *vocab, classVocab, order, useLM,
				mixLambda7,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6 + mixLambda7);
	}
	if (mixFile8) {
	    useLM = makeMixLM(mixFile8, *vocab, classVocab, order, useLM,
				mixLambda8,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6 + mixLambda7 + mixLambda8);
	}
	if (mixFile9) {
	    useLM = makeMixLM(mixFile9, *vocab, classVocab, order, useLM,
				mixLambda9, 1.0);
	}
    } else if (mixFile && loglinearMix) {
	/*
	 * Create log-linear mixture LM
	 */
	double mixLambda1 = 1.0 - mixLambda - mixLambda2 - mixLambda3
			    - mixLambda4 - mixLambda5 - mixLambda6 - mixLambda7
			    - mixLambda8 - mixLambda9;

	Array<const char *> filenames;
	Array<double> lambdas;

	/* Add redundant filename entry for base LM to make filenames array 
	 * symmetric with lambdas */
	filenames[0] = "";
	filenames[1] = mixFile;
	lambdas[0] = mixLambda;
	lambdas[1] = mixLambda1;

	if (mixFile2) {
	    filenames[2] = mixFile2;
	    lambdas[2] = mixLambda2;
	}
	if (mixFile3) {
	    filenames[3] = mixFile3;
	    lambdas[3] = mixLambda3;
	}
	if (mixFile4) {
	    filenames[4] = mixFile4;
	    lambdas[4] = mixLambda4;
	}
	if (mixFile5) {
	    filenames[5] = mixFile5;
	    lambdas[5] = mixLambda5;
	}
	if (mixFile6) {
	    filenames[6] = mixFile6;
	    lambdas[6] = mixLambda6;
	}
	if (mixFile7) {
	    filenames[7] = mixFile7;
	    lambdas[7] = mixLambda7;
	}
	if (mixFile8) {
	    filenames[8] = mixFile8;
	    lambdas[8] = mixLambda8;
	}
	if (mixFile9) {
	    filenames[9] = mixFile9;
	    lambdas[9] = mixLambda9;
	}
	useLM = makeLoglinearMixLM(filenames, *vocab, classVocab, order,
					useLM, lambdas);
    }    

    if (cache > 0) {
	/*
	 * Create a mixture model with the cache lm as the second component
	 */
	CacheLM *cacheLM = new CacheLM(*vocab, cache);
	assert(cacheLM != 0);

	BayesMix *mixLM = new BayesMix(*vocab, *useLM, *cacheLM,
						0, 1.0 - cacheLambda, 0.0);
	assert(mixLM != 0);

        useLM = mixLM;
    }

    if (dynamic) {
	/*
	 * Create a mixture model with the dynamic lm as the second component
	 */
	DynamicLM *dynamicLM = new DynamicLM(*vocab);
	assert(dynamicLM != 0);

	BayesMix *mixLM = new BayesMix(*vocab, *useLM, *dynamicLM,
						0, 1.0 - dynamicLambda, 0.0);
	assert(mixLM != 0);

        useLM = mixLM;
    }

    if (adaptMarginals != 0) {
	/* 
	 * Adapt base LM to adaptive marginals given by unigram LM
	 */
	Ngram *adaptMargLM = new Ngram(*vocab, 1);
	assert(adaptMargLM != 0);

	{
	    File file(adaptMarginals, "r");
	    adaptMargLM->read(file);
	}

	LM *baseMargLM;

	if (baseMarginals == 0) {
	    baseMargLM = useLM;
	} else {
	    baseMargLM = new Ngram(*vocab, 1);
	    assert(baseMargLM != 0);

	    File file(baseMarginals, "r");
	    baseMargLM->read(file);
	}

	AdaptiveMarginals *adaptLM =
		new AdaptiveMarginals(*vocab, *useLM, *baseMargLM,
					*adaptMargLM, adaptMarginalsBeta);
	if (adaptMarginalsRatios) {
	    adaptLM->computeRatios = true;
	}
	assert(adaptLM != 0);
	useLM = adaptLM;
    }

    /*
     * Reverse words in scoring
     */
    if (reverseSents) {
	useLM->reverseWords = true;
    }

    /*
     * Control insertion of <s> and </s>
     */
    if (noSOS) {
    	useLM->addSentStart = false;
    }
    if (noEOS) {
    	useLM->addSentEnd = false;
    }

    /*
     * Skip noise tags in scoring
     */
    if (noiseVocabFile) {
	File file(noiseVocabFile, "r");
	useLM->noiseVocab.read(file);
    }
    if (noiseTag) {				/* backward compatibility */
	useLM->noiseVocab.addWord(noiseTag);
    }

    if (memuse) {
	MemStats memuse;
	useLM->memStats(memuse);
	memuse.print();
    }

    /*
     * Apply multiword wrapper if requested
     */
    if (multiwords) {
	useLM = new MultiwordLM(*(MultiwordVocab *)vocab, *useLM);
	assert(useLM != 0);
    }

    if (zeroprobWord) {
	useLM = new NonzeroLM(*vocab, *useLM, zeroprobWord);
	assert(useLM != 0);
    }

    /*
     * Rescore N-gram probs in LM file
     */
    if (rescoreNgramFile) {
	// create new vocab to avoid including class and multiwords
	// from the rescoring LM
	SubVocab *ngramVocab = new SubVocab(*vocab);
	assert(ngramVocab != 0);

	// read N-gram to be rescored
	Ngram *rescoreLM = new Ngram(*ngramVocab, order);
	assert(rescoreLM != 0);

	File file(rescoreNgramFile, "r");
	rescoreLM->read(file);

	rescoreLM->rescoreProbs(*useLM);

	// free memory for LMs used in rescoring
	if (ngramLM != useLM) {
	    delete ngramLM;
	    ngramLM = 0;
	}
	delete useLM;

	// use rescored LM below
	useLM = rescoreLM;
    }

    /*
     * Compute perplexity on a text file, if requested
     */
    if (pplFile) {
	File file(pplFile, "r");
	TextStats stats;

	/*
	 * Send perplexity info to stdout 
	 */
	useLM->dout(cout);
	useLM->pplFile(file, stats, escape);
	useLM->dout(cerr);

	cout << "file " << pplFile << ": " << stats;
    }

    /*
     * Compute perplexity on a count file, if requested
     */
    if (countFile) {
	TextStats stats;
	File file(countFile, "r");

	/*
	 * Send perplexity info to stdout 
	 */
	useLM->dout(cout);
	if (useFloatCounts) {
	    useLM->pplCountsFile(file, countOrder ? countOrder : order,
					stats, escape, countEntropy,
					(NgramCounts<FloatCount> *)0);
	} else {
	    useLM->pplCountsFile(file, countOrder ? countOrder : order,
					stats, escape, countEntropy,
					(NgramStats *)0);
	}
	useLM->dout(cerr);

	cout << "file " << countFile << ": " << stats;
    }

    /*
     * Rescore N-best list, if requested
     */
    if (nbestFile) {
	rescoreNbest(*useLM, nbestFile, NULL);
    }

    /*
     * Rescore multiple N-best lists
     */
    if (nbestFiles) {
	File file(nbestFiles, "r");

	char *line;
	while ((line = file.getline())) {
	    char *fname = strtok(line, wordSeparators);
	    if (!fname) continue;

	    RefString sentid = idFromFilename(fname);

	    if (writeNbestDir) {
		makeArray(char, scoreFile,
			  strlen(writeNbestDir) + 1
				 + strlen(sentid) + strlen(GZIP_SUFFIX) + 1);
		sprintf(scoreFile, "%s/%s%s", writeNbestDir, sentid,
								GZIP_SUFFIX);
		rescoreNbest(*useLM, fname, scoreFile);
	    } else {
		rescoreNbest(*useLM, fname, NULL);
	    }
	}
    }

    /*
     * Rescore stream of N-best hyps, if requested
     */
    if (rescoreFile) {
	File file(rescoreFile, "r");

	LM *oldLM;

	if (decipherLM) {
	    oldLM =
		 new DecipherNgram(*vocab, decipherOrder, !decipherNoBackoff);
	    assert(oldLM != 0);

	    File file(decipherLM, "r");

	    if (!oldLM->read(file, limitVocab)) {
		cerr << "format error in Decipher LM\n";
		exit(1);
	    }
	} else {
	    /*
	     * Create dummy LM for the sake of rescoreFile()
	     */
	    oldLM = new NullLM(*vocab);
	    assert(oldLM != 0);
	}

	useLM->rescoreFile(file, rescoreLMW, rescoreWTW,
				*oldLM, decipherLMW, decipherWTW, escape);

#ifdef DEBUG
	delete oldLM;
#endif
    }

    if (generateSents) {
	File outFile(stdout);
	unsigned i;

	for (i = 0; i < generateSents; i++) {
	    VocabString *sent = useLM->generateSentence(maxWordsPerLine,
							(VocabString *)0);
	    Vocab::write(outFile, sent);
	    putchar('\n');
	}
    }

    if (generatePrefixFile) {
	File inFile(generatePrefixFile, "r");
	File outFile(stdout);

	VocabString prefix[maxWordsPerLine+1];

	char *line;
	while ((line = inFile.getline())) {
	    unsigned numWords =
			vocab->parseWords(line, prefix, maxWordsPerLine);
	    prefix[numWords] = 0;

	    VocabString *sent = useLM->generateSentence(maxWordsPerLine,
							(VocabString *)0,
							prefix);
	    Vocab::write(outFile, sent);
	    putchar('\n');
	}
    }

    if (writeLM) {
	File file(writeLM, "w");
	if (!useLM->write(file)) {
	    cerr << "error writing " << writeLM << endl;
	}
    }

    if (writeBinLM) {
	File file(writeBinLM, "wb");
        if (!useLM->writeBinary(file)) {
	    cerr << "error writing " << writeBinLM << endl;
	}
    }

    if (writeBinV1LM) {
	File file(writeBinV1LM, "w");
        if (!((Ngram *)useLM)->writeBinaryV1(file)) {
	    cerr << "error writing " << writeBinV1LM << endl;
	}
    }

    if (writeVocab) {
	File file(writeVocab, "w");
	vocab->write(file);
    }

    if (serverPort > 0) {
	cerr << "starting prob server on port " << serverPort << endl;
    	useLM->probServer(serverPort, serverMaxClients);
	// never returns !
    }

#ifdef DEBUG
    if (&ngramLM->vocab != vocab) {
	delete &ngramLM->vocab;
    }
    if (ngramLM != useLM) {
	delete ngramLM;
    }
    delete useLM;

    delete stopWords;
    delete hiddenEvents;
    delete classVocab;
    delete vocab;

    return 0;
#endif /* DEBUG */

    exit(0);
}

