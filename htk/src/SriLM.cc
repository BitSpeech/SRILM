/*-------------------------------------------------------- */
/*                                                         */
/*     Author : Andreas Stolcke, SRI International         */
/*                                                         */
/*---------------------------------------------------------*/
/*                                                         */
/*       File : SriLM.cc                                   */
/*              HTK-LM/SRI-LM interface                    */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,1997 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /export/di/ws97/tools/srilm-0.97beta/htk/src/RCS/SriLM.cc,v 1.13 1997/07/25 00:16:11 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "option.h"
#include "File.h"
#include "Boolean.h"
#include "LM.h"
#include "Ngram.h"
#include "DFNgram.h"
#include "BayesMix.h"
#include "NullLM.h"
#include "LHash.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(VocabString,VocabString);
#endif

extern "C" {

#define LANG_ERR     5000

/*
 * Avoid naming conflict 
 */
#define Vocab HTK_Vocab
#define Boolean HTK_Boolean
#define Array HTK_Array

#include "include.h"
#include "SriLM.h"

#undef Vocab
#undef Boolean
#undef Array

static inline
VocabIndex
WordIndex(Word *word, Vocab *vocab)
{
    if (word == 0) {
	return vocab->unkIndex;
    } else {
	return (VocabIndex)word->lid;
    }
}

static
VocabIndex *
PathIndices(Path *path, Vocab *vocab)
{
    static VocabIndex sentence[maxWordsPerLine+1];

    int i = 0;
    for ( ; path; path = path->prev) {
	sentence[i++] = WordIndex(path->word, vocab);
	assert(i <= maxWordsPerLine);
    }
    sentence[i] = Vocab_None;
    return sentence;
}

#define MAX_ARG_LEN	1000
#define MAX_ARG_NUM	1000

/*
 * Search control parameters
 */
static int nosearch = 0;
static int norebuild = 0;
static double rescore = 0.0;
static char *searchlmfile = 0;
static LM *searchlm = 0;
static int searchorder = 0;
static char *vocabMapFile = 0;

/*
 * LM parameters (same as in ngram(1))
 */
static int order = 3;
static int debug = 0;
static char *lmFile  = 0;
static char *mixFile  = 0;
static char *mixFile2 = 0;
static char *mixFile3 = 0;
static char *mixFile4 = 0;
static char *mixFile5 = 0;
static int bayesLength = -1;
static double bayesScale = 1.0;
static double mixLambda = 0.5;
static double mixLambda2 = 0.0;
static double mixLambda3 = 0.0;
static double mixLambda4 = 0.0;
static double mixLambda5 = 0.0;
static int memuse = 0;
static int df = 0;
static int toLower = 0;
static int null = 0;

static Option options[] = {
    /*
     * Search control parameters 
     */
    { OPT_TRUE, "nosearch", &nosearch, "nbest rescoring (no lattice search)" },
    { OPT_TRUE, "norebuild", &norebuild, "skip rebuild lattice for lookahead" },
    { OPT_FLOAT, "rescore", &rescore, "use LM for n-best rescoring (arg is LM weight)" },
    { OPT_INT, "searchorder", &searchorder, "max ngram order in lattice search" },
    { OPT_STRING, "searchlm", &searchlmfile, "separate LM used for lattice search" },
    { OPT_STRING, "vocab-map", &vocabMapFile, "vocabulary map" },

    /*
     * LM parameters (same as in ngram(1))
     */
    { OPT_INT, "order", &order, "max ngram order" },
    { OPT_INT, "debug", &debug, "debugging level for lm" },
    { OPT_TRUE, "df", &df, "use disfluency ngram model" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_STRING, "lm", &lmFile, "file in ARPA LM format" },
    { OPT_INT, "bayes", &bayesLength, "context length for Bayes mixture LM" },
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
    { OPT_TRUE, "null", &null, "use a null language model" },

    { OPT_TRUE, "memuse", &memuse, "show memory usage" },
};


/*
 * Build mixture LM (from ngram.cc)
 */
LM *
makeMixLM(const char *filename, Vocab &vocab, unsigned order,
						LM *oldLM, double lambda)
{
    File file(filename, "r", 0);

    if (file.error()) {
	HError(LANG_ERR+1, "Error opening mix-lm file %s", filename);
	return 0;
    }

    Ngram *lm = new Ngram(vocab, order);
    assert(lm != 0);

    lm->debugme(debug);

    if (!lm->read(file)) {
	HError(LANG_ERR+1, "Format error in mix-lm file %s", filename);
	return 0;
    }

    if (oldLM) {
	if (bayesLength >= 0) {
	    /*
	     * On the fly interpolation using Bayes
	     */
	    LM *newLM = new BayesMix(vocab, *lm, *oldLM,
				    bayesLength, lambda, bayesScale);
	    assert(newLM != 0);
	    newLM->debugme(debug);
	    return newLM;
	} else {
	    /*
	     * Static interpolation
	     */
	    Ngram *newLM = new Ngram(vocab, order);
	    assert(newLM != 0);

	    newLM->debugme(debug);

	    /*
	     * XXX: Should check that oldLM is Ngram
	     */
	    newLM->mixProbs(*lm, *(Ngram *)oldLM, lambda);

	    delete oldLM;
	    delete lm;

	    return newLM;
	}
	   
    } else {
	return lm;
    }
}

/*
 * Read vocabulary map
 */
void
readVocabMap(LHash<VocabString,VocabString> &map, File &file)
{
    char *line;

    while (line = file.getline()) {
	VocabString entry[2];

	unsigned nwords = Vocab::parseWords(line, entry, 2);

	if (nwords != 2) {
	    file.position() << "vocab map needs two words per line\n";
	    continue;
	}
		
	*map.insert(entry[0]) = strdup(entry[1]);

	assert(map.find(entry[0]) != 0);
    }
}

int
ReadSriLM(Language *lang,FILE *file)
{
    int flags;
    int argc = 1;
    char arg[MAX_ARG_LEN];
    char *argv[MAX_ARG_NUM];

    /*
     * Parse the contents of the LM file into an argv-style array
     */
    argv[0] = "SRILM";
    while (argc + 1 < MAX_ARG_NUM &&
	   fscanf(file," %999s ", arg) == 1)
    {
	argv[argc++] = strdup(arg);
    }
    argv[argc] = (char *)0;

    /*
     * Parse the description into options and values
     */
    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    /*
     * Create our own Vocab object
     */
    Vocab *vocab = new Vocab;
    assert(vocab != 0);

    vocab->toLower = toLower ? true : false;

    /*
     * Read the vocabulary map, if any
     */
    LHash<VocabString,VocabString> vocabMap;

    if (vocabMapFile) {
	File file(vocabMapFile, "r", 0);

	if (file.error()) {
	    HError(LANG_ERR+1,"Error opening vocab map file %s", vocabMapFile);
	    return (0);
	}

	readVocabMap(vocabMap, file);
    }
    
    /*
     * the vocabulary of the LM has already been initialized,
     * so transfer it to our own vocab.  the indices assigned by
     * our Vocab get stored as the "lid" values for later ease of
     * reference (note they must not exceeed 64K).
     */
    for (Word *word = lang->vocab->words; word != 0; word = word->chain) {
	/*
	 * Map HTK sentence-begin/end to those found in the Ngram file
	 */
	if (word == lang->vocab->start) {
	    word->lid = vocab->ssIndex;
	} else if (word == lang->vocab->end) {
	    word->lid = vocab->seIndex;
	} else {
	    /*
	     * Check vocabulary map before inserting string into our
	     * vocabulary
	     */
	    VocabString *mapped = vocabMap.find(word->sym->name);
	    VocabIndex idx = vocab->addWord(mapped ? *mapped : word->sym->name);

	    word->lid = (unsigned short)idx;
	    assert(idx == (VocabIndex)word->lid);
	}
    }

    unsigned vocabSize = vocab->numWords();

    /*
     * Build the LM object according to specified options
     */
    Ngram *ngramLM;
    LM *useLM;

    ngramLM = df ? new DFNgram(*vocab, order) :
		   new Ngram(*vocab, order);
    assert(ngramLM != 0);

    ngramLM->debugme(debug);

    if (null) {
	useLM = new NullLM(*vocab);
	assert(useLM != 0);
    } else if (lmFile) {
	if (mixFile) {
	    /*
	     * create a Bayes mixture LM 
	     */

	    double totalLambda = 0.0;
	    useLM = 0;

	    if (mixFile5) {
		totalLambda = mixLambda5;
		useLM = makeMixLM(mixFile5, *vocab, order, useLM, 0);
		if (useLM == 0) return (0);
	    }
	    if (mixFile4) {
		totalLambda += mixLambda4;
		useLM = makeMixLM(mixFile4, *vocab, order, useLM,
						mixLambda4/totalLambda);
		if (useLM == 0) return (0);
	    }
	    if (mixFile3) {
		totalLambda += mixLambda3;
		useLM = makeMixLM(mixFile3, *vocab, order, useLM,
						mixLambda3/totalLambda);
		if (useLM == 0) return (0);
	    }
	    if (mixFile2) {
		totalLambda += mixLambda2;
		useLM = makeMixLM(mixFile2, *vocab, order, useLM,
						mixLambda2/totalLambda);
		if (useLM == 0) return (0);
	    }
	    if (mixFile) {
		useLM = makeMixLM(mixFile, *vocab, order, useLM,
					1.0 - totalLambda - mixLambda);
		if (useLM == 0) return (0);
	    }
	    useLM = makeMixLM(lmFile, *vocab, order, useLM, mixLambda);
	    if (useLM == 0) return (0);

	} else {
	    /*
	     * Read just a single LM
	     */
	    File file(lmFile, "r", 0);

	    if (file.error()) {
		HError(LANG_ERR+1,"Error opening -lm file %s", lmFile);
		return (0);
	    }

	    if (!ngramLM->read(file)) {
		HError(LANG_ERR+1,"Format error in -lm file %s", lmFile);
		return (0);
	    }

	    useLM = ngramLM;
	}
    } else {
	HError(LANG_ERR+1,"Need at least an -lm file specified");
	return (0);
    }


    if (memuse) {
	MemStats memuse;
	useLM->memStats(memuse);
	memuse.print();
    }


    /*
     * Hook LM object into the Language structure
     */
    lang->info = useLM;

    /*
     * If requested, load a separate language model for lattice search
     * only.  It will be used to compute conditional word probabilities,
     * and its score will be subtracted off of the final score to
     * so the final ranking is determined by the 'real' model alone.
     */
    if (searchlmfile) {
	/*
	 * The search lm is always an ngram, by default of the same
	 * order as the main LM.
	 */
	if (strcmp(searchlmfile, "SAME") == 0) {
	    searchlm = (LM *)lang->info;
	} else {
	    searchlm = new Ngram(*vocab, searchorder ? searchorder : order);

	    assert(searchlm != 0);

	    searchlm->debugme(debug);

	    {
		File file(searchlmfile, "r", 0);

		if (file.error()) {
		    HError(LANG_ERR+1,"Error opening %s", searchlmfile);
		    return (0);
		}

		if (!searchlm->read(file)) {
		    HError(LANG_ERR+1,"Format error in %s", searchlmfile);
		    return (0);
		}
	    }

	    if (memuse) {
		MemStats memuse;
		searchlm->memStats(memuse);
		memuse.print();
	    }
	}
    }

    unsigned newVocabSize = vocab->numWords();

    if (newVocabSize > vocabSize) {
	cerr << "WARNING: LM contains " << (newVocabSize - vocabSize)
	     << " words not in HTK vocabulary\n";
    }

    if (nosearch) {
	flags = ls_start | ls_end | ls_fullprob; /* First do N-best rescoring */
    } else {
	flags = ls_start | ls_end | ls_prob | ls_reptr | ls_aplat;
					       /* Later actual search */

	if (!null) {
	    flags |= ls_allprobs;
	}
	if (searchlm) {
	    flags |= ls_fullprob;
	}
    }
    return(flags);
}

void
StartSriLM(Language *lang,char *datafn)
{
}

void
EndSriLM(Language *lang)
{
}

float
SriLMFullProb(Language *lang,Path *path)
{
    LM *lm = (LM *)lang->info;
    VocabIndex sentence[maxWordsPerLine+1];
    TextStats sentStats;
    float ngramScore = 0.0;

    /*
     * Skip sentence-end symbol
     */
    if (path && path->word == lang->vocab->end) {
	if (path->ll) {
	    ngramScore += path->ll->ngprob;
	}
	path = path->prev;
    }

    /*
     * Translate into indices word-by-word, stopping before
     * the sentence-start symbol
     */
    int i = 0;
    for ( ; path && path->word != lang->vocab->start; path = path->prev) {
	if (path->ll) {
	    ngramScore += path->ll->ngprob;
	}
	sentence[i++] = WordIndex(path->word, &lm->vocab);
	assert(i <= maxWordsPerLine);
    }
    sentence[i] = Vocab_None;

    Vocab::reverse(sentence);

    LogP prob = lm->sentenceProb(sentence, sentStats);

    /*
     * Subtract the previously computed product of conditional
     * word probabilities computed by the search lm
     */
    if (searchlm) {
	TextStats sentStats;

	searchlm->debug(0);
	prob -= searchlm->sentenceProb(sentence, sentStats);
    }

    /*
     * When doing n-best rescoring we need to subtract the score
     * computed from the lattice ngram scores (scaled to be 
     * compatible with our own language model weight, which is
     * supplied as the -rescore argument).
     */
    if (rescore != 0.0) {
       ngramScore /= rescore;
    } else {
       ngramScore = 0.0;
    }

    /*
     * sentenceProb skips over OOVs and zero probability words
     * and reports those seperately, but here we just return a 
     * number. 
     * Also, convert from log10 to log-e representation.
     */
    if (sentStats.zeroProbs > 0) {
	return LogP_Zero * M_LN10 - ngramScore;
    } else {
	return prob * M_LN10 - ngramScore;
    }
}

float
SriLMProb(Language *lang,Word *word,Path *path)
{
    LM *lm;
    
    if (searchlm) {
	lm = searchlm;
    } else {
	lm = (LM *)lang->info;
    }

    /*
     * Translate both word and path to their Vocab indices
     */
    return lm->wordProb(WordIndex(word, &lm->vocab),
			PathIndices(path, &lm->vocab)) * M_LN10;
}

float *
SriLMAllProbs(Language *lang,Path *path)
{
    static float *allProbs = 0;

    LM *lm;
    if (searchlm) {
	lm = searchlm;
    } else {
	lm = (LM *)lang->info;
    }

    VocabIndex *context = PathIndices(path, &lm->vocab);

    if (!allProbs) {
	allProbs = new float[lm->vocab.highIndex() + 1](LogP_Zero * M_LN10);
	assert(allProbs != 0);
    }

    Word *word;
    int first = 1;
    for (word = lang->vocab->words; word != 0; word = word->chain) {
	if (first) {
	    allProbs[word->lid] =
		lm->wordProb(WordIndex(word, &lm->vocab), context) * M_LN10;
	    first = 0;
	} else {
	    allProbs[word->lid] =
	         lm->wordProbRecompute(WordIndex(word, &lm->vocab), context) *
				M_LN10;
	}
    }

    return allProbs;
}

void *
SriLMRePtr(Language *lang,Path *path)
{
    LM *lm;
    if (searchlm) {
	lm = searchlm;
    } else {
	lm = (LM *)lang->info;
    }

    VocabIndex *context = PathIndices(path, &lm->vocab);

    return lm->contextID(context);
}

/* EXPORT->ApLatNgram: Rebuild lattice to incorporate exact NGram  */
/*   likelihoods and calculate exact lookahead values.             */
Lattice *
ApLatSriLM(Language *lang,Lattice *lat)
{
    int i;
    LatLink *ll;
    Word *word;
    Lattice *newlat;

    if (norebuild) {
	return (Lattice *)0;
    } else {
	newlat=LangRebuildLat(lat,lang);
			/* Function used by JLRebuild to apply */
		        /*  new language model to lattice */
	for(i=0,ll=newlat->llinks;i<newlat->nl;i++,ll++) {
            ll->lahead=ll->acprob+ll->ngprob+ll->prprob+ll->lmprob+lang->pen;
	}

	BestLAhead(-1,newlat,NULL);

	return(newlat);
    }
}

} // extern "C"
