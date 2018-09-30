/*
 * nbest-optimize --
 *	Optimize score combination for N-best rescoring
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2000 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/nbest-optimize.cc,v 1.11 2000/07/13 06:17:50 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <locale.h>
#include <unistd.h>
#include <math.h>

#include "option.h"
#include "File.h"
#include "Vocab.h"
#include "zio.h"

#include "NullLM.h"
#include "RefList.h"
#include "NBestSet.h"
#include "WordMesh.h"
#include "Array.cc"
#include "LHash.cc"

#define DEBUG_TRAIN 1
#define DEBUG_ALIGNMENT	2
#define DEBUG_SCORES 3
#define DEBUG_RANKING 4

typedef double NBestScore;

unsigned numScores;				/* number of score dimensions */
LHash<RefString,NBestScore **> nbestScores;	/* matrices of nbest scores,
						 * one matrix per nbest list */
LHash<RefString,WordMesh *> nbestAlignments;	/* nbest alignments */

Array<double> lambdas;				/* score weights */
Array<double> lambdaDerivs;			/* error derivatives wrt same */
Array<double> prevLambdaDerivs;
Array<double> prevLambdaDeltas;
Array<Boolean> fixLambdas;			/* lambdas to keep constant */
unsigned numRefWords;				/* number of reference words */
unsigned totalError;				/* number of word errors */
double totalLoss;				/* smoothed loss */
Array<double> bestLambdas;			/* lambdas at lowest error */
unsigned bestError;				/* lower error count */

static unsigned debug = 0;
static char *vocabFile = 0;
static int tolower = 0;
static int multiwords = 0;
static char *noiseTag = 0;
static char *noiseVocabFile = 0;
static char *refFile = 0;
static char *nbestFiles = 0;
static unsigned maxNbest = 0;
static char *printHyps = 0;
static int quickprop = 0;

static double rescoreLMW = 8.0;
static double rescoreWTW = 0.0;
static double posteriorScale = 0.0;

static char *initLambdas = 0;
static double alpha = 1.0;
static double epsilon = 0.1;
static double epsilonStepdown = 0.0;
static double minEpsilon = 0.0001;
static double minLoss = 0;
static double maxDelta = 1000;
static unsigned maxIters = 1000;
static double converge = 0.0001;
static unsigned maxBadIters = 10;

static int optRest;

static Option options[] = {
    { OPT_STRING, "refs", &refFile, "reference transcripts" },
    { OPT_STRING, "nbest-files", &nbestFiles, "list of training Nbest files" },
    { OPT_UINT, "max-nbest", &maxNbest, "maximum number of hyps to consider" },

    { OPT_STRING, "vocab", &vocabFile, "set vocabulary" },
    { OPT_TRUE, "tolower", &tolower, "map vocabulary to lowercase" },
    { OPT_TRUE, "multiwords", &multiwords, "split multiwords in N-best hyps" },
    { OPT_STRING, "noise", &noiseTag, "noise tag to skip" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary to skip" },
    { OPT_FLOAT, "rescore-lmw", &rescoreLMW, "rescoring LM weight" },
    { OPT_FLOAT, "rescore-wtw", &rescoreWTW, "rescoring word transition weight" },
    { OPT_FLOAT, "posterior-scale", &posteriorScale, "divisor for log posterior estimates" },
    { OPT_STRING, "init-lambdas", &initLambdas, "initial lambda values" },
    { OPT_FLOAT, "alpha", &alpha, "sigmoid slope parameter" },
    { OPT_FLOAT, "epsilon", &epsilon, "learning rate parameter" },
    { OPT_FLOAT, "epsilon-stepdown", &epsilonStepdown, "epsilon step-down factor" },
    { OPT_FLOAT, "min-epsilon", &minEpsilon, "minimum epsilon after step-down" },
    { OPT_FLOAT, "min-loss", &minLoss, "samples with loss below this are ignored" },
    { OPT_FLOAT, "max-delta", &maxDelta, "threshold to filter large deltas" },
    { OPT_UINT, "maxiters", &maxIters, "maximum number of learning iterations" },
    { OPT_UINT, "max-bad-iters", &maxBadIters, "maximum number of iterations without improvement" },
    { OPT_FLOAT, "converge", &converge, "minimum relative change in objective function" },
    { OPT_STRING, "print-hyps", &printHyps, "output file for final top hyps" },
    { OPT_UINT, "debug", &debug, "debugging level" },
    { OPT_TRUE, "quickprop", &quickprop, "use QuickProp gradient descent" },
    { OPT_REST, "-", &optRest, "indicate end of option list" },
};

const int refID = -1; 	// pseudo-hyp ID used to identify the reference in
			// an N-best alignment

double
sigmoid(double x)
{
    return 1/(1 + exp(- alpha * x));
}

void
dumpScores(ostream &str, NBestSet &nbestSet)
{
    NBestSetIter iter(nbestSet);
    NBestList *nbest;
    RefString id;

    while (nbest = iter.next(id)) {
	str << "id = " << id << endl;

	NBestScore ***scores = nbestScores.find(id);

	if (!scores) {
	    str << "no scores found!\n";
	} else {
	    for (unsigned j = 0; j < nbest->numHyps(); j ++) {
		str << "Hyp " << j << ":" ;
		for (unsigned i = 0; i < numScores; i ++) {
		    str << " " << (*scores)[i][j];
		}
		str << endl;
	    }
	}
    }
}

void
dumpAlignment(ostream &str, WordMesh &alignment)
{
    for (unsigned pos = 0; pos < alignment.length(); pos ++) {
	Array<int> *hypMap;
	VocabIndex word;

	str << "position " << pos << endl;

	WordMeshIter iter(alignment, pos);
	while (hypMap = iter.next(word)) {
	    str << "  word = " << alignment.vocab.getWord(word) << endl;

	    for (unsigned k = 0; k < hypMap->size(); k ++) {
		str << " " << (*hypMap)[k];
	    }
	    str << endl;
	}
    }
}

/*
 * compute hypothesis score (weighted sum of log scores)
 */
LogP
hypScore(unsigned hyp, NBestScore **scores)
{
    LogP score = 0.0;
    double *weights = lambdas.data(); /* bypass index range check for speed */

    for (unsigned i = 0; i < numScores; i ++) {
	score += weights[i] * scores[i][hyp];
    }
    return score;
}

/*
 * compute summed hyp scores (sum of unnormalized posteriors of all hyps
 *	containing a word)
 *	isCorrect is set to true if hyps contains the reference (refID)
 *	The last parameter is used to collect auxiliary sums needed for
 *	derivatives
 */
Prob
wordScore(Array<int> &hyps, NBestScore **scores, Boolean &isCorrect,
								Prob *a = 0)
{
    Prob totalScore = 0.0;
    isCorrect = false;

    if (a != 0) {
	for (unsigned i = 0; i < numScores; i ++) {
	    a[i] = 0.0;
	}
    }

    for (unsigned k = 0; k < hyps.size(); k ++) {
	if (hyps[k] == refID) {
	    isCorrect = true;
	} else {
	    Prob score = LogPtoProb(hypScore(hyps[k], scores));

	    totalScore += score;
	    if (a != 0) {
		for (unsigned i = 0; i < numScores; i ++) {
		    a[i] += score * scores[i][hyps[k]];
		}
	    }
	}
    }

    return totalScore;
}


/*
 * compute loss and derivatives for a single nbest list
 */
void
computeDerivs(RefString id, NBestScore **scores, WordMesh &alignment)
{
    /* 
     * process all positions in alignment
     */
    for (unsigned pos = 0; pos < alignment.length(); pos++) {
	VocabIndex corWord = Vocab_None;
	Prob corScore = 0.0;
	Array<int> *corHyps;

	VocabIndex bicWord = Vocab_None;
	Prob bicScore = 0.0;
	Array<int> *bicHyps;

	if (debug >= DEBUG_RANKING) {
	    cerr << "   position " << pos << endl;
	}

	WordMeshIter iter(alignment, pos);

	Array<int> *hypMap;
	VocabIndex word;
	while (hypMap = iter.next(word)) {
	    /*
	     * compute total score for word and check if it's the correct one
	     */
	    Boolean isCorrect;
	    Prob totalScore = wordScore(*hypMap, scores, isCorrect);

	    if (isCorrect) {
		corWord = word;
		corScore = totalScore;
		corHyps = hypMap;
	    } else {
		if (bicWord == Vocab_None || bicScore < totalScore) {
		    bicWord = word;
		    bicScore = totalScore;
		    bicHyps = hypMap;
		}
	    }
	}

	/*
	 * There must be a correct hyp
	 */
	assert(corWord != Vocab_None);

	if (debug >= DEBUG_RANKING) {
	    cerr << "      cor word = " << alignment.vocab.getWord(corWord)
		 << " score = " << corScore << endl;
	    cerr << "      bic word = " << (bicWord == Vocab_None ? "NONE" :
					    alignment.vocab.getWord(bicWord))
		 << " score = " << bicScore << endl;
	}

	unsigned wordError = (bicScore > corScore);
	double smoothError = 
			sigmoid(ProbToLogP(bicScore) - ProbToLogP(corScore));

	totalError += wordError;
	totalLoss += smoothError;

	/*
	 * If all word hyps are correct or incorrect, or loss is below a set
	 * threshold, then this sample cannot help us and we exclude it from
	 * the derivative computation
	 */
	if (bicScore == 0.0 || corScore == 0.0 || smoothError < minLoss) {
	    continue;
	}

	/*
	 * Compute the auxiliary vectors for derivatives
	 */
	Boolean dummy;
	Prob corA[numScores];
	wordScore(*corHyps, scores, dummy, corA);

	Prob bicA[numScores];
	wordScore(*bicHyps, scores, dummy, bicA);

	/*
	 * Accumulate derivatives
	 */
	double sigmoidDeriv = alpha * smoothError * (1 - smoothError);

	for (unsigned i = 0; i < numScores; i ++) {
	    double delta = (bicA[i] / bicScore - corA[i] / corScore);

	    if (fabs(delta) > maxDelta) {
		cerr << "skipping large delta " << delta
		     << " at id " << id
		     << " position " << pos
		     << " score " << i
		     << endl;
	    } else {
		lambdaDerivs[i] += sigmoidDeriv * delta;
	    }
#if 0
cerr << "i = " << i << " sigmoid = " << sigmoidDeriv
     << " bicA[i] " << bicA[i] << " bicScore = " << bicScore
       << " corA[i] " << corA[i] <<  " corScore =  " << corScore
	 << endl;
#endif
	}
    }
}

/*
 * do a single pass over all nbest lists, computing loss function
 * and accumulating derivatives for lambdas.
 */
void
computeDerivs(NBestSet &nbestSet)
{
    /*
     * Initialize error counts and derivatives
     */
    totalError = 0;
    totalLoss = 0.0;

    for (unsigned i = 0; i < numScores; i ++) {
	lambdaDerivs[i] = 0.0;
    }

    NBestSetIter iter(nbestSet);
    NBestList *nbest;
    RefString id;

    while (nbest = iter.next(id)) {
	NBestScore ***scores = nbestScores.find(id);
	assert(scores != 0);
	WordMesh **alignment = nbestAlignments.find(id);
	assert(alignment != 0);

	computeDerivs(id, *scores, **alignment);
    }
}

/*
 * print lambdas
 */
void
printLambdas(ostream &str, Array<double> &lambdas)
{
    str << "   lambdas =";
    for (unsigned i = 0; i < numScores; i ++) {
	str << " " << lambdas[i];
    }
    str << endl;

    str << "   normed =";
    for (unsigned i = 0; i < numScores; i ++) {
	str << " " << lambdas[i]/lambdas[0];
    }
    str << endl;
    str << "   posterior scale = " << 1/lambdas[0]
	<< endl;
}

/*
 * One step of gradient descent on loss function
 */
void
updateLambdas()
{
    static Boolean havePrev = false;

    for (unsigned i = 0; i < numScores; i ++) {
	if (!fixLambdas[i]) {
	    double delta;

	    if (!havePrev || !quickprop ||
		lambdaDerivs[i]/prevLambdaDerivs[i] > 0)
	    {
		delta = - epsilon * lambdaDerivs[i] / numRefWords;
	    } else {
		/*
		 * Use QuickProp update rule
		 */
		delta = prevLambdaDeltas[i] * lambdaDerivs[i] /
			    (prevLambdaDerivs[i] - lambdaDerivs[i]);
	    }
	    lambdas[i] += delta;

	    prevLambdaDeltas[i] = delta;
	    prevLambdaDerivs[i] = lambdaDerivs[i];
	}
    }
	
    havePrev = true;
}

/*
 * Iterate gradient descent on loss function
 */
void
train(NBestSet &nbestSet)
{
    unsigned iter = 0;
    unsigned badIters = 0;
    double oldLoss;

    if (debug >= DEBUG_TRAIN) {
	printLambdas(cerr, lambdas);
    }

    while (iter++ < maxIters) {

	computeDerivs(nbestSet);

	if (iter > 0 && fabs(totalLoss - oldLoss)/oldLoss < converge) {
	    cerr << "stopping due to convergence\n";
	    break;
	}

	if (debug >= DEBUG_TRAIN) {
	    cerr << "iteration " << iter << ":"
		 << " errors = " << totalError
		 << " (" << ((double)totalError/numRefWords) << "/word)"
		 << " loss = " << totalLoss
		 << " (" << (totalLoss/numRefWords) << "/word)"
		 << endl;
	}

	if (iter == 1 || !isnan(totalLoss) && totalError < bestError) {
	    cerr << "NEW BEST ERROR\n";
	    bestError = totalError;
	    bestLambdas = lambdas;
	    badIters = 0;
	} else {
	    badIters ++;
	}

	if (badIters > maxBadIters || isnan(totalLoss)) {
	    if (epsilonStepdown > 0.0) {
		epsilon *= epsilonStepdown;
		if (epsilon < minEpsilon) {
		    cerr << "minimum epsilon reached\n";
		    break;
		}
		cerr << "setting epsilon to " << epsilon
		     << " due to lack of error decrease\n";

		/*
		 * restart descent at last best point, and 
		 * disable QuickProp for the next iteration
		 */
		prevLambdaDerivs = lambdaDerivs;	
		lambdas = bestLambdas;
		badIters = 0;
	    } else {
		cerr << "stopping due to lack of error decrease\n";
		break;
	    }
	} else {
	    updateLambdas();
	}

	if (debug >= DEBUG_TRAIN) {
	    printLambdas(cerr, lambdas);
	}

	oldLoss = totalLoss;
    }
}

/*
 * output top hypothesis
 */
void
printTopHyp(File &file, RefString id, NBestScore **scores, WordMesh &alignment)
{
    fprintf(file, "%s", id);

    /* 
     * process all positions in alignment
     */
    for (unsigned pos = 0; pos < alignment.length(); pos++) {
	VocabIndex bestWord = Vocab_None;
	Prob bestScore = 0.0;

	WordMeshIter iter(alignment, pos);

	Array<int> *hypMap;
	VocabIndex word;
	while (hypMap = iter.next(word)) {
	    /*
	     * compute total score for word and check if it's the correct one
	     */
	    Boolean dummy;
	    Prob totalScore = wordScore(*hypMap, scores, dummy);

	    if (bestWord == Vocab_None || totalScore > bestScore) {
		bestWord = word;
		bestScore = totalScore;
	    }
	}

	assert(bestWord != Vocab_None);

	if (bestWord != alignment.deleteIndex) {
	    fprintf(file, " %s", alignment.vocab.getWord(bestWord));
	}
    }
    fprintf(file, "\n");
}

void
printTopHyps(File &file, NBestSet &nbestSet)
{
    NBestSetIter iter(nbestSet);
    NBestList *nbest;
    RefString id;

    while (nbest = iter.next(id)) {
	NBestScore ***scores = nbestScores.find(id);
	assert(scores != 0);
	WordMesh **alignment = nbestAlignments.find(id);
	assert(alignment != 0);

	printTopHyp(file, id, *scores, **alignment);
    }
}

/*
 * Read a single score file into a column of the score matrix
 */
void
readScoreFile(const char *scoreDir, RefString id, NBestScore *scores,
							unsigned numHyps) 
{
    char fileName[strlen(scoreDir) + 1 + strlen(id) + strlen(GZIP_SUFFIX) + 1];
					
    sprintf(fileName, "%s/%s", scoreDir, id);

    /* 
     * If plain file doesn't exist try gzipped version
     */
    if (access(fileName, F_OK) < 0) {
	strcat(fileName, GZIP_SUFFIX);
    }

    File file(fileName, "r", 0);

    char *line;
    unsigned hypNo = 0;
    Boolean decipherScores = false;

    while (!file.error() && (line = file.getline())) {
	if (strncmp(line, nbest1Magic, sizeof(nbest1Magic)-1) == 0 ||
	    strncmp(line, nbest2Magic, sizeof(nbest2Magic)-1) == 0)
	{
	    decipherScores = true;
	    continue;
	}

	if (hypNo >= numHyps) {
	    break;
	}

	/*
	 * parse the first word as a score
	 */
	double score;

	if (decipherScores) {
	    if (sscanf(line, "(%lf)", &score) != 1) {
		cerr << "bad Decipher score: " << line << endl;
		break;
	    } else  {
		scores[hypNo ++] = BytelogToLogP(score);
	    }
	} else {
	    if (sscanf(line, "%lf", &score) != 1) {
		cerr << "bad score: " << line << endl;
		break;
	    } else  {
		scores[hypNo ++] = score;(score);
	    }
	}
    }

    /* 
     * Set missing scores to zero
     */
    while (hypNo < numHyps) {
	scores[hypNo ++] = 0;
    }
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    argc = Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (!nbestFiles || !refFile) {
	cerr << "cannot proceed without nbest files and references\n";
	exit(2);
    }

    Vocab vocab;
    NullLM nullLM(vocab);
    RefList refs(vocab);

    NBestSet trainSet(vocab, refs, maxNbest, false, multiwords);
    trainSet.debugme(debug);

    if (vocabFile) {
	File file(vocabFile, "r");
	vocab.read(file);
    }

    vocab.toLower = tolower ? true : false;

    /*
     * Skip noise tags in scoring
     */
    if (noiseVocabFile) {
	File file(noiseVocabFile, "r");
	nullLM.noiseVocab.read(file);
    }
    if (noiseTag) {				/* backward compatibility */
	nullLM.noiseVocab.addWord(noiseTag);
    }

    /*
     * Posterior scaling:  if not specified (= 0.0) use LMW for
     * backward compatibility.
     */
    if (posteriorScale == 0.0) {
	posteriorScale = (rescoreLMW == 0.0) ? 1.0 : rescoreLMW;
    }

    if (refFile) {
	cerr << "reading references...\n";
	File file(refFile, "r");

	refs.read(file, true);	 // add reference words to vocabulary
    }

    {
	cerr << "reading nbest lists...\n";
	File file(nbestFiles, "r");
	trainSet.read(file);
    }

    /*
     * there are three scores in the N-best list, plus as many as 
     * user supplies in separate directories on the command line
     */
    numScores = 3 + argc - 1;

    lambdas[0] = 1/posteriorScale;
    lambdas[1] = rescoreLMW/posteriorScale;
    lambdas[2] = rescoreWTW/posteriorScale;
    for (unsigned i = 3; i < numScores; i ++) {
	lambdas[i] = 0.0;
    }

    for (unsigned i = 0; i < numScores; i ++) {
	fixLambdas[i] = false;
    }

    /*
     * Initialize lambdas from command line values if specified
     */
    if (initLambdas) {
	unsigned offset = 0;

	for (unsigned i = 0; i < numScores; i ++) {
	    unsigned consumed = 0;
 
	    if (sscanf(&initLambdas[offset], " =%lf%n",
						&lambdas[i], &consumed) > 0)
	    {
		fixLambdas[i] = true;
	    } else if (sscanf(&initLambdas[offset], "%lf%n",
						&lambdas[i], &consumed) == 0)
	    {
		break;
	    }

	    offset += consumed;
	}
    }

    /*
     * Set up the score matrices
     */
    cerr << "reading scores...\n";

    NBestSetIter iter(trainSet);

    RefString id;
    NBestList *nbest;
    while (nbest = iter.next(id)) {
	/*
	 * Allocate score matrix for this nbest list
	 */
	NBestScore **scores = new (NBestScore *)[numScores];
	assert(scores != 0);

	for (unsigned i = 0; i < numScores; i ++) {
	    scores[i] = new NBestScore[nbest->numHyps()];
	    assert(scores[i] != 0);
	}

	/*
	 * Transfer the standard scores from N-best list to score matrix
	 */
	for (unsigned j = 0; j < nbest->numHyps(); j ++) {
	    scores[0][j] = nbest->getHyp(j).acousticScore;
	    scores[1][j] = nbest->getHyp(j).languageScore;
	    scores[2][j] = nbest->getHyp(j).numWords;
	}

	/*
	 * Read additional scores
	 */
	for (unsigned i = 1; i < argc; i ++) {
	    readScoreFile(argv[i], id, scores[i + 2], nbest->numHyps());
	}

	/*
	 * Scale scores to help prevent underflow
	 */
	for (unsigned i = 0; i < numScores; i ++) {
	    for (unsigned j = nbest->numHyps(); j > 0; j --) {
		scores[i][j-1] -= scores[i][0];
	    }
	}
	
	/* 
	 * save score matrix under nbest id
	 */
	*nbestScores.insert(id) = scores;
    }

    if (debug >= DEBUG_SCORES) {
	dumpScores(cerr, trainSet);
    }

    cerr << "aligning nbest lists...\n";

    iter.init();

    numRefWords = 0;

    /*
     * Create multiple alignments
     */
    while (nbest = iter.next(id)) {
	VocabIndex *ref = refs.findRef(id);

	if (!ref) {
	    continue;
	}

	/*
	 * compute total length of references for later normalizations
	 */
	numRefWords += Vocab::length(ref);

	/*
	 * Remove pauses and noise from nbest hyps since these would
	 * confuse the inter-hyp alignments.
	 */
	nbest->removeNoise(nullLM);

	/*
	 * compute posteriors for improved word alignment
	 */
	nbest->computePosteriors(rescoreLMW, rescoreWTW, posteriorScale);

	/*
	 * create word-mesh for multiple alignment
	 */
	WordMesh *alignment = new WordMesh(vocab);
	assert(alignment != 0);

	*nbestAlignments.insert(id) = alignment;

	/*
	 * start alignment with the reference string
	 *	Note we give reference posterior 1 only to constrain the
	 *	alignment. The loss computation in training ignores the
	 *	posteriors assigned to hyps at this point.
	 */
	int hypID = refID;

	alignment->alignWords(ref, 1.0, 0, &hypID);

	/*
	 * Now align all N-best hyps
	 */
	for (unsigned j = 0; j < nbest->numHyps(); j ++) {
	    NBestHyp &hyp = nbest->getHyp(j);

	    hypID = j;
	    alignment->alignWords(hyp.words, hyp.posterior, 0, &hypID);
	}

	if (debug >= DEBUG_ALIGNMENT) {
	    dumpAlignment(cerr, *alignment);
	}
    }

    cerr << numRefWords << " reference words\n";

    /*
     * preemptive trouble avoidance: prevent division by zero
     */
    if (numRefWords == 0) {
	numRefWords = 1;
    }

    train(trainSet);

    cout << "best error = " << bestError
	 << " (" << ((double)bestError/numRefWords) << "/word)"
	 << endl;
    printLambdas(cout, bestLambdas);

    if (printHyps) {
	File file(printHyps, "w");

	lambdas = bestLambdas;
	printTopHyps(file, trainSet);
    }

    exit(0);
}

