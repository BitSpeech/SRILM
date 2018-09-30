/*
 * nbest-optimize --
 *	Optimize score combination for N-best rescoring
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2000-2001 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/nbest-optimize.cc,v 1.28 2001/10/31 07:00:31 stolcke Exp $";
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
#include "WordAlign.h"
#include "WordMesh.h"
#include "Array.cc"
#include "LHash.cc"

#define DEBUG_TRAIN 1
#define DEBUG_ALIGNMENT	2
#define DEBUG_SCORES 3
#define DEBUG_RANKING 4


typedef float NBestScore;			/* same as LogP */

unsigned numScores;				/* number of score dimensions */
unsigned numFixedWeights;			/* number of fixed weights */
LHash<RefString,NBestScore **> nbestScores;	/* matrices of nbest scores,
						 * one matrix per nbest list */
LHash<RefString,WordMesh *> nbestAlignments;	/* nbest alignments */
LHash<RefString,unsigned *> nbestErrors;	/* nbest error counts */

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

Array<double> lambdaSteps;			/* simplex step sizes  */
Array<double> simplex;				/* current simplex points  */

static int oneBest = 0;				/* optimize 1-best error */
static int noReorder = 0;
static unsigned debug = 0;
static char *vocabFile = 0;
static int toLower = 0;
static int multiwords = 0;
static char *noiseTag = 0;
static char *noiseVocabFile = 0;
static char *refFile = 0;
static char *errorsDir = 0;
static char *nbestFiles = 0;
static unsigned maxNbest = 0;
static char *printHyps = 0;
static int quickprop = 0;

static double rescoreLMW = 8.0;
static double rescoreWTW = 0.0;
static double posteriorScale = 0.0;
static double posteriorScaleStep = 1.0;

static char *initLambdas = 0;
static char *initSimplex = 0;
static double alpha = 1.0;
static double epsilon = 0.1;
static double epsilonStepdown = 0.0;
static double minEpsilon = 0.0001;
static double minLoss = 0;
static double maxDelta = 1000;
static unsigned maxIters = 100000;
static double converge = 0.0001;
static unsigned maxBadIters = 10;
static unsigned maxAmoebaRestarts = 100000;

static int optRest;

static Option options[] = {
    { OPT_STRING, "refs", &refFile, "reference transcripts" },
    { OPT_STRING, "nbest-files", &nbestFiles, "list of training N-best files" },
    { OPT_UINT, "max-nbest", &maxNbest, "maximum number of hyps to consider" },
    { OPT_TRUE, "1best", &oneBest, "optimize 1-best error" },
    { OPT_TRUE, "no-reorder", &noReorder, "don't reorder N-best hyps before aligning and align refs first" },
    { OPT_STRING, "errors", &errorsDir, "directory containing error counts" },
    { OPT_STRING, "vocab", &vocabFile, "set vocabulary" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_TRUE, "multiwords", &multiwords, "split multiwords in N-best hyps" },
    { OPT_STRING, "noise", &noiseTag, "noise tag to skip" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary to skip" },
    { OPT_FLOAT, "rescore-lmw", &rescoreLMW, "rescoring LM weight" },
    { OPT_FLOAT, "rescore-wtw", &rescoreWTW, "rescoring word transition weight" },
    { OPT_FLOAT, "posterior-scale", &posteriorScale, "divisor for log posterior estimates" },
    { OPT_STRING, "init-lambdas", &initLambdas, "initial lambda values" },
    { OPT_STRING, "init-amoeba-simplex", &initSimplex, "initial amoeba simplex points" },
    { OPT_FLOAT, "alpha", &alpha, "sigmoid slope parameter" },
    { OPT_FLOAT, "epsilon", &epsilon, "learning rate parameter" },
    { OPT_FLOAT, "epsilon-stepdown", &epsilonStepdown, "epsilon step-down factor" },
    { OPT_FLOAT, "min-epsilon", &minEpsilon, "minimum epsilon after step-down" },
    { OPT_FLOAT, "min-loss", &minLoss, "samples with loss below this are ignored" },
    { OPT_FLOAT, "max-delta", &maxDelta, "threshold to filter large deltas" },
    { OPT_UINT, "maxiters", &maxIters, "maximum number of learning iterations" },
    { OPT_UINT, "max-bad-iters", &maxBadIters, "maximum number of iterations without improvement" },
    { OPT_UINT, "max-amoeba-restarts", &maxAmoebaRestarts, "maximum number of Amoeba restarts" },
    { OPT_FLOAT, "converge", &converge, "minimum relative change in objective function" },
    { OPT_STRING, "print-hyps", &printHyps, "output file for final top hyps" },
    { OPT_UINT, "debug", &debug, "debugging level" },
    { OPT_TRUE, "quickprop", &quickprop, "use QuickProp gradient descent" },
    { OPT_REST, "-", &optRest, "indicate end of option list" },
};

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
	Array<HypID> *hypMap;
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
    static NBestScore **lastScores = 0;
    static Array<LogP> *cachedScores = 0;

    if (scores != lastScores) {
	delete cachedScores;
	cachedScores = new Array<LogP>;
	assert(cachedScores != 0);
	lastScores = scores;
    }

    if (hyp < cachedScores->size()) {
	if ((*cachedScores)[hyp] != 0.0) {
	    return (*cachedScores)[hyp];
	}
    } else {
	for (unsigned j = cachedScores->size(); j < hyp; j ++) {
	    (*cachedScores)[j] = 0.0;
	}
    }

    LogP score = 0.0;

    double *weights = lambdas.data(); /* bypass index range check for speed */

    for (unsigned i = 0; i < numScores; i ++) {
	score += weights[i] * scores[i][hyp];
    }
    return ((*cachedScores)[hyp] = score);
}

/*
 * compute summed hyp scores (sum of unnormalized posteriors of all hyps
 *	containing a word)
 *	isCorrect is set to true if hyps contains the reference (refID)
 *	The last parameter is used to collect auxiliary sums needed for
 *	derivatives
 */
Prob
wordScore(Array<HypID> &hyps, NBestScore **scores, Boolean &isCorrect,
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
	    /*
	     * This hyp represents the correct word string, but doesn't 		     * contribute to the posterior probability for the word.
	     */
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
	Array<HypID> *corHyps;

	VocabIndex bicWord = Vocab_None;
	Prob bicScore = 0.0;
	Array<HypID> *bicHyps;

	if (debug >= DEBUG_RANKING) {
	    cerr << "   position " << pos << endl;
	}

	WordMeshIter iter(alignment, pos);

	Array<HypID> *hypMap;
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
 * compute 1-best word error for a single nbest list
 * Note: uses global lambdas variable (yuck!)
 */
double
compute1bestErrors(RefString id, NBestScore **scores, NBestList &nbest)
{
    unsigned numHyps = nbest.numHyps();
    unsigned bestHyp;
    LogP bestScore;

    if (numHyps == 0) {
	return 0.0;
    }

    /*
     * Find hyp with highest score
     */
    for (unsigned i = 0; i < numHyps; i ++) {
	LogP score = hypScore(i, scores);

	if (i == 0 || score > bestScore) {
	    bestScore = score;
	    bestHyp = i;
	}
    }

    return nbest.getHyp(bestHyp).numErrors;
}

/*
 * compute sausage word error for a single nbest list
 * Note: uses global lambdas variable (yuck!)
 */
double
computeSausageErrors(RefString id, NBestScore **scores, WordMesh &alignment)
{
    int result = 0;

    /* 
     * process all positions in alignment
     */
    for (unsigned pos = 0; pos < alignment.length(); pos++) {
	VocabIndex corWord = Vocab_None;
	Prob corScore = 0.0;
	Array<HypID> *corHyps;

	VocabIndex bicWord = Vocab_None;
	Prob bicScore = 0.0;
	Array<HypID> *bicHyps;

	if (debug >= DEBUG_RANKING) {
	    cerr << "   position " << pos << endl;
	}

	WordMeshIter iter(alignment, pos);
	
	Array<HypID> *hypMap;
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

	    if (corWord != Vocab_None &&
		bicWord != Vocab_None &&
		bicScore > corScore)
	    {
		result ++;
		break;
	    }
	}
    }

    return result;
}

/*
 * Compute word error for vector of weights
 */
double
computeErrors(NBestSet &nbestSet, double *weights)
{
    int result = 0;
    double result_tmp = 0;

    Array<double> savedLambdas;

    unsigned i;
    for (i = 0; i < numScores; i ++) {
        savedLambdas[i] = lambdas[i];
        lambdas[i] = weights[i];
    }

    NBestSetIter iter(nbestSet);
    NBestList *nbest;
    RefString id;

    while (nbest = iter.next(id)) {
	NBestScore ***scores = nbestScores.find(id);
	assert(scores != 0);

	if (oneBest) {
	    result += (int) compute1bestErrors(id, *scores, *nbest);
	} else {
	    WordMesh **alignment = nbestAlignments.find(id);
	    assert(alignment != 0);

	    result += (int) computeSausageErrors(id, *scores, **alignment);
	}
    }

    for (i = 0; i < numScores; i ++) {
        lambdas[i] = savedLambdas[i];
    }
    return result;
}

/*
 * print lambdas
 */
void
printLambdas(ostream &str, Array<double> &lambdas)
{
    double normalizer = 0.0;

    str << "   weights =";
    for (unsigned i = 0; i < numScores; i ++) {
	if (normalizer == 0.0 && lambdas[i] != 0.0) {
	    normalizer = lambdas[i];
	}

	str << " " << lambdas[i];
    }
    str << endl;

    str << "   normed =";
    for (unsigned i = 0; i < numScores; i ++) {
	str << " " << lambdas[i]/normalizer;
    }
    str << endl;

    str << "   scale = " << 1/normalizer
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
	    cerr << "NEW BEST ERROR: " << totalError 
		 << " (" << ((double)totalError/numRefWords) << "/word)\n";
	    printLambdas(cerr, lambdas);

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
 * Evaluate a single point in the (unconstrained) parameter space
 */
double
amoebaComputeErrors(NBestSet &nbestSet, double *p)
{
    int i, j;
    Array <double> weights;

    if (p[0] < 0.5) {
	/*
	 * This prevents posteriors to go through the roof, leading 
	 * to numerical problems.  Since the scaling of posteriors is 
	 * a redundant dimension this doesn't constrain the result.
	 */
	return numRefWords;
    }

    for (i = 0, j = 1; i < numScores; i++) {
	if (fixLambdas[i]) {
	    weights[i] = lambdas[i];
	} else {
	    weights[i] = p[j] / p[0];
	    j++;
	}
    }

    double error = computeErrors(nbestSet, weights.data());

    if (error < bestError) {
	cerr << "NEW BEST ERROR: " << error
	     << " (" << ((double)error/numRefWords) << "/word)\n";
	printLambdas(cerr, weights);

	bestError = (int) error;
	bestLambdas = weights;
    }

    return error;
}

/*
 * Try moving a single simplex corner
 */
double
amoebaEval(NBestSet &nbest, double **p, double *y, double *psum, unsigned ndim,
	   double (*funk) (NBestSet &, double[]), unsigned ihi, double fac)
{
    double ptry[ndim];
    double fac1 = (1.0 - fac) / ndim;
    double fac2 = fac1 - fac;

    for (unsigned j = 0; j < ndim; j++) {
	ptry[j] = psum[j] * fac1 - p[ihi][j] * fac2;
    }
    double ytry = (*funk) (nbest, ptry);
    if (ytry < y[ihi]) {
	y[ihi] = ytry;
	for (unsigned j = 0; j < ndim; j++) {
	    psum[j] += ptry[j] - p[ihi][j];
	    p[ihi][j] = ptry[j];
	}
    }
    return ytry;
}

inline void
computeSum(unsigned ndim, unsigned mpts, double *psum, double **p)
{
    for (unsigned j = 0; j < ndim; j++) {
	double sum = 0.0;
	for (unsigned i = 0; i < mpts; i++) {
	    sum += p[i][j];
	}
	psum[j] = sum;
    }
}

inline void
swap(double &a, double &b)
{
    double h = a;
    a = b;
    b = h;
}

/*
 * Run Amoeba optimization
 */
void
amoeba(NBestSet &nbest, double **p, double *y, unsigned ndim, double ftol,
       double (*funk) (NBestSet &, double[]), unsigned &nfunk)
{
    unsigned ihi, inhi, mpts = ndim + 1;
    double psum[ndim];

    if (debug >= DEBUG_TRAIN) {
	cerr << "Starting amoeba with " << ndim << " dimensions" << endl;
    }

    computeSum(ndim, mpts, psum, p);
    
    double rtol = 10000.0;

    unsigned ilo = 0;
    unsigned unchanged = 0;

    while (1) {
	double ysave, ytry;
	double ylo_pre = y[ilo];

	ilo = 0;

	ihi = y[0] > y[1] ? (inhi = 1, 0) : (inhi = 0, 1);
	for (unsigned i = 0; i < mpts; i++) {
	    if (y[i] <= y[ilo]) {
		ilo = i;
	    }
	    if (y[i] > y[ihi]) {
		inhi = ihi;
		ihi = i;
	    } else if (y[i] > y[inhi] && i != ihi)
		inhi = i;
	}

	if (debug >= DEBUG_TRAIN) {
	    cerr << "Current low " << y[ilo] << ": ";
	    cerr << "Current high " << y[ihi] << ":";
	    /*
	     * for (unsigned j=0; j<ndim; j++)
	     *	   cerr << " " << p[ihi][j] ; cerr << endl; 
	     */
	    cerr << "Current next high " << y[inhi] << endl;
	}

	rtol = 2.0 * fabs(y[ihi] - y[ilo]) / (fabs(y[ihi]) + fabs(y[ilo]));
	if (ylo_pre == y[ilo] && rtol < converge) {
	    unchanged++;
	} else {
	    unchanged = 0;
	    if (ylo_pre > y[ilo]) {
		int k;

		if (debug >= DEBUG_TRAIN) {
		    cerr << "scale " << p[ilo][0] << endl;
		    for (unsigned j = 1, k = 0; k < numScores && j < ndim; k++)
		    {
			if (!fixLambdas[k])
			    cerr << "lambda_" << j - 1
				 << " " << p[ilo][j++] << endl;
		    }
		}
	    }
	}

	if (unchanged > maxBadIters) {
	    swap(y[0], y[ilo]);
	    for (unsigned i = 0; i < ndim; i++) {
		swap(p[0][i], p[ilo][i]);
	    }
	    break;
	}

	if (debug >= DEBUG_TRAIN) {
	    cerr << " fractional range " << rtol << endl;
	    cerr << " limit range " << ftol << endl;
	}

	if (rtol <= ftol) {
	    swap(y[0], y[ilo]);
	    for (unsigned i = 0; i < ndim; i++) {
		swap(p[0][i], p[ilo][i]);
	    }
	    break;
	}

	nfunk += 1;

	// Try a reflection
	ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, -1.0);
	if (debug >= DEBUG_TRAIN) {
	    cerr << " Reflected amoeba returned " << ytry << endl;
	}

	// If successful try more
	if (ytry <= y[ilo]) {
	    nfunk += 1;
	    ysave = ytry;
	    ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, 1.5);
	    if (debug >= DEBUG_TRAIN) {
		cerr << " Expanded amoeba by 1.5 returned " << ytry <<
		    endl;
	    }

	    // If successful try more
	    if (ytry <= ysave) {
		ysave = ytry;
		nfunk += 1;
		ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, 2.0);
		if (debug >= DEBUG_TRAIN) {
		    cerr << " Expanded amoeba by 2.0 returned " << ytry <<
			endl;
		}
	    }

	    // If successful try more
	    if (ytry <= ysave) {
		nfunk += 1;
		ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, 3.0);
		if (debug >= DEBUG_TRAIN) {
		    cerr << " Expanded amoeba by 3.0 returned " << ytry << endl;
		}
	    }
	} else if (ytry >= y[inhi]) {
	    // If failed shrink
	    ysave = y[ihi];

	    // shrink half
	    nfunk += 1;
	    ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, 0.7);
	    if (debug >= DEBUG_TRAIN) {
		cerr << " Shrunken amoeba by 0.7 returned " << ytry << endl;
	    }

	    // try again opposite direction
	    if (ytry >= ysave) {
		nfunk += 1;
		ytry =
		    amoebaEval(nbest, p, y, psum, ndim, funk, ihi, -0.7);
		if (debug >= DEBUG_TRAIN) {
		    cerr << " Shrunken reflected amoeba by -0.7 returned "
			 << ytry << endl;
		}
	    }

	    // try again 
	    if (ytry >= ysave) {
		nfunk += 1;
		ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, 0.5);
		if (debug >= DEBUG_TRAIN) {
		    cerr << " Shrunken amoeba by 0.5 returned " << ytry << endl;
		}
	    }

	    // try again opposite direction
	    if (ytry >= ysave) {
		nfunk += 1;
		ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, -0.5);
		if (debug >= DEBUG_TRAIN) {
		    cerr << " Shrunken reflected amoeba by -0.5 returned "
			 << ytry << endl;
		}
	    }
	    if (ytry >= ysave) {
		nfunk += 1;
		ytry = amoebaEval(nbest, p, y, psum, ndim, funk, ihi, 0.3);
		if (debug >= DEBUG_TRAIN) {
		    cerr << " Shrunken amoeba by 0.3 returned " << ytry << endl;
		}
	    }

	    // if failed to get rid of high contract everything by 0.7
	    if (ytry >= ysave) {
		for (unsigned i = 0; i < mpts; i++) {
		    if (i != ilo) {
			for (unsigned j = 0; j < ndim; j++)
			    p[i][j] = psum[j] =
				0.7 * p[i][j] + 0.3 * p[ilo][j];
			y[i] = (*funk) (nbest, psum);
		    }
		}
		nfunk += ndim;
		computeSum(ndim, mpts, psum, p);
	    }
	} else {
	    --nfunk;
	}
    }
}

/*
 * Amoeba optimization with restarts
 */
void
trainAmoeba(NBestSet &nbestSet)
{
    // Before training reset the lambdas to their unscaled version
    for(unsigned i = 0; i < numScores; i++) {
	if (!fixLambdas[i]) {
	    lambdas[i] *= posteriorScale;
	}
    }

    // Initialize ameoba points
    // There is one search dimension per score dimension, excluding fixed
    // weights, but adding one for the posterior score (which is stored in
    // the vector even if it is kept fixed).
    int numFreeWeights = numScores - numFixedWeights + 1;

    double *points[numFreeWeights + 1];
    for (unsigned i = 0; i <= numFreeWeights; i++) {
	points[i] = new double[numFreeWeights];
	assert(points[i] != 0);
    }

    double errs[numFreeWeights + 1];
    double prevPoints[numFreeWeights];

    prevPoints[0] = points[0][0] = posteriorScale;
    simplex[0] = posteriorScaleStep;
    for (unsigned i = 0, j = 1; i < numScores; i++) {
	if (!fixLambdas[i]) {
	    prevPoints[j] = points[0][j] = lambdas[i];
	    simplex[j] = lambdaSteps[i];
	    j++;
	}
    }

    double *dir[numFreeWeights + 1];
    for (unsigned i = 0; i <= numFreeWeights; i++) {
	dir[i] = new double[numFreeWeights];
	assert(dir[i] != 0);
	for (unsigned j = 0; j < numFreeWeights; j++) {
	    dir[i][j] = 0.0;
	}
    }

    unsigned nevals = 0;
    unsigned loop = 1;

    unsigned same = 0;
    unsigned shift = 0;
    unsigned reps = 0;

    /* force an improvement */
    bestError = (unsigned)-1;

    while (loop) {
	reps++;

	for (unsigned i = 1; i <= numFreeWeights; i++) {
	    unsigned k = 0;
	    dir[i][k] += (((k + loop + shift - 1) % numFreeWeights) + 1 == i) ?
			    loop * simplex[k] : 0.0;
	    k++;
	    for (unsigned j = 0; j < numScores; j++) {
		if (!fixLambdas[j]) {
		    dir[i][k] +=
			(((k + loop + shift - 1) % numFreeWeights) + 1 == i) ?
			    loop * simplex[k] : 0.0;
		    k++;
		}
	    }
	}

	if (debug >= DEBUG_TRAIN) {
	    cerr << "Simplex points:" << endl;
	}

	for (unsigned i = 0; i <= numFreeWeights; i++) {
	    for (unsigned j = 0; j < numFreeWeights; j++) {
		points[i][j] = points[0][j] + dir[i][j];
	    }
	    errs[i] = amoebaComputeErrors(nbestSet, points[i]);

	    if (debug >= DEBUG_TRAIN) {
		cerr << "Point " << i << " : ";

		for (unsigned j = 0; j < numFreeWeights; j++) {
		    cerr << points[i][j] << " ";
		}
		cerr << "errors = " << errs[i] << endl;
	    }
	}

	unsigned prevErrors = (int) errs[0];

	/*
	 * The objective function fractional tolerance is
	 * decreasing with each retry.
	 */
	amoeba(nbestSet, points, errs, numFreeWeights, converge / reps,
					       amoebaComputeErrors, nevals);

	if ((int) errs[0] < prevErrors) {
	    loop++;
	    same = 0;
	} else if (same < numFreeWeights) {
	    loop++;
	    same++;
	} else {
	    loop = 0;
	}

	if (loop > numFreeWeights / 3) {
	    loop = 1;
	    for (unsigned i = 0; i <= numFreeWeights; i++) {
		for (unsigned j = 0; j < numFreeWeights; j++) {
		    dir[i][j] = 0.0;
		}
	    }
	    shift++;
	}

	// reset step sizes  
	posteriorScale = points[0][0];
	if (loop == 1) {
	    simplex[0] = posteriorScaleStep;
	} else if (fabs(prevPoints[0] - points[0][0])
					> 1.3 * fabs(simplex[0]))
	{
	    simplex[0] = points[0][0] - prevPoints[0]; 
	} else {
	    simplex[0] = simplex[0] * 1.3;
	}
	prevPoints[0] = points[0][0];

	unsigned j = 1;
	for (unsigned i = 0; i < numScores; i++) {
	    if (!fixLambdas[i]) {
		lambdas[i] = points[0][j];
		if (loop == 1) {
		    simplex[j] = lambdaSteps[i];
		} else if (fabs(prevPoints[j] - points[0][j])
					> 1.3 * fabs(simplex[j]))
		{
		    simplex[j] = points[0][j] - prevPoints[j];
		} else {
		    simplex[j] = simplex[j] * 1.3;
		}
		prevPoints[j] = points[0][j];

		if (debug >= DEBUG_TRAIN) {
		    cerr << "lambda_" << i << " " << points[0][j]
			  << " " << simplex[j] << endl;
		}

		j++;
	    }
	}

	if (debug >= DEBUG_TRAIN) {
	    cerr << "scale " << points[0][0] << endl;
	    cerr << "errors " << errs[0] << endl;
	    cerr << "unchanged for " << same << " iterations " << endl;
	}

	if (nevals >= maxIters) {
	    cerr << "maximum number of iterations exceeded" << endl;
	    loop = 0;
	}

	if (reps > maxAmoebaRestarts) {
	    cerr << "maximum number of Amoeba restarts reached" << endl;
	    loop = 0;
	}
    }

    for (unsigned i = 0; i <= numFreeWeights; i++) {
	delete points[i];
	delete dir[i];
    }

    // Scale the lambdas back
    for (unsigned i = 0; i < numScores; i++) {
	if (!fixLambdas[i]) {
	    lambdas[i] /= posteriorScale;
	}
    }
}

/*
 * output 1-best hyp
 */
void
printTop1bestHyp(File &file, RefString id, NBestScore **scores,
							NBestList &nbest)
{
    unsigned numHyps = nbest.numHyps();
    unsigned bestHyp;
    LogP bestScore;

    fprintf(file, "%s", id);

    /*
     * Find hyp with highest score
     */
    for (unsigned i = 0; i < numHyps; i ++) {
	LogP score = hypScore(i, scores);

	if (i == 0 || score > bestScore) {
	    bestScore = score;
	    bestHyp = i;
	}
    }

    if (numHyps > 0) {
	VocabIndex *hyp = nbest.getHyp(bestHyp).words;

	for (unsigned j = 0; hyp[j] != Vocab_None; j ++) {
	    fprintf(file, " %s", nbest.vocab.getWord(hyp[j]));
	}
    }
    fprintf(file, "\n");
}

/*
 * output best sausage hypotheses
 */
void
printTopSausageHyp(File &file, RefString id, NBestScore **scores,
							WordMesh &alignment)
{
    fprintf(file, "%s", id);

    /* 
     * process all positions in alignment
     */
    for (unsigned pos = 0; pos < alignment.length(); pos++) {
	VocabIndex bestWord = Vocab_None;
	Prob bestScore = 0.0;

	WordMeshIter iter(alignment, pos);

	Array<HypID> *hypMap;
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

	if (oneBest) {
	    printTop1bestHyp(file, id, *scores, *nbest);
	} else {
	    WordMesh **alignment = nbestAlignments.find(id);
	    assert(alignment != 0);

	    printTopSausageHyp(file, id, *scores, **alignment);
	}
    }
}

/*
 * Read a single score file into a column of the score matrix
 */
Boolean
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
	    /*
	     * Read decipher scores as floats event though they are supposed
	     * to be int's.  This way we accomodate some preexisting rescoring
	     * programs.
	     */
	    if (sscanf(line, "(%lf)", &score) != 1) {
		file.position() << "bad Decipher score: " << line << endl;
		break;
	    } else  {
		scores[hypNo ++] = BytelogToLogP((int)score);
	    }
	} else {
	    if (sscanf(line, "%lf", &score) != 1) {
		file.position() << "bad score: " << line << endl;
		break;
	    } else  {
		scores[hypNo ++] = score;
	    }
	}
    }

    /* 
     * Set missing scores to zero
     */
    if (!file.error() && hypNo < numHyps) {
	cerr << "warning: " << (numHyps - hypNo) << " scores missing from "
	     << fileName << endl;
    }
	
    while (hypNo < numHyps) {
	scores[hypNo ++] = 0;
    }

    return !file.error();
}

/*
 * Read error counts file
 */
Boolean
readErrorsFile(const char *errorsDir, RefString id, NBestList &nbest,
							unsigned &numWords)
{
    unsigned numHyps = nbest.numHyps();
    char fileName[strlen(errorsDir) + 1 + strlen(id) + strlen(GZIP_SUFFIX) + 1];
					
    sprintf(fileName, "%s/%s", errorsDir, id);

    /* 
     * If plain file doesn't exist try gzipped version
     */
    if (access(fileName, F_OK) < 0) {
	strcat(fileName, GZIP_SUFFIX);
    }

    File file(fileName, "r", 0);

    char *line;
    unsigned hypNo = 0;

    while (!file.error() && (line = file.getline())) {

	if (hypNo >= numHyps) {
	    break;
	}

	/*
	 * parse errors line
	 */
	float corrRate, errRate;
	unsigned numSub, numDel, numIns, numErrs, numWds;

	if (sscanf(line, "%f %f %u %u %u %u %u", &corrRate, &errRate,
			 &numSub, &numDel, &numIns, &numErrs, &numWds) != 7)
	{
	    file.position() << "bad errors: " << line << endl;
	    return 0;
	} else if (hypNo > 0 && numWds != numWords) {
	    file.position() << "inconsistent number of words: " << line << endl;
	    return 0;
	} else {
	    if (hypNo == 0) {
		numWords = numWds;
	    }
	    nbest.getHyp(hypNo ++).numErrors = numErrs;
	}
    }

    if (hypNo < numHyps) {
	file.position() << "too few errors lines" << endl;
	return 0;
    }

    return !file.error();
}

typedef struct {
    LogP score;
    unsigned rank;
} HypRank;			/* used in sorting nbest hyps by score */

static int
compareHyps(const void *h1, const void *h2)
{
    LogP score1 = ((HypRank *)h1)->score;
    LogP score2 = ((HypRank *)h2)->score;
    
    return score1 > score2 ? -1 :
		score1 < score2 ? 1 : 0;
}

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    argc = Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (!nbestFiles) {
	cerr << "cannot proceed without nbest files\n";
	exit(2);
    }
    if (!oneBest && !refFile) {
	cerr << "cannot proceed without references\n";
	exit(2);
    }
    if (oneBest && !refFile && !errorsDir) {
	cerr << "cannot proceed without references or error counts\n";
	exit(2);
    }

    if (oneBest && !initSimplex) {
	cerr << "1-best optimization only supported in simplex mode\n";
	exit(2);
    }

    Vocab vocab;
    NullLM nullLM(vocab);
    RefList refs(vocab);

    NBestSet trainSet(vocab, refs, maxNbest, false, multiwords);
    trainSet.debugme(debug);
    trainSet.warn = false;	// don't warn about missing refs

    if (vocabFile) {
	File file(vocabFile, "r");
	vocab.read(file);
    }

    vocab.toLower = toLower ? true : false;

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
    numFixedWeights = 0;

    lambdas[0] = 1/posteriorScale;
    lambdas[1] = rescoreLMW/posteriorScale;
    lambdas[2] = rescoreWTW/posteriorScale;

    for (unsigned i = 0; i < 3; i ++) {
	fixLambdas[i] = false;
	lambdaSteps[i] = 1.0;
    }

    for (unsigned i = 3; i < numScores; i ++) {
	lambdas[i] = 0.0;
	fixLambdas[i] = false;
	lambdaSteps[i] = 1.0;
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
		numFixedWeights++;
	    } else if (sscanf(&initLambdas[offset], "%lf%n",
						&lambdas[i], &consumed) > 0)
	    {
	        lambdas[i] /= posteriorScale;
		lambdaSteps[i] = 1.0;
	    } else {
		break;
	    }
	    offset += consumed;
	}
    }

    /*
     * Initialize simplex points
     */
    if (initSimplex) {
	unsigned offset = 0;

	unsigned consumed = 0;
	for (unsigned i = 0; i < numScores; i++) {

	    if (!fixLambdas[i]) {
	        if (sscanf(&initSimplex[offset], "%lf%n",
					&lambdaSteps[i], &consumed) <= 0)
		{
		    break;
		}

	        if (lambdaSteps[i] == 0.0) {
		    cerr << "Fixing " << i << "th parameter\n";
		    fixLambdas[i] = true;
		    numFixedWeights++;
		}

		offset += consumed;
	    }
	}

	/*
	 * no sense in searching posterior scaling dimension in 1-best mode
	 */
	if (oneBest) {
	    posteriorScaleStep = 0.0;
	}

	if (oneBest || sscanf(&initSimplex[offset], "%lf%n",
				       &posteriorScaleStep, &consumed) <= 0)
	{
	    cerr << "Posterior scale step size set to " << posteriorScaleStep
	         << endl;
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
	    if (!readScoreFile(argv[i], id, scores[i + 2], nbest->numHyps())) {
		cerr << "warning: error reading scores for " << id
		     << " from " << argv[i] << endl;
	    }
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

	/*
	 * Copy combined scores back into N-best list acoustic score for
	 * posterior probability computation (since computePosteriors()
	 * doesn't take additional scores).
	 */
	for (unsigned j = 0; j < nbest->numHyps(); j ++) {
	    nbest->getHyp(j).acousticScore = hypScore(j, scores);
	}
    }

    if (debug >= DEBUG_SCORES) {
	dumpScores(cerr, trainSet);
    }

    if (oneBest) {
	cerr << (errorsDir ? "reading" : "computing") << " error counts...\n";
    } else {
	cerr << "aligning nbest lists...\n";
    }

    iter.init();

    numRefWords = 0;

    /*
     * Create multiple alignments
     */
    while (nbest = iter.next(id)) {
	unsigned numWords;
	VocabIndex *ref = refs.findRef(id);

	if (!ref && (!oneBest || oneBest && !errorsDir)) {
	    cerr << "missing reference for " << id << endl;
	    exit(1);
	}

	/*
	 * Remove pauses and noise from nbest hyps since these would
	 * confuse the inter-hyp alignments.
	 */
	nbest->removeNoise(nullLM);

	/*
	 * In 1-best mode we only need the error counts for each hypothesis;
	 * in sausage (default) mode we need to construct multiple alignment
	 * of reference and all n-best hyps.
	 */
	if (oneBest) {
	    if (errorsDir) {
		/*
		 *  read error counts 
		 */
		if (!readErrorsFile(errorsDir, id, *nbest, numWords)) {
		    cerr << "couldn't get error counts for " << id << endl;
		    exit(2);
		}
	    } else {
		/*
		 * need to recompute hyp errors (after removeNoise() above)
		 */
		unsigned sub, ins, del;
		nbest->wordError(ref, sub, ins, del);

		numWords = Vocab::length(ref);
	    }
	} else {
	    unsigned numHyps = nbest->numHyps();

	    /*
	     * Sort hyps by initial scores.  (Combined initial scores are
	     * stored in acousticScore from before.)
	     * Keep hyp order outside of N-best lists, since scores must be
	     * kept in sync.
	     */
	    HypRank reordering[numHyps];
	    NBestScore ***scores = nbestScores.find(id);
	    assert(scores != 0);

	    for (unsigned j = 0; j < numHyps; j ++) {
		reordering[j].rank = j;
		reordering[j].score = nbest->getHyp(j).acousticScore;
	    }
	    if (!noReorder) {
		qsort(reordering, numHyps, sizeof(HypRank), compareHyps);
	    }
	    
	    /*
	     * compute posteriors for passing to alignWords().
	     * Note: these now reflect all scores and initial lambdas.
	     */
	    nbest->computePosteriors(0.0, 0.0, 1.0);

	    /*
	     * create word-mesh for multiple alignment
	     */
	    WordMesh *alignment = new WordMesh(vocab);
	    assert(alignment != 0);

	    *nbestAlignments.insert(id) = alignment;

	    numWords = Vocab::length(ref);

	    /*
	     * Default is to start alignment with hyps strings,
	     * or with the reference if -align-refs-first was given.
	     *	Note we give reference posterior 1 only to constrain the
	     *	alignment. The loss computation in training ignores the
	     *	posteriors assigned to hyps at this point.
	     */
	    HypID hypID;

	    if (noReorder) {
	    	hypID = refID;
		alignment->alignWords(ref, 1.0, 0, &hypID);
	    }

	    /*
	     * Now align all N-best hyps, in order of decreasing scores
	     */
	    for (unsigned j = 0; j < numHyps; j ++) {
		unsigned hypRank = reordering[j].rank;
		NBestHyp &hyp = nbest->getHyp(hypRank);

		hypID = hypRank;

		/*
		 * Check for overflow in the hypIDs
		 */
		if ((unsigned)hypID != hypRank || hypID == refID) {
		    cerr << "Sorry, too many hypotheses in " << id << endl;
		    exit(2);
		}

		alignment->alignWords(hyp.words, hyp.posterior, 0, &hypID);
	    }

	    if (!noReorder) {
		hypID = refID;
		alignment->alignWords(ref, 1.0, 0, &hypID);
	    }

	    if (debug >= DEBUG_ALIGNMENT) {
		dumpAlignment(cerr, *alignment);
	    }
	}

	/*
	 * compute total length of references for later normalizations
	 */
	numRefWords += numWords;
    }

    cerr << numRefWords << " reference words\n";

    /*
     * preemptive trouble avoidance: prevent division by zero
     */
    if (numRefWords == 0) {
	numRefWords = 1;
    }

    unsigned errors = (int) computeErrors(trainSet, lambdas.data());
    printLambdas(cout, lambdas);

    if (initSimplex == 0) {
	train(trainSet);
    } else {
	trainAmoeba(trainSet);
    }

    cout << "original errors = " << errors
	 << " (" << ((double)errors/numRefWords) << "/word)"
	 << endl;
    cout << "best errors = " << bestError
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

