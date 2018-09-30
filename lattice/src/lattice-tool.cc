/*
 * lattice-tool --
 *	Manipulate word lattices
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1997-2011 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Id: lattice-tool.cc,v 1.156 2011/04/21 06:12:49 stolcke Exp $";
#endif

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <stdio.h>
#include <math.h>
#include <locale.h>
#include <errno.h>
#ifndef _MSC_VER
#include <sys/time.h>
#include <unistd.h>
#endif
#include <signal.h>
#include <setjmp.h>

#ifndef SIGALRM
#define NO_TIMEOUT
#endif

#include "option.h"
#include "version.h"
#include "Vocab.h"
#include "MultiwordVocab.h"
#include "ProductVocab.h"
#include "Lattice.h"
#include "MultiwordLM.h"
#include "NonzeroLM.h"
#include "Ngram.h"
#include "LMClient.h"
#include "ClassNgram.h"
#include "SimpleClassNgram.h"
#include "ProductNgram.h"
#include "BayesMix.h"
#include "LoglinearMix.h"
#include "RefList.h"
#include "LatticeLM.h"
#include "WordMesh.h"
#include "zio.h"
#include "mkdir.h"

#define DebugPrintFunctionality 1	// same as in Lattice.cc

static int haveError = 0;
static int version = 0;
static int compactExpansion = 0; 
static int oldExpansion = 0; 
static int base = 0; 
static int density = 0; 
static int connectivity = 0; 
static int compactPause = 0; 
static int noBackoffWeights = 0;
static int nodeEntropy = 0; 
static int oldDecoding = 0;
static int viterbiDecode = 0;
static int nbestDecode = 0;
static int nbestViterbi = 0;
static unsigned nbestDuplicates = 0;
static int nbestMaxHyps = 0;
static int outputCTM = 0;
static int computePosteriors = 0;
static char *writePosteriors = 0;
static char *writePosteriorsDir = 0;
static char *writeMesh = 0;
static char *writeMeshDir = 0;
static char *initMesh = 0;
static char *writeNgrams = 0;
static char *writeNgramIndex = 0;
static double minCount = 0.0;
static double ngramsMaxPause = 0.0;
static double ngramsTimeTolerance = 0.0;
static int acousticMesh = 0;
static int posteriorDecode = 0;
static double posteriorScale = 8.0;
static double beamwidth = 0.0;
static unsigned maxDegree = 0;
static double posteriorPruneThreshold = 0.0;
static double densityPruneThreshold = 0.0;
static unsigned nodesPruneThreshold = 0;
static int fastPrune = 0;
static int reduceBeforePruning = 0;
static int noPause = 0; 
static int insertPause = 0; 
static int noNulls = 0; 
static int loopPause = 0;
static int overwrite = 0; 
static int simpleReduction = 0; 
static int overlapBase = 0;
static double overlapRatio = 0.0;
static int dag = 0; 
static char *dictFile = 0;
static int dictAlign = 0;
static int intlogs = 0;
static char *lmFile  = 0;
static char *useServer = 0;
static int cacheServedNgrams = 0;
static char *vocabFile = 0;
static char *vocabAliasFile = 0;
static char *noneventFile = 0;
static char *hiddenVocabFile = 0;
static int limitVocab = 0;
static int useUnk = 0;
static int keepUnk = 0;
static int printSentTags = 0;
static char *mapUnknown = 0;
static char *zeroprobWord = 0;
static char *classesFile = 0;
static int simpleClasses = 0;
static int factored = 0;
static int keepNullFactors = 1;
static char *mixFile  = 0;
static char *mixFile2 = 0;
static char *mixFile3 = 0;
static char *mixFile4 = 0;
static char *mixFile5 = 0;
static char *mixFile6 = 0;
static char *mixFile7 = 0;
static char *mixFile8 = 0;
static char *mixFile9 = 0;
static int bayesLength = 0;
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
static char *inLattice = 0; 
static char *inLattice2 = 0; 
static char *inLatticeList = 0; 
static char *outLattice = 0; 
static char *outLatticeDir = 0;
static char *refFile = 0; 
static char *refList = 0; 
static char *writeRefs = 0;
static double addRefsProb = 0.0;
static int keepPause = 0;
static char *noiseVocabFile = 0;
static char *ignoreVocabFile = 0;
static char *indexName = 0; 
static char *operation = 0; 
static double interSegmentTime = 0.0;
static int order = 3;
static int noExpansion = 0;
static int simpleReductionIter = 0; 
static int preReductionIter = 0; 
static int postReductionIter = 0; 
static int collapseSameWords = 0;
static int debug = 0;
static int readHTK = 0; 
static int writeHTK = 0;
static int useHTKnulls = 1;
static int readMesh = 0;
static int writeInternal = 0;
static int maxTime = 0;
static unsigned maxNodes = 0;
static int splitMultiwords = 0;
static int splitMultiwordsAfterLM = 0;
static char *multiwordDictFile = 0;
static int toLower = 0;
static int useMultiwordLM = 0;
static const char *multiChar = MultiwordSeparator;
static char *pplFile = 0;
static char *writeVocabFile = 0;
static char *wordPosteriorsFile = 0;
static HTKHeader htkparms(HTK_undef_float, HTK_undef_float, HTK_undef_float,
			  HTK_undef_float, HTK_undef_float, HTK_undef_float,
			  HTK_undef_float, HTK_undef_float, HTK_undef_float,
			  HTK_undef_float, HTK_undef_float, HTK_undef_float,
			  HTK_undef_float, HTK_undef_float, HTK_undef_float);
static NBestOptions nbestOut(0,0,0,0,0,0,0,0,0,0,0,0,0,0);

static Option options[] = {
    { OPT_TRUE, "version", &version, "print version information" },
    { OPT_TRUE, "in-lattice-dag", &dag, "input lattices are defined in a directed acyclic graph" },
    { OPT_STRING, "in-lattice", &inLattice, "input lattice for lattice operation including expansion or bigram weight substitution" },
    { OPT_STRING, "in-lattice2", &inLattice2, "a second input lattice for lattice operation" },
    { OPT_STRING, "in-lattice-list", &inLatticeList, "input lattice list for expansion or bigram weight substitution" },
    { OPT_STRING, "out-lattice", &outLattice, "resulting output lattice" },
    { OPT_STRING, "out-lattice-dir", &outLatticeDir, "resulting output lattice dir" },
    { OPT_STRING, "dictionary", &dictFile, "pronunciation dictionary" },
    { OPT_TRUE, "dictionary-align", &dictAlign, "use pronunciation dictionary in alignment for posterior decoding" },
    { OPT_TRUE, "intlogs", &intlogs, "dictionary uses intlog probabilities" },
    { OPT_STRING, "lm", &lmFile, "LM used for expansion or weight substitution" },
    { OPT_STRING, "use-server", &useServer, "port@host to use as LM server" },
    { OPT_TRUE, "cache-served-ngrams", &cacheServedNgrams, "enable client side caching" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_STRING, "vocab-aliases", &vocabAliasFile, "vocab alias file" },
    { OPT_STRING, "nonevents", &noneventFile, "non-event vocabulary" },
    { OPT_STRING, "hidden-vocab", &hiddenVocabFile, "subvocabulary to keep separate in lattice alignment" },
    { OPT_TRUE, "limit-vocab", &limitVocab, "limit LM reading to specified vocabulary" },
    { OPT_TRUE, "unk", &useUnk, "map unknown words to <unk>" },
    { OPT_TRUE, "keep-unk", &keepUnk, "preserve unknown word labels in output" },
    { OPT_STRING, "map-unk", &mapUnknown, "word to map unknown words to" },
    { OPT_STRING, "zeroprob-word", &zeroprobWord, "word to back off to for zero probs" },
    { OPT_TRUE, "print-sent-tags", &printSentTags, "preserve sentence begin/end tags in output" },
    { OPT_STRING, "classes", &classesFile, "class definitions" },
    { OPT_TRUE, "simple-classes", &simpleClasses, "use unique class model" },
    { OPT_TRUE, "factored", &factored, "use a factored LM" },
    { OPT_FALSE, "no-null-factors", &keepNullFactors, "remove <NULL> in factored LM" },
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
    { OPT_INT, "order", &order, "ngram order used for expansion or bigram weight substitution" },
    { OPT_TRUE, "no-expansion", &noExpansion, "do not apply expansion with LM" },
    { OPT_STRING, "ref-list", &refList, "reference file used for computing WER (lines starting with utterance id)" }, 
    { OPT_STRING, "ref-file", &refFile, "reference file used for computing WER (utterances in same order in lattice list)" },
    { OPT_FLOAT, "add-refs", &addRefsProb, "add reference words to lattice with given probability" },
    { OPT_STRING, "write-refs", &writeRefs, "output references to file (for validation)" },
    { OPT_STRING, "ppl", &pplFile, "compute perplexity according to lattice" },
    { OPT_STRING, "write-vocab", &writeVocabFile, "output lattice vocabulary" },
    { OPT_TRUE, "keep-pause", &keepPause, "treat pauses as regular word for WER computation and decoding" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary to ignore in WER computation and decoding" },
    { OPT_STRING, "ignore-vocab", &ignoreVocabFile, "pause-like words to ignore in lattice operations" },
    { OPT_TRUE, "overwrite", &overwrite, "overwrite existing output lattice dir" }, 
    { OPT_TRUE, "reduce", &simpleReduction, "reduce bigram lattice(s) using the simple algorithm" },
    { OPT_INT, "reduce-iterate", &simpleReductionIter, "reduce input lattices iteratively" },
    { OPT_INT, "pre-reduce-iterate", &preReductionIter, "reduce pause-less lattices iteratively" },
    { OPT_INT, "post-reduce-iterate", &postReductionIter, "reduce output lattices iteratively" },
    { OPT_TRUE, "reduce-before-pruning", &reduceBeforePruning, "apply posterior pruning after lattice reduction" },
    { OPT_FLOAT, "overlap-ratio", &overlapRatio, "if two incoming/outgoing node sets of two given nodes with the same lable overlap beyong this ratio, they are merged" }, 
    { OPT_INT, "overlap-base", &overlapBase, "use the smaller (0) incoming/outgoing node set to compute overlap ratio, or the larger (1) set to compute the overlap ratio" },
    { OPT_TRUE, "compact-expansion", &compactExpansion, "use compact LM expansion algorithm (using backoff nodes)" },
    { OPT_TRUE, "topo-compact-expansion", &compactExpansion, "(same as above, for backward compatibility)" },
    { OPT_TRUE, "old-expansion", &oldExpansion, "use old unigram/bigram/trigram expansion algorithms" },
    { OPT_TRUE, "no-backoff-weights", &noBackoffWeights, "suppress backoff weights in lattice exansion (a hack)" },
    { OPT_TRUE, "multiwords", &useMultiwordLM, "use multiword wrapper LM" },
    { OPT_TRUE, "split-multiwords", &splitMultiwords, "split multiwords into separate nodes" },
    { OPT_TRUE, "split-multiwords-after-lm", &splitMultiwordsAfterLM, "split multiwords after LM expansion" },
    { OPT_STRING, "multiword-dictionary", &multiwordDictFile, "multiword splitting dictionary" },
    { OPT_STRING, "multi-char", &multiChar, "multiword component delimiter" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lower case" },
    { OPT_STRING, "operation", &operation, "conventional lattice operations, including \"concatenate\" and \"or\"" }, 
    { OPT_FLOAT, "inter-segment-time", &interSegmentTime, "pause length to insert between concatenated lattices" },
    { OPT_TRUE, "density", &density, "compute densities of lattices" }, 
    { OPT_TRUE, "connectivity", &connectivity, "check the connectivity of given lattices" }, 
    { OPT_TRUE, "compute-node-entropy", &nodeEntropy, "compute the node entropy of given lattices" }, 
    { OPT_TRUE, "compute-posteriors", &computePosteriors, "compute the node posteriors of given lattices" }, 
    { OPT_STRING, "write-posteriors", &writePosteriors, "write posterior lattice format to this file" }, 
    { OPT_STRING, "write-posteriors-dir", &writePosteriorsDir, "write posterior lattices to this directory" }, 
    { OPT_STRING, "word-posteriors-for-sentences", &wordPosteriorsFile, "compute word-level posteriors for sentences in this file" },
    { OPT_STRING, "write-mesh", &writeMesh, "write posterior mesh (sausage) to this file" }, 
    { OPT_STRING, "write-mesh-dir", &writeMeshDir, "write posterior meshes to this directory" }, 
    { OPT_STRING, "init-mesh", &initMesh, "align lattice to initial mesh from this file" }, 
    { OPT_TRUE, "acoustic-mesh", &acousticMesh, "record acoustic information in word meshes" }, 
    { OPT_TRUE, "posterior-decode", &posteriorDecode, "decode best words from posterior mesh" }, 
    { OPT_FLOAT, "posterior-prune", &posteriorPruneThreshold, "posterior node pruning threshold" }, 
    { OPT_FLOAT, "posterior-scale", &posteriorScale, "posterior scaling factor" }, 
    { OPT_FLOAT, "density-prune", &densityPruneThreshold, "max lattice density for pruning" }, 
    { OPT_UINT, "nodes-prune", &nodesPruneThreshold, "max number of real nodes for pruning" }, 
    { OPT_TRUE, "fast-prune", &fastPrune, "fast posterior pruning (no posterior recomputation)" }, 
    { OPT_TRUE, "old-decoding", &oldDecoding, "use old latitice 1best/nbest decoding algorithms" },
    { OPT_TRUE, "viterbi-decode", &viterbiDecode, "output words on highest probability path" },
    { OPT_FLOAT, "decode-beamwidth", &beamwidth, "decoding beamwidth" },
    { OPT_INT, "decode-max-degree", &maxDegree, "maximum allowed in degree in the search graph" },
    { OPT_INT, "nbest-decode", &nbestDecode, "number of nbest hyps to generate from lattice" },    
    { OPT_INT, "nbest-max-stack", &nbestMaxHyps, "max stack size for nbest generation" },    
    { OPT_TRUE, "nbest-viterbi", &nbestViterbi, "use Viterbi algorithm to generate nbest (instead of A-star)" },    
    { OPT_UINT, "nbest-duplicates", &nbestDuplicates, "number of hyps to output per unique word string (words in -noise-words may or may not differ)" },    
    { OPT_TRUE, "output-ctm", &outputCTM, "output decoded words in CTM format" },
    { OPT_STRING, "write-ngrams", &writeNgrams, "write expected ngram counts to file" }, 
    { OPT_STRING, "write-ngram-index", &writeNgramIndex, "write ngram index to file" }, 
    { OPT_FLOAT, "min-count", &minCount, "prune ngram counts below this value" },
    { OPT_FLOAT, "ngrams-max-pause", &ngramsMaxPause, "maximum pause duration allowed inside indexed ngrams" },
    { OPT_FLOAT, "ngrams-time-tolerance", &ngramsTimeTolerance, "timestamp tolerance for ngram indexing" },
    { OPT_STRING, "index-name", &indexName, "print a list of node index-name pairs to this file" },
    { OPT_TRUE, "no-pause", &noPause, "output lattices with no pauses" },
    { OPT_TRUE, "insert-pause", &insertPause, "insert optional pauses" },
    { OPT_TRUE, "no-nulls", &noNulls, "eliminate null nodes" },
    { OPT_TRUE, "compact-pause", &compactPause, "output lattices with compact pauses" },
    { OPT_TRUE, "loop-pause", &loopPause, "output lattices with loop pauses" },
    { OPT_TRUE, "collapse-same-words", &collapseSameWords, "collapse nodes with same words" },
    { OPT_INT, "debug", &debug, "debug level" },
    { OPT_TRUE, "read-htk", &readHTK, "read input lattices in HTK format" },
    { OPT_TRUE, "write-htk", &writeHTK, "write output lattices in HTK format" },
    { OPT_FALSE, "no-htk-nulls", &useHTKnulls, "don't use null nodes to encode HTK lattices" },
    { OPT_TRUE, "read-mesh", &readMesh, "read input lattices in word mesh format" },
    { OPT_TRUE, "write-internal", &writeInternal, "write out internal node numbering" },
#ifndef NO_TIMEOUT
    { OPT_UINT, "max-time", &maxTime, "maximum no. of seconds allowed per lattice" },
#endif
    { OPT_UINT, "max-nodes", &maxNodes, "maximum no. of nodes allowed in expanding lattice" },
    { OPT_FLOAT, "htk-acscale", &htkparms.acscale, "HTK acscale override" }, 
    { OPT_FLOAT, "htk-lmscale", &htkparms.lmscale, "HTK lmscale override" }, 
    { OPT_FLOAT, "htk-ngscale", &htkparms.ngscale, "HTK ngscale override" }, 
    { OPT_FLOAT, "htk-prscale", &htkparms.prscale, "HTK prscale override" }, 
    { OPT_FLOAT, "htk-duscale", &htkparms.duscale, "HTK duscale override" }, 
    { OPT_FLOAT, "htk-wdpenalty", &htkparms.wdpenalty, "HTK wdpenalty override" }, 
    { OPT_FLOAT, "htk-x1scale", &htkparms.x1scale, "HTK xscore1 override" }, 
    { OPT_FLOAT, "htk-x2scale", &htkparms.x2scale, "HTK xscore2 override" }, 
    { OPT_FLOAT, "htk-x3scale", &htkparms.x3scale, "HTK xscore3 override" }, 
    { OPT_FLOAT, "htk-x4scale", &htkparms.x4scale, "HTK xscore4 override" }, 
    { OPT_FLOAT, "htk-x5scale", &htkparms.x5scale, "HTK xscore5 override" }, 
    { OPT_FLOAT, "htk-x6scale", &htkparms.x6scale, "HTK xscore6 override" }, 
    { OPT_FLOAT, "htk-x7scale", &htkparms.x7scale, "HTK xscore7 override" }, 
    { OPT_FLOAT, "htk-x8scale", &htkparms.x8scale, "HTK xscore8 override" }, 
    { OPT_FLOAT, "htk-x9scale", &htkparms.x9scale, "HTK xscore9 override" }, 
    { OPT_FLOAT, "htk-logbase", &htkparms.logbase, "base for HTK log scores" }, 
    { OPT_FLOAT, "htk-logzero", &HTK_LogP_Zero, "pseudo HTK score value for -infinity" },
    { OPT_TRUE, "htk-words-on-nodes", &htkparms.wordsOnNodes, "HTK lattices output with words on nodes" }, 
    { OPT_TRUE, "htk-scores-on-nodes", &htkparms.scoresOnNodes, "HTK lattices output with acoustic scores on nodes" }, 
    { OPT_TRUE, "htk-quotes", &htkparms.useQuotes, "use quotes in HTK lattices" }, 
    { OPT_STRING, "out-nbest-dir", &nbestOut.nbestOutDir, "resulting nbest list dir" },
    { OPT_STRING, "out-nbest-dir-ngram", &nbestOut.nbestOutDirNgram, "resulting nbest list ngram score dir" },
    { OPT_STRING, "out-nbest-dir-pron", &nbestOut.nbestOutDirPron, "resulting nbest list pron score dir" },
    { OPT_STRING, "out-nbest-dir-dur", &nbestOut.nbestOutDirDur, "resulting nbest list duration score dir" },
    { OPT_STRING, "out-nbest-dir-xscore1", &nbestOut.nbestOutDirXscore1, "resulting nbest list xscore1 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore2", &nbestOut.nbestOutDirXscore2, "resulting nbest list xscore2 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore3", &nbestOut.nbestOutDirXscore3, "resulting nbest list xscore3 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore4", &nbestOut.nbestOutDirXscore4, "resulting nbest list xscore4 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore5", &nbestOut.nbestOutDirXscore5, "resulting nbest list xscore5 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore6", &nbestOut.nbestOutDirXscore6, "resulting nbest list xscore6 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore7", &nbestOut.nbestOutDirXscore7, "resulting nbest list xscore7 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore8", &nbestOut.nbestOutDirXscore8, "resulting nbest list xscore8 score dir" },
    { OPT_STRING, "out-nbest-dir-xscore9", &nbestOut.nbestOutDirXscore9, "resulting nbest list xscore9 score dir" },
    { OPT_STRING, "out-nbest-dir-rttm", &nbestOut.nbestOutDirRttm, "resulting nbest hyps output in rttm format (with extra preceding column that gives hyp number)" }
};

/*
 * Output hypotheses in CTM format
 */
static void
printCTM(Vocab &vocab, const NBestWordInfo *winfo, const char *name)
{
    for (unsigned i = 0; winfo[i].word != Vocab_None; i ++) {
	cout << name << " 1 ";
	if (winfo[i].valid()) {
	    cout << winfo[i].start << " " << winfo[i].duration;
	} else {
	    cout << "? ?";
	}
	cout << " " << vocab.getWord(winfo[i].word)
	     << " " << winfo[i].wordPosterior << endl;
    }
}

#ifndef NO_TIMEOUT
/*
 * deal with different signal hander types
 */
#ifndef _sigargs
#define _sigargs int
#endif
typedef void (*sighandler_t)(_sigargs);

static jmp_buf thisContext;

void catchAlarm(int signal)
{
    longjmp(thisContext, 1);
}
#endif /* !NO_TIMEOUT */


/*
 * Compute word-level posteriors for sentences in this file
 */
void
computeWordPosteriors(WordMesh &mesh, File &file)
{
    Lattice lat(mesh.vocab);
    lat.createFromMesh(mesh);

    makeArray(VocabString, words, maxWordsPerLine);
    makeArray(NodeIndex, path, 2 * maxWordsPerLine);
	
    while (char *line = file.getline()) {
	unsigned numWords = mesh.vocab.parseWords(line, words, maxWordsPerLine);

	if (!numWords) 
	    continue;

	LogP pathLogPosterior = 0.0;
	unsigned numNodes = lat.findBestPath(numWords, words, path, 2*maxWordsPerLine, pathLogPosterior);

	LogP sumPosterior = 0.0;

	if (numNodes) {
	    unsigned i, j;

	    for (i = 0, j = 0; i < numWords && j < numNodes; j++) {
		LatticeNode *nd = lat.findNode(path[j]);
		assert(nd != 0);
		if (nd->word == lat.vocab.getIndex(words[i])) {
		    assert(nd->htkinfo);
		    // log
		    LogP logPos = nd->htkinfo->xscore1;
		    Prob pos = LogPtoProb(logPos);

		    cout << words[i] << " (" << pos << ") ";
		    i++;
		    sumPosterior += pos;
		}
	    }
	    cout << "(Average posterior: " << sumPosterior/numWords << ")\n";
	} else {
	    // not in lattice
	    for (unsigned i = 0; i < numWords; i++) {
		cout << words[i] << " (0) ";
	    }
	    cout << "(Average posterior: 0)\n";
	}
    }
}

void processLattice(char *inLat, char *outLat, Lattice *lattice2,
		    NgramCounts<FloatCount> &ngramCounts,
		    File *ngramIndexFile,
		    LM &lm, Vocab &vocab, SubVocab &hiddenVocab,
		    VocabMultiMap &dictionary,
		    LHash<const char*, Array< Array<char*>* > > &multiwordDict,
		    SubVocab &ignoreWords, SubVocab &noiseWords,
		    VocabIndex *refIndices = 0)
{
    Lattice lat(vocab, idFromFilename(inLat), ignoreWords); 
    lat.debugme(debug);

    if (useUnk) lat.useUnk = true;
    if (keepUnk) lat.keepUnk = true;
    if (printSentTags) lat.printSentTags = true;

    {
	File file(inLat, "r", 0);

	if (file.error()) {
	    cerr << "error opening ";
	    perror(inLat);
	    haveError = 1;
	    return;
	}

	Boolean status;

	if (readHTK) {
	    htkparms.amscale = posteriorScale;
	    status = lat.readHTK(file, &htkparms, useHTKnulls);
	} else if (readMesh) {
	    lat.setHTKHeader(htkparms);
	    status = lat.readMesh(file);
	} else {
	    lat.setHTKHeader(htkparms);
	    status = lat.readPFSGs(file);
	}
	if (!status) {
	    cerr << "error reading " << inLat << endl;
	    haveError = 1;
	    return;
	}
    }

#ifndef NO_TIMEOUT
    if (maxTime) {
	alarm(maxTime);
	if (setjmp(thisContext)) {
	    cerr << "WARNING: processing lattice " << inLat
	         << " aborted after " << maxTime << " seconds\n";
	    return;
	}
	signal(SIGALRM, (sighandler_t)catchAlarm);
    }
#endif /* !NO_TIMEOUT */

    if (dictFile && !dictAlign) {
	// pronunciation scoring (only useful for HTK lattices)
	// do this BEFORE splitting multiwords since pronunciations apply to
	// the original multiwords
	if (!lat.scorePronunciations(dictionary, intlogs)) {
	    cerr << "WARNING: error scoring pronunciations for " << inLat
		 << endl;
        }
    }

    if (splitMultiwords) {
    	if (multiwordDictFile) {
	    lat.splitHTKMultiwordNodes((MultiwordVocab &)vocab, multiwordDict);
	}
	lat.splitMultiwordNodes((MultiwordVocab &)vocab, lm);
    }

    if ((posteriorPruneThreshold > 0 ||
	 densityPruneThreshold > 0.0 ||
	 nodesPruneThreshold > 0) && !reduceBeforePruning)
    {
	if (!lat.prunePosteriors(posteriorPruneThreshold, posteriorScale,
				    densityPruneThreshold, nodesPruneThreshold,
				    fastPrune))
	{
	    cerr << "WARNING: posterior pruning of lattice " << inLat
	         << " failed\n";
#ifndef NO_TIMEOUT
	    alarm(0);
#endif
	    haveError = 1;
	    return;
        } 
    }

    if (writePosteriors) {
	File file(writePosteriors, "w");
	lat.writePosteriors(file, posteriorScale);
    }
    if (writePosteriorsDir) {
	makeArray(char, outfile,
		  strlen(writePosteriorsDir) + 1 +
		  strlen(lat.getName()) + sizeof(GZIP_SUFFIX));
	sprintf(outfile, "%s/%s%s", writePosteriorsDir,
					lat.getName(), GZIP_SUFFIX);

	File file(outfile, "w");
	lat.writePosteriors(file, posteriorScale);
    }

    if (viterbiDecode) {
        LM *plm = (lmFile || useServer) ? &lm : 0;

	if (outputCTM) {
	    NBestWordInfo *bestwords = new NBestWordInfo[maxWordsPerLine + 1];
	    assert(bestwords != 0);

	    LogP prob;
	    
	    if (oldDecoding) {
	        prob = lat.bestWords(bestwords, maxWordsPerLine, noiseWords);
	    } else {
	        prob = lat.decode1Best(bestwords, maxWordsPerLine, noiseWords, 
				       plm, order,
				       beamwidth > 0.0 ?
					   log10(beamwidth) * posteriorScale :
					   0.0);
	    }

	    if (prob != LogP_Zero || bestwords[0].word != Vocab_None) {
		printCTM(vocab, bestwords, lat.getName());
	    } 

	    delete [] bestwords;
	} else {
	    VocabIndex *bestwords = new VocabIndex [maxWordsPerLine + 1];
	    
	    LogP prob;

	    if (!oldDecoding) {
		prob = lat.decode1Best(bestwords, maxWordsPerLine, noiseWords,
				       plm, order,
				       beamwidth > 0.0 ?
					   log10(beamwidth) * posteriorScale :
					   0.0);
	    } else {
		prob = lat.bestWords(bestwords, maxWordsPerLine, noiseWords);
	    }

	    if (prob != LogP_Zero || bestwords[0] != Vocab_None) {
		cout << lat.getName() << " "
		     << (lat.vocab.use(), bestwords) << endl;
	    } else {
		cout << lat.getName()<< endl;
	    }
	    delete [] bestwords;
	}
    }

    if (nbestDecode > 0) {
	// output top N hyps
	nbestOut.openFiles(lat.getName());
	
	if (oldDecoding || nbestViterbi || nbestDuplicates > 0) {
  	    if (!oldDecoding) {
		cerr << "warning: using -old-decoding method\n";
	    }
	    
	    if (nbestViterbi) {
	  	lat.computeNBestViterbi(nbestDecode, nbestOut, noiseWords,
					useMultiwordLM ? multiChar : 0);
	    } else {
		lat.computeNBest(nbestDecode, nbestOut, noiseWords,
				 useMultiwordLM ? multiChar : 0,
				 nbestMaxHyps, nbestDuplicates);
	    }
	} else {
	    LM *plm = (lmFile || useServer) ? &lm : 0;	  
	    lat.decodeNBest(nbestDecode, nbestOut, noiseWords, plm,
	    		    order, maxDegree, 
			    beamwidth > 0.0 ?
				log10(beamwidth) * posteriorScale :
				0.0,
			    useMultiwordLM ? multiChar : 0);
	}
	
	nbestOut.closeFiles();    
    }

    if (computePosteriors) {
	lat.computePosteriors(posteriorScale, true);
    }
    
    if (density) {
	double d = lat.computeDensity();

	if (d == HUGE_VAL) {
	    cerr << "WARNING: duration for lattice " << inLat << " unknown\n";
	} else {
	    cout << lat.getName() << " " << lat.computeDensity() << endl;
	}
    }

    if (connectivity) {
	if (!lat.areConnected(lat.getInitial(), lat.getFinal())) {
	    cerr << "WARNING: lattice " << inLat << " is not connected\n";
#ifndef NO_TIMEOUT
	    alarm(0);
#endif
	    haveError = 1;
	    return;
	} 
    }

    if (nodeEntropy) { 
	lat.computeNodeEntropy();
    }

    if (indexName) { 
	File indexFile(indexName, "w");
	lat.printNodeIndexNamePair(indexFile); 
    }

    if (refFile || refList) {
	if (refIndices) {
	    unsigned numWords = vocab.length(refIndices); 
	    if (!numWords) {
	      cerr << "WARNING: reference has 0 length\n";
	    }

	    unsigned total, sub, ins, del;
	    total = lat.latticeWER(refIndices, sub, ins, del, noiseWords);
	    
	    cout << "sub " << sub 
		 << " ins " << ins
		 << " del " << del
		 << " wer " << total
		 << " words " << numWords
		 << endl;
	} else {
	    cerr << "WARNING: reference is missing for lattice "
		 << inLat << endl;
	    haveError = 1;
	}
    }

    if (addRefsProb != 0.0) {
	if (refIndices) {
	    lat.addWords(refIndices, addRefsProb, !noPause);
	} else if (!(refFile || refList)) {
	    cerr << "WARNING: reference is missing for lattice "
		 << inLat << endl;
	    haveError = 1;
	}
    }

    if (noNulls) {
	lat.removeAllXNodes(Vocab_None);
    }

    if (simpleReduction || simpleReductionIter) {
	cerr << "reducing input lattices (overlap ratio = "
	     << overlapRatio << ")\n"; 

	if (overlapRatio == 0.0) { 
	    // if reduceBeforePruning is specified then merged transitions probs
	    // should be addded, not maxed
	    lat.simplePackBigramLattice(simpleReductionIter,
	    				reduceBeforePruning); 
	} else {
	    lat.approxRedBigramLattice(simpleReductionIter, overlapBase,
	    				overlapRatio);
	}
    }

    if ((posteriorPruneThreshold > 0.0 ||
	 densityPruneThreshold > 0.0 ||
	 nodesPruneThreshold > 0) && reduceBeforePruning)
    {
	if (!lat.prunePosteriors(posteriorPruneThreshold, posteriorScale,
				    densityPruneThreshold, nodesPruneThreshold,
				    fastPrune))
	{
	    cerr << "WARNING: posterior pruning of lattice " << inLat
	         << " failed\n";
#ifndef NO_TIMEOUT
	    alarm(0);
#endif
	    haveError = 1;
	    return;
        } 
    }

    Boolean recoverPauses = false;

    if (noPause) {
	lat.removeAllXNodes(vocab.pauseIndex());
    }

    if (maxNodes > 0 && lat.getNumNodes() > maxNodes) {
      cerr << "WARNING: processing lattice " << inLat
	   << " aborted -- too many nodes after reduction: "
	   << lat.getNumNodes() << endl;
#ifndef NO_TIMEOUT
      alarm(0);
#endif
      return;
    }

    // by default we leave HTK lattices scores alone
    HTKScoreMapping htkScoreMapping = mapHTKnone;

    if (!readHTK) {
	// preserve PFSG weights as HTK acoustic scores
	htkScoreMapping = mapHTKacoustic;
    }

    if ((lmFile || useServer) && !noExpansion) {
	// remove pause and NULL nodes prior to LM application,
	// so each word has a proper predecessor
	// (This can be skipped for unigram LMs, unless we're explicitly
	// asked to eliminate pauses.  It also not necessary for the 
	// new general LM expansion algorithms.
	if (noPause || compactPause || (oldExpansion && order >= 2)) {
	    if (!noPause) {
		lat.removeAllXNodes(vocab.pauseIndex());
	    }
	    if (!noNulls) {
		lat.removeAllXNodes(Vocab_None);
	    }
	  
	    recoverPauses = true;

	    /*
	     * attempt further reduction on pause-less lattices
	     */
	    if (preReductionIter) {
		cerr << "reducing pause-less lattices (overlap ratio = "
		     << overlapRatio << ")\n"; 

		File f(stderr);
		if (overlapRatio == 0.0) { 
		    lat.simplePackBigramLattice(preReductionIter); 
		} else {
		    lat.approxRedBigramLattice(preReductionIter, overlapBase,
		    				overlapRatio);
		}
	    }
	}

	Boolean status;

	if (oldExpansion) {
	    switch (order) {
	    case 1:
	    case 2: 
		// unigram/bigram weight replacement
		status = lat.replaceWeights(lm); 
		break;
	    default:
		// trigram expansion
		if (compactExpansion) {
		  status = lat.expandToCompactTrigram(*(Ngram *)&lm, maxNodes); 
		} else {
		  status = lat.expandToTrigram(lm, maxNodes); 
		}
	    }
	} else {
	    if (noBackoffWeights) {
		// hack to ignore backoff weights in LM expansion
		lat.noBackoffWeights = true;
	    }

	    // general LM expansion
	    status = lat.expandToLM(lm, maxNodes, compactExpansion); 
	}

	if (!status) {
	    cerr << "WARNING: expansion of lattice " << inLat << " failed\n";
#ifndef NO_TIMEOUT
	    alarm(0);
#endif
	    haveError = 1;
	    return;
	}

	/*
	 * after LM application need to make sure that probs will fit in
	 * bytelog range
	 */
	lat.limitIntlogs = true;

	/*
	 * Replace old HTK language scores in output with new LM scores
	 */
	htkScoreMapping = mapHTKlanguage;
    }

    if ((!noPause && recoverPauses) || insertPause) {
	if (compactPause) {
	    lat.recoverCompactPauses(loopPause, insertPause);
	} else {
	    lat.recoverPauses(loopPause, insertPause);
	}
    }

    /*
     * attempt further reduction on output lattices after LM expansion
     */
    if (postReductionIter) {
        cerr << "reducing output lattices (overlap ratio = "
	     << overlapRatio << ")\n"; 

	if (overlapRatio == 0.0) { 
	    lat.simplePackBigramLattice(postReductionIter); 
	} else {
	    lat.approxRedBigramLattice(postReductionIter, overlapBase, 
				       overlapRatio);
	}
    }

    if (splitMultiwordsAfterLM) {
	/*
	 * Split multiwords after LM application
	 * We create an empty LM so that none of the multiwords appear with
	 * non-zero probability
	 */
	Ngram dummy(vocab);

    	if (multiwordDictFile) {
	    lat.splitHTKMultiwordNodes((MultiwordVocab &)vocab, multiwordDict);
	}

	lat.splitMultiwordNodes((MultiwordVocab &)vocab, dummy);
    }

    if (collapseSameWords) {
	lat.collapseSameWordNodes(noiseWords);
    }

    /*
     * perform lattice algebra
     */
    Lattice *finalLat,
	    resultLat(vocab, idFromFilename(outLat), ignoreWords); 

    if (operation && lattice2 != 0) {
        resultLat.debugme(debug);
    
	if (!strcmp(operation, LATTICE_OR)) {
	    resultLat.latticeOr(lat, *lattice2);
	} else if (!strcmp(operation, LATTICE_CONCATE)) {
	    resultLat.latticeCat(lat, *lattice2, interSegmentTime);
	} else {
	    cerr << "unknown operation (" << operation << ")\n";
	    cerr << "allowed operations are " << LATTICE_OR
		 << " and " << LATTICE_CONCATE << endl;
	    exit(2);
	}

	finalLat = &resultLat;
    } else {
	finalLat = &lat;
    }

    if (writeMesh || writeMeshDir || posteriorDecode || wordPosteriorsFile) {
	VocabDistance *wordDistance = 0;

	/*
	 * Use word distance constrained by hidden-vocabulary membership
	 * if specified
	 */
	if (hiddenVocabFile) {
	    wordDistance = new SubVocabDistance(vocab, hiddenVocab);
	    assert(wordDistance!= 0);
	} else if (dictFile && dictAlign) {
	    wordDistance = new DictionaryAbsDistance(vocab, dictionary);
	    assert(wordDistance != 0);
	}

	WordMesh mesh(vocab, lat.getName(), wordDistance);

	if (initMesh) {
	    /*
	     * Initialize alignment with given sausage
	     */
	    File file(initMesh, "r");

	    if (!mesh.read(file)) {
		cerr << "warning: error int initial mesh\n";
		haveError = 1;
	    }
	}

	/*
	 * Preserve acoustic information in word mesh if requested,
	 * or if needed for CTM generation.
	 */
	lat.alignLattice(mesh, noiseWords, posteriorScale,
						acousticMesh || outputCTM);

	if (posteriorDecode) {
	    /*
	     * Recover best hyp from lattice
	     */
	    unsigned maxLength = mesh.length();
	    double subs, inss, dels;

	    if (outputCTM) {
		NBestWordInfo *bestWords = new NBestWordInfo[maxLength + 1];
		assert(bestWords != 0);

		double errors = mesh.minimizeWordError(bestWords, maxLength + 1,
							      subs, inss, dels);
		printCTM(vocab, bestWords, lat.getName());

		delete [] bestWords;
	    } else {
		makeArray(VocabIndex, bestWords, maxLength + 1);

		double errors = mesh.minimizeWordError(bestWords, maxLength + 1,
							      subs, inss, dels);

		cout << lat.getName() << " "
		     << (mesh.vocab.use(), bestWords) << endl;
	    }
	}

	if (wordPosteriorsFile) {
	    File file(wordPosteriorsFile, "r");
	    computeWordPosteriors(mesh, file);
	}

	if (refIndices && (writeMesh || writeMeshDir)) {
	    mesh.alignReference(refIndices);
	}

	if (writeMesh) {
	    File file(writeMesh, "w");
	    mesh.write(file);
	}
	if (writeMeshDir) {
	    makeArray(char, outfile,
		      strlen(writeMeshDir) + 1 +
		      strlen(lat.getName()) + sizeof(GZIP_SUFFIX));
	    sprintf(outfile, "%s/%s%s", writeMeshDir,
					lat.getName(), GZIP_SUFFIX);

	    File file(outfile, "w");
	    mesh.write(file);
	}

	delete wordDistance;
    }

    if (writeNgrams) {
	lat.countNgrams(order, ngramCounts, posteriorScale);
    }

    if (writeNgramIndex) {
	lat.indexNgrams(order, *ngramIndexFile, minCount, ngramsMaxPause,
					ngramsTimeTolerance, posteriorScale);
    }

#ifndef NO_TIMEOUT
    // kill alarm timer -- we're done
    alarm(0);
#endif

    if (pplFile) {
	if (!noPause) {
	    // treat pauses as regular words for LatticeLM computation
	    finalLat->ignoreVocab.remove(finalLat->vocab.pauseIndex());
	}
		
	LatticeLM latlm(*finalLat);
	
        File file(pplFile, "r");
        TextStats stats;

        /*
         * Send perplexity info to stdout 
         */
        latlm.dout(cout);
        latlm.pplFile(file, stats);
        latlm.dout(cerr);

        cout << "file " << pplFile << ": " << stats;
    }

    if (outLattice || outLatticeDir) {
      File file(outLat, "w");
      if (writeHTK) {
        finalLat->writeHTK(file, htkScoreMapping, computePosteriors);
      } else if (writeInternal) {
        finalLat->writePFSG(file);
      } else {
        finalLat->writeCompactPFSG(file);
      }
    }
}

LM *
makeMixLM(const char *filename, Vocab &vocab, SubVocab *classVocab,
		    unsigned order, LM *oldLM, double lambda1, double lambda2)
{
    LM *lm;

    if (useServer && strchr(filename, '@') && !strchr(filename, '/')) {
	/*
	 * Filename looks like a network LM spec -- create LMClient object
	 */
	lm = new LMClient(vocab, filename, order,
					    cacheServedNgrams ? order : 0);
	assert(lm != 0);
    } else {
	File file(filename, "r");

	/*
	 * create factored LM or class-LM if specified, otherwise a regular ngram
	 */
	lm = factored ? 
		      new ProductNgram((ProductVocab &)vocab, order) :
		      (classVocab != 0) ?
			(simpleClasses ?
			      new SimpleClassNgram(vocab, *classVocab, order) :
			      new ClassNgram(vocab, *classVocab, order)) :
			new Ngram(vocab, order);
	assert(lm != 0);

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

    if (oldLM) {
	/*
	 * Compute mixture lambda (make sure 0/0 = 0)
	 */
	Prob lambda = (lambda1 == 0.0) ? 0.0 : lambda1/lambda2;

	LM *newLM = new BayesMix(vocab, *lm, *oldLM,
				    bayesLength, lambda, bayesScale);
	assert(newLM != 0);

	return newLM;
    } else {
	return lm;
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
main (int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");
    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (version) {
	printVersion(RcsId);
	exit(0);
    }

    if (!inLattice && !inLatticeList) {
        cerr << "need to specify at least one input file!\n";
	return 0; 
    }

    if (lmFile && useServer) {
    	cerr << "-lm and -use-server are mutually exclusive\n";
	exit(2);
    }

    if (factored &&
	(classesFile ||
	 splitMultiwords || splitMultiwordsAfterLM || useMultiwordLM))
    {
	cerr << "factored LMs cannot be used with class definitions or multiwords\n";
	exit(2);
    }

    if ((factored || classesFile || mixFile || useMultiwordLM) &&
	oldExpansion && compactExpansion)
    {
        cerr << "cannot use factored LM, class-ngram LM, mixture LM, or multiword LM wrapper for old compact trigram expansion\n";
        exit(2);
    }

    if (hiddenVocabFile && dictFile && dictAlign) {
	cerr << "cannot use both -hidden-vocab and -dictionary-align, choose one\n";
        exit(2);
    }

    /*
     * Set debug level for all LMs
     */
    LM::initialDebugLevel = debug;

    /*
     * Use multiword vocab in case we need it for -multiwords processing
     */
    Vocab *vocab;

    if (factored) {
	vocab = new ProductVocab;
    } else if (splitMultiwords || splitMultiwordsAfterLM || useMultiwordLM) {
	vocab = new MultiwordVocab(multiChar);
    } else {
	vocab = new Vocab;
    }
    assert(vocab != 0);
    vocab->unkIsWord() = (useUnk || keepUnk) ? true : false;
    vocab->toLower() = toLower ? true : false;

    if (factored) {
	((ProductVocab *)vocab)->nullIsWord() = keepNullFactors ? true : false;
    }

    /*
     * Change unknown word string if requested
     */
    if (mapUnknown) {
	vocab->remove(vocab->unkIndex());
	vocab->unkIndex() = vocab->addWord(mapUnknown);
    }

    /*
     * Read predefined vocabulary
     * (required by -limit-vocab and useful with -unk)
     */
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
	 * If pause is treated as a regular word also don't consider it a 
	 * non-event for LM purposes.
	 */
	if (keepPause) {
	    vocab->removeNonEvent(vocab->pauseIndex());
	}

	/*
	 * create temporary sub-vocabulary for non-event words
	 */
	SubVocab nonEvents(*vocab);

	File file(noneventFile, "r");
	nonEvents.read(file);
	vocab->addNonEvents(nonEvents);
    }

    /*
     * Optionally read a subvocabulary that is to be kept separate from
     * regular words during alignment
     */
    SubVocab hiddenVocab(*vocab);
    if (hiddenVocabFile) {
	File file(hiddenVocabFile, "r");

	hiddenVocab.read(file);
    }

    Ngram *ngram;

    /*
     * create base N-gram model (either factored, class- or word-based)
     */
    SubVocab *classVocab = 0;
    if (factored) {
	ngram = new ProductNgram(*(ProductVocab *)vocab, order);
    } else if (classesFile) {
        classVocab = new SubVocab(*vocab);
	assert(classVocab != 0);

	if (simpleClasses) {
	    ngram = new SimpleClassNgram(*vocab, *classVocab, order);
	} else {
	    ngram = new ClassNgram(*vocab, *classVocab, order);

	    if (order > 2 && !oldExpansion) {
		cerr << "warning: general class LM does not allow efficient lattice expansion; consider using -simple-classes\n";
	    }
	}
    } else {
	ngram = new Ngram(*vocab, order);
    }
    assert(ngram != 0);

    if (lmFile) {
      File file1(lmFile, "r");
      if (!ngram->read(file1, limitVocab)) {
	cerr << "format error in lm file\n";
	exit(1);
      }
    }

    if (classesFile) {
	File file(classesFile, "r");
	((ClassNgram *)ngram)->readClasses(file);
    }

    SubVocab noiseVocab(*vocab);
    // -pau- is ignored in WER computation by default
    if (!keepPause) {
	noiseVocab.addWord(vocab->pauseIndex());
    }

    // read additional "noise" words to be ignored in WER computation
    if (noiseVocabFile) {
	File file(noiseVocabFile, "r");
	noiseVocab.read(file);
    }

    SubVocab ignoreVocab(*vocab);

    // read additional words to ignore
    if (ignoreVocabFile) {
	File file(ignoreVocabFile, "r");
	ignoreVocab.read(file);
    } else if (!keepPause) {
	// -pau- is ignored by default
	ignoreVocab.addWord(vocab->pauseIndex());
    }

    /*
     * Prepare dictionary for pronunciation scoring
     */
    Vocab dictVocab;
    VocabMultiMap dictionary(*vocab, dictVocab, intlogs);
    if (dictFile) {
        File file(dictFile, "r");

        if (!dictionary.read(file, limitVocab)) {
            cerr << "format error in dictionary file\n";
            exit(1);
        }
    }

    /*
     * Read dictionary for multiword splitting
     */
    LHash<const char*, Array< Array<char*>* > > multiwordDict;
    if (multiwordDictFile) {
	File file(multiwordDictFile, "r");

	if (!Lattice::readMultiwordDict(file, multiwordDict)) {
            cerr << "format error in multiword dictionary file\n";
	    exit(1);
	}
    }

    /*
     * Build the LM used for lattice expansion
     */
    LM *useLM;
    
    if (useServer) {
    	useLM = new LMClient(*vocab, useServer, order,
					cacheServedNgrams ? order : 0);
	assert(useLM != 0);
    } else {
	useLM = ngram;
    }

    if (mixFile && !loglinearMix) {
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

    /*
     * create multiword wrapper around LM so far, if requested
     */
    if (useMultiwordLM) {
	useLM = new MultiwordLM((MultiwordVocab &)*vocab, *useLM);
	assert(useLM != 0);
    } 

    if (zeroprobWord) {
	useLM = new NonzeroLM(*vocab, *useLM, zeroprobWord);
	assert(useLM != 0);
    }


    Lattice *lattice2 = 0;

    if (inLattice2) {
	lattice2 = new Lattice(*vocab);
	lattice2->debugme(debug);

        File file(inLattice2, "r");
        if (!(readHTK ? lattice2->readHTK(file, &htkparms, useHTKnulls)
		      : lattice2->readPFSGs(file)))
	{
	    cerr << "error reading second lattice operand\n";
	    exit(1);
	}
    } else {
	if (operation) {
	    cerr << "lattice operation needs second lattice\n";
	    exit(2);
	}
    }

    /*
     * create Ngram count trie (even if not used)
     */
    NgramCounts<FloatCount> ngramCounts(*vocab, order);

    /*
     * ngram index file
     */
    File *ngramIdxFile = 0;
    
    if (writeNgramIndex) {
	ngramIdxFile = new File(writeNgramIndex, "w");
	assert(ngramIdxFile != 0);
    }

    RefList reflist(*vocab, refList);
    if (refList || refFile) {
	File file1(refList ? refList : refFile, "r"); 
	reflist.read(file1, true); 	// add ref words to vocabulary

    }
    if (writeRefs) {
	File file1(writeRefs, "w");
	reflist.write(file1);
    }

    if (inLattice) { 
	VocabIndex *refVocabIndex = 0;

	if (refList) {
	    refVocabIndex = reflist.findRef(idFromFilename(inLattice));
	} else if (refFile) {
	    refVocabIndex = reflist.findRefByNumber(0);
	}

	makeArray(char, fileName,
		  outLatticeDir ? strlen(outLatticeDir) + 1024
		                : strlen(LATTICE_NONAME) + 1); 
	if (!outLattice && outLatticeDir) {
	    char *sentid = strrchr(inLattice, '/');
	    if (sentid != NULL) {  
		sentid += 1;
	    } else {
		sentid = inLattice;
	    }

	    sprintf(fileName, "%s/%.1023s", outLatticeDir, sentid);
	} else {
	    // make sure we have some name
	    strcpy(fileName, LATTICE_NONAME);
	}

        processLattice(inLattice, outLattice ? outLattice : fileName, lattice2,
			ngramCounts, ngramIdxFile, *useLM, *vocab, hiddenVocab,
			dictionary, multiwordDict, ignoreVocab, noiseVocab,
			refVocabIndex);
    } 

    if (inLatticeList) {
        if (outLatticeDir) {
	    if (MKDIR(outLatticeDir) < 0) {
		if (errno == EEXIST) {
		    if (!overwrite) {
			cerr << "Dir " << outLatticeDir
			     << " already exists, please give another one\n";
			exit(2);
		    }
		} else {
		    perror(outLatticeDir);
		    exit(1);
		}
	    }
	}
	
	File listOfFiles(inLatticeList, "r"); 
	makeArray(char, fileName,
		  outLatticeDir ? strlen(outLatticeDir) + 1024 : 1); 
	int flag; 
	char buffer[1024]; 
	unsigned latticeCount = 0;
	while ((flag = fscanf(listOfFiles, " %1024s", buffer)) == 1) {

	    cerr << "processing file " << buffer << "\n"; 

	    char *sentid = strrchr(buffer, '/');
	    if (sentid != NULL) {  
	    	sentid += 1;
	    } else {
	    	sentid = buffer;
	    }

	    if (outLatticeDir) {
		sprintf(fileName, "%s/%s", outLatticeDir, sentid);
		cerr << "     to be dumped to " << fileName << "\n"; 
	    } else {
		fileName[0] = '\0';
	    }

	    VocabIndex *refVocabIndex = 0;
	    if (refList) {
		refVocabIndex = reflist.findRef(idFromFilename(buffer));
	    } else if (refFile) {
		refVocabIndex = reflist.findRefByNumber(latticeCount);
	    }
	    processLattice(buffer, fileName, lattice2,
	    				ngramCounts, ngramIdxFile,
					*useLM, *vocab, hiddenVocab,
					dictionary, multiwordDict,
					ignoreVocab, noiseVocab, refVocabIndex); 

	    latticeCount ++;
	}
    }

    if (writeNgrams) {
	/*
	 * prune counts if specified
	 */
	if (minCount > 0.0) {
	    ngramCounts.pruneCounts(minCount);
	}

	File file(writeNgrams, "w");

	if (debug >= DebugPrintFunctionality) {
	    cerr << "writing ngram counts to " << writeNgrams << endl;
	}
	ngramCounts.write(file, 0, true);
    }

    if (writeNgramIndex) {
        delete ngramIdxFile;
    }

    if (writeVocabFile) {
	File file(writeVocabFile, "w");
	vocab->write(file);
    }


#ifdef DEBUG
    // save some time by only deallocating LMs when debugging
    if (ngram != useLM) {
	delete ngram;
    }
    delete useLM;
#endif /* DEBUG */

    delete vocab;
    delete classVocab;
    delete lattice2;

    exit(haveError);
}

