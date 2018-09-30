/*
 * ngram-count --
 *	Create and manipulate word ngram counts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/ngram-count.cc,v 1.31 1999/08/01 09:33:14 stolcke Exp $";
#endif

#include <stdlib.h>
#include <iostream.h>
#include <locale.h>

#include "option.h"
#include "File.h"
#include "Vocab.h"
#include "Ngram.h"
#include "VarNgram.h"
#include "TaggedNgram.h"
#include "SkipNgram.h"
#include "StopNgram.h"
#include "NgramStats.h"
#include "TaggedNgramStats.h"
#include "StopNgramStats.h"
#include "Discount.h"

const unsigned maxorder = 6;		/* this is only relevant to the 
					 * the -gt<n> and -write<n> flags */

static char *filetag = 0;
static int order = 3;
static unsigned debug = 0;
static char *textFile = 0;
static char *readFile = 0;

static int writeOrder = 0;		/* default is all ngram orders */
static char *writeFile[maxorder+1];

static unsigned gtmin[maxorder+1] = {1, 1, 1, 2, 2, 2, 2};
static unsigned gtmax[maxorder+1] = {5, 1, 7, 7, 7, 7, 7};

static double cdiscount[maxorder+1] = {-1, -1, -1, -1, -1, -1, -1};
static int ndiscount[maxorder+1] = {0, 0, 0, 0, 0, 0, 0};
static int wbdiscount[maxorder+1] = {0, 0, 0, 0, 0, 0, 0};

static char *gtFile[maxorder+1];
static char *lmFile = 0;
static char *initLMFile = 0;

static char *vocabFile = 0;
static char *writeVocab = 0;
static int memuse = 0;
static int recompute = 0;
static int sort = 0;
static int keepunk = 0;
static int tagged = 0;
static int tolower = 0;
static int trustTotals = 0;
static double prune = 0.0;
static int minprune = 2;
static int useFloatCounts = 0;

static double varPrune = 0.0;

static int skipNgram = 0;
static double skipInit = 0.5;
static int maxEMiters = 100;
static double minEMdelta = 0.001;

static char *stopWordFile = 0;
static char *dummyTag = 0;

static Option options[] = {
    { OPT_INT, "order", &order, "max ngram order" },
    { OPT_FLOAT, "varprune", &varPrune, "pruning threshold for variable order ngrams" },
    { OPT_INT, "debug", &debug, "debugging level for LM" },
    { OPT_TRUE, "recompute", &recompute, "recompute lower-order counts by summation" },
    { OPT_TRUE, "sort", &sort, "sort ngrams output" },
    { OPT_INT, "write-order", &writeOrder, "output ngram counts order" },
    { OPT_STRING, "tag", &filetag, "file tag to use in messages" },
    { OPT_STRING, "text", &textFile, "text file to read" },
    { OPT_STRING, "read", &readFile, "counts file to read" },

    { OPT_STRING, "write", &writeFile[0], "counts file to write" },
    { OPT_STRING, "write1", &writeFile[1], "1gram counts file to write" },
    { OPT_STRING, "write2", &writeFile[2], "2gram counts file to write" },
    { OPT_STRING, "write3", &writeFile[3], "3gram counts file to write" },
    { OPT_STRING, "write4", &writeFile[4], "4gram counts file to write" },
    { OPT_STRING, "write5", &writeFile[5], "5gram counts file to write" },
    { OPT_STRING, "write6", &writeFile[6], "6gram counts file to write" },

    { OPT_INT, "gtmin", &gtmin[0], "lower GT discounting cutoff" },
    { OPT_INT, "gtmax", &gtmax[0], "upper GT discounting cutoff" },
    { OPT_INT, "gt1min", &gtmin[1], "lower 1gram discounting cutoff" },
    { OPT_INT, "gt1max", &gtmax[1], "upper 1gram discounting cutoff" },
    { OPT_INT, "gt2min", &gtmin[2], "lower 2gram discounting cutoff" },
    { OPT_INT, "gt2max", &gtmax[2], "upper 2gram discounting cutoff" },
    { OPT_INT, "gt3min", &gtmin[3], "lower 3gram discounting cutoff" },
    { OPT_INT, "gt3max", &gtmax[3], "upper 3gram discounting cutoff" },
    { OPT_INT, "gt4min", &gtmin[4], "lower 4gram discounting cutoff" },
    { OPT_INT, "gt4max", &gtmax[4], "upper 4gram discounting cutoff" },
    { OPT_INT, "gt5min", &gtmin[5], "lower 5gram discounting cutoff" },
    { OPT_INT, "gt5max", &gtmax[5], "upper 5gram discounting cutoff" },
    { OPT_INT, "gt6min", &gtmin[6], "lower 6gram discounting cutoff" },
    { OPT_INT, "gt6max", &gtmax[6], "upper 6gram discounting cutoff" },

    { OPT_STRING, "gt", &gtFile[0], "Good-Turing discount parameter file" },
    { OPT_STRING, "gt1", &gtFile[1], "Good-Turing 1gram discounts" },
    { OPT_STRING, "gt2", &gtFile[2], "Good-Turing 2gram discounts" },
    { OPT_STRING, "gt3", &gtFile[3], "Good-Turing 3gram discounts" },
    { OPT_STRING, "gt4", &gtFile[4], "Good-Turing 4gram discounts" },
    { OPT_STRING, "gt5", &gtFile[5], "Good-Turing 5gram discounts" },
    { OPT_STRING, "gt6", &gtFile[6], "Good-Turing 6gram discounts" },

    { OPT_FLOAT, "cdiscount", &cdiscount[0], "discounting constant" },
    { OPT_FLOAT, "cdiscount1", &cdiscount[1], "1gram discounting constant" },
    { OPT_FLOAT, "cdiscount2", &cdiscount[2], "2gram discounting constant" },
    { OPT_FLOAT, "cdiscount3", &cdiscount[3], "3gram discounting constant" },
    { OPT_FLOAT, "cdiscount4", &cdiscount[4], "4gram discounting constant" },
    { OPT_FLOAT, "cdiscount5", &cdiscount[5], "5gram discounting constant" },
    { OPT_FLOAT, "cdiscount6", &cdiscount[6], "6gram discounting constant" },

    { OPT_TRUE, "ndiscount", &ndiscount[0], "use natural discounting" },
    { OPT_TRUE, "ndiscount1", &ndiscount[1], "1gram natural discounting" },
    { OPT_TRUE, "ndiscount2", &ndiscount[2], "2gram natural discounting" },
    { OPT_TRUE, "ndiscount3", &ndiscount[3], "3gram natural discounting" },
    { OPT_TRUE, "ndiscount4", &ndiscount[4], "4gram natural discounting" },
    { OPT_TRUE, "ndiscount5", &ndiscount[5], "5gram natural discounting" },
    { OPT_TRUE, "ndiscount6", &ndiscount[6], "6gram natural discounting" },

    { OPT_TRUE, "wbdiscount", &wbdiscount[0], "use Witten-Bell discounting" },
    { OPT_TRUE, "wbdiscount1", &wbdiscount[1], "1gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount2", &wbdiscount[2], "2gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount3", &wbdiscount[3], "3gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount4", &wbdiscount[4], "4gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount5", &wbdiscount[5], "5gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount6", &wbdiscount[6], "6gram Witten-Bell discounting"},
    { OPT_STRING, "lm", &lmFile, "LM to estimate" },
    { OPT_STRING, "init-lm", &initLMFile, "initial LM for EM estimation" },
    { OPT_TRUE, "unk", &keepunk, "keep <unk> in LM" },
    { OPT_STRING, "dummy", &dummyTag, "dummy tag used to mark backoff ngram counts" },
    { OPT_TRUE, "float-counts", &useFloatCounts, "use fractional counts" },
    { OPT_TRUE, "tagged", &tagged, "build a tagged LM" },
    { OPT_TRUE, "skip", &skipNgram, "build a skip N-gram LM" },
    { OPT_FLOAT, "skip-init", &skipInit, "default initial skip probability" },
    { OPT_INT, "em-iters", &maxEMiters, "max number of EM iterations" },
    { OPT_FLOAT, "em-delta", &minEMdelta, "min log likelihood delta for EM" },
    { OPT_STRING, "stop-words", &stopWordFile, "stop-word vocabulary for stop-Ngram LM" },

    { OPT_TRUE, "tolower", &tolower, "map vocabulary to lowercase" },
    { OPT_TRUE, "trust-totals", &trustTotals, "trust lower-order counts for estimation" },
    { OPT_FLOAT, "prune", &prune, "prune redundant probs" },
    { OPT_INT, "minprune", &minprune, "prune only ngrams at least this long" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_STRING, "write-vocab", &writeVocab, "write vocab to file" },
    { OPT_TRUE, "memuse", &memuse, "show memory usage" },
    { OPT_DOC, 0, 0, "the default action is to write counts to stdout" }
};

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    Boolean written = false;

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (useFloatCounts + tagged + skipNgram +
	(stopWordFile != 0) + (varPrune != 0.0) > 1)
    {
	cerr << "fractional counts, variable, tagged, stop-word Ngram and skip N-gram models are mutually exclusive\n";
	exit(2);
    }

    Vocab *vocab = tagged ? new TaggedVocab : new Vocab;
    assert(vocab);

    vocab->unkIsWord = keepunk ? true : false;
    vocab->toLower = tolower ? true : false;

    if (dummyTag) {
	vocab->addWord(dummyTag);
    }

    SubVocab *stopWords = 0;

    if (stopWordFile != 0) {
	stopWords = new SubVocab(*vocab);
	assert(stopWords);

	stopWords->remove(stopWords->ssIndex);
	stopWords->remove(stopWords->seIndex);
    }

    /*
     * The skip-ngram model requires count order one higher than
     * the normal model.
     */
    NgramStats *intStats =
	(stopWords != 0) ? new StopNgramStats(*vocab, *stopWords, order) :
	   tagged ? new TaggedNgramStats(*(TaggedVocab *)vocab, order) :
	      useFloatCounts ? 0 :
	         new NgramStats(*vocab, skipNgram ? order + 1 : order);
    NgramCounts<FloatCount> *floatStats =
	      !useFloatCounts ? 0 :
		 new NgramCounts<FloatCount>(*vocab, order);

#define STATS (useFloatCounts ? floatStats : intStats)
#define USE_STATS(what) (useFloatCounts ? floatStats->what : intStats->what)

    assert(STATS);

    USE_STATS(debugme(debug));

    if (vocabFile) {
	File file(vocabFile, "r");
	USE_STATS(vocab.read(file));
	USE_STATS(openVocab) = false;
    }

    if (stopWordFile) {
	File file(stopWordFile, "r");
	stopWords->read(file);
    }

    if (readFile) {
	File file(readFile, "r");
	USE_STATS(read(file));
    }

    if (textFile) {
	File file(textFile, "r");
	USE_STATS(countFile(file));
    }

    if (memuse) {
	MemStats memuse;
	USE_STATS(memStats(memuse));
	memuse.print();
    }

    if (recompute) {
	USE_STATS(sumCounts(order));
    }

    unsigned int i;
    for (i = 1; i <= maxorder; i++) {
	if (writeFile[i]) {
	    File file(writeFile[i], "w");
	    USE_STATS(write(file, i, sort));
	    written = true;
	}
    }

    /*
     * This stores the discounting parameters for the various orders
     * Note this is only needed when estimating an LM
     */
    Discount **discounts = new Discount*[order];
    assert(discounts != 0);
    for (i = 0; i < order; i ++) {
	discounts[i] = 0;
    }

    /*
     * Check for any Good Turing parameter files.
     * These have a dual interpretation.
     * If we're not estimating a new LM, simple WRITE the GT parameters
     * out.  Otherwise try to READ them from these files.
     */
    /*
     * Estimate discounting parameters 
     * Note this is only required if 
     * - the user wants them written to a file
     * - we also want to estimate a LM later
     */
    for (i = 1; i <= maxorder || i <= order; i++) {
	unsigned useorder = (i > maxorder) ? 0 : i;

	/*
	 * use natural or Witten-Bell discounting if requested, otherwise
	 * constant discounting, if that was specified,
	 * otherwise GoodTuring.
	 */
	if (ndiscount[useorder]) {
	    NaturalDiscount *nd = new NaturalDiscount(gtmin[useorder]);

	    assert(nd);
	    nd->debugme(debug);

	    if (!(useFloatCounts ? nd->estimate(*floatStats, i) :
				   nd->estimate(*intStats, i)))
	    {
		cerr << "error in Natural Discounting estimator for order "
		     << i << endl;
		exit(1);
	    }

	    if (i <= order) {
		discounts[i-1] = nd;
	    }
	} else if (wbdiscount[useorder]) {
	    WittenBell *wb = new WittenBell(gtmin[useorder]);

	    assert(wb);
	    wb->debugme(debug);

	    if (!(useFloatCounts ? wb->estimate(*floatStats, i) :
				   wb->estimate(*intStats, i)))
	    {
		cerr << "error in Witten-Bell Discounting estimator for order "
		     << i << endl;
		exit(1);
	    }

	    if (i <= order) {
		discounts[i-1] = wb;
	    }
	} else if (cdiscount[useorder] != -1.0) {
	    ConstDiscount *cd =
		new ConstDiscount(cdiscount[useorder], gtmin[useorder]);

	    assert(cd);
	    cd->debugme(debug);

	    if (i <= order) {
		discounts[i-1] = cd;
	    }
	} else if (gtFile[useorder] || (i <= order && lmFile)) {
	    GoodTuring *gt =
		new GoodTuring(gtmin[useorder], gtmax[useorder]);
	    assert(gt);
	    gt->debugme(debug);

	    if (gtFile[useorder] && lmFile) {
		File file(gtFile[useorder], "r");

		if (!gt->read(file)) {
		    cerr << "error in reading Good Turing parameter file "
			 << gtFile[useorder] << endl;
		    exit(1);
		}
	    } else {
		/*
		 * Estimate GT params, and write them only if 
		 * a file was specified, but no language model is
		 * being estimated
		 */
		if (!(useFloatCounts ? gt->estimate(*floatStats, i) :
				       gt->estimate(*intStats, i)))
		{
		    cerr << "error in Good Turing estimator for order "
			 << i << endl;
		    exit(1);
		}
		if (gtFile[useorder]) {
		    File file(gtFile[useorder], "w");
		    gt->write(file);
		    written = true;
		}
	    }

	    if (i <= order) {
		discounts[i-1] = gt;
	    }
	}
    }

    /*
     * Estimate a new model from the existing counts,
     * either using a default discounting scheme, or the GT parameters
     * read in from files
     */
    if (lmFile) {
	Ngram *lm;
	
	if (varPrune != 0.0) {
	    lm = new VarNgram(*vocab, order, varPrune);
	    assert(lm != 0);
	} else if (skipNgram) {
	    SkipNgram *skipLM =  new SkipNgram(*vocab, order);
	    assert(skipLM != 0);

	    skipLM->maxEMiters = maxEMiters;
	    skipLM->minEMdelta = minEMdelta;
	    skipLM->initialSkipProb = skipInit;

	    lm = skipLM;
	} else {
	    lm = (stopWords != 0) ? new StopNgram(*vocab, *stopWords, order) :
		       tagged ? new TaggedNgram(*(TaggedVocab *)vocab, order) :
			  new Ngram(*vocab, order);
	    assert(lm != 0);
	}

	/*
	 * Set debug level on LM object
	 */
	lm->debugme(debug);

	/*
	 * Dummy tag is used to mark ngrams that should be backed off
	 */
	if (dummyTag) {
	    lm->dummyIndex = vocab->getIndex(dummyTag);
	}

	/*
	 * Read initial LM parameters in case we're doing EM
	 */
	if (initLMFile) {
	    File file(initLMFile, "r");

	    if (!lm->read(file)) {
		cerr << "format error in init-lm file\n";
		exit(1);
	    }
	}
        
	if (trustTotals) {
	    lm->trustTotals = true;
	}
	if (!(useFloatCounts ? lm->estimate(*floatStats, discounts) :
			       lm->estimate(*intStats, discounts)))
	{
	    cerr << "LM estimation failed\n";
	    exit(1);
	} else {
	    /*
	     * Remove redundant probs (perplexity increase below threshold)
	     */
	    if (prune != 0.0) {
		lm->pruneProbs(prune, minprune);
	    }

	    File file(lmFile, "w");
	    lm->write(file);
	}
	written = true;

	// XXX: don't free the lm since this itself may take a long time
	// and we're going to exit anyways.
#ifdef DEBUG
	delete lm;
#endif
    }

    if (writeVocab) {
	File file(writeVocab, "w");
	vocab->write(file);
	written = true;
    }

    /*
     * If nothing has been written out so far, make it the default action
     * to dump the counts 
     */
    if (writeFile[0] || !written) {
	File file(writeFile[0] ? writeFile[0] : "-", "w");
	USE_STATS(write(file, writeOrder, sort));
    }

#ifdef DEBUG
    /*
     * Free all objects
     */
    for (i = 0; i < order; i ++) {
	delete discounts[i];
	discounts[i] = 0;
    }
    delete [] discounts;

    delete STATS;

    if (stopWords != 0) {
	delete stopWords;
    }

    delete vocab;
    return(0);
#endif /* DEBUG */

    exit(0);
}

