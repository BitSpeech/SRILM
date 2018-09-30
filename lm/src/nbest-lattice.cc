/*
 * nbest-lattice --
 *	Build and rerank N-Best lattices
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2001 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/nbest-lattice.cc,v 1.51 2001/06/08 05:57:49 stolcke Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>

#include "option.h"
#include "File.h"
#include "Prob.h"
#include "Vocab.h"
#include "NBest.h"
#include "NullLM.h"
#include "WordLattice.h"
#include "WordMesh.h"
#include "WordAlign.h"
#include "VocabMultiMap.h"

#define DEBUG_ERRORS		1
#define DEBUG_POSTERIORS	2

/*
 * Pseudo-posterior used to prime lattice with centroid hyp
 */
const Prob primePosterior = 100.0;

/*
 * default value for posterior* weights to indicate they haven't been set
 */
const double undefinedWeight =	(1.0/0.0);

static unsigned debug = 0;
static int werRescore = 0;
static unsigned maxRescore = 0;
static char *vocabFile = 0;
static int tolower = 0;
static int multiwords = 0;
static char *readFile = 0;
static char *writeFile = 0;
static char *rescoreFile = 0;
static char *nbestErrorFile = 0;
static char *latticeErrorFile = 0;
static char *nbestFiles = 0;
static char *writeNbestFile = 0;
static unsigned maxNbest = 0;
static double rescoreLMW = 8.0;
static double rescoreWTW = 0.0;
static double posteriorScale = 0.0;
static double posteriorAMW = 1.0;
static double posteriorLMW = undefinedWeight;
static double posteriorWTW = undefinedWeight;
static char *noiseTag = 0;
static char *noiseVocabFile = 0;
static int keepNoise = 0;
static int noMerge = 0;
static int noReorder = 0;
static double postPrune = 0.0;
static int primeLattice = 0;
static int primeWith1best = 0;
static int noViterbi = 0;
static int useMesh = 0;
static char *dictFile = 0;
static double deletionBias = 1.0;
static int dumpPosteriors = 0;
static char *refString = 0;
static int dumpErrors = 0;

static Option options[] = {
    { OPT_UINT, "debug", &debug, "debugging level" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_TRUE, "tolower", &tolower, "map vocabulary to lowercase" },
    { OPT_TRUE, "multiwords", &multiwords, "split multiwords in N-best hyps" },
    { OPT_TRUE, "wer", &werRescore, "optimize expected WER using N-best list" },
    { OPT_FALSE, "lattice-wer", &werRescore, "optimize expected WER using lattice" },
    { OPT_STRING, "read", &readFile, "lattice file to read" },
    { OPT_STRING, "write", &writeFile, "lattice file to write" },

    { OPT_STRING, "rescore", &rescoreFile, "hyp stream input file to rescore" },
    { OPT_STRING, "nbest-errors", &nbestErrorFile, "compute n-best error of hypotheses" },
    { OPT_STRING, "lattice-errors", &latticeErrorFile, "compute lattice error of hypotheses" },
    { OPT_STRING, "nbest", &rescoreFile, "same as -rescore" },
    { OPT_STRING, "write-nbest", &writeNbestFile, "output n-best list" },
    { OPT_STRING, "nbest-files", &nbestFiles, "list of n-best filenames" },
    { OPT_UINT, "max-nbest", &maxNbest, "maximum number of hyps to consider" },
    { OPT_UINT, "max-rescore", &maxRescore, "maximum number of hyps to rescore" },
    { OPT_FLOAT, "posterior-prune", &postPrune, "ignore n-best hyps whose cumulative posterior mass is below threshold" },
    { OPT_FLOAT, "rescore-lmw", &rescoreLMW, "rescoring LM weight" },
    { OPT_FLOAT, "rescore-wtw", &rescoreWTW, "rescoring word transition weight" },
    { OPT_FLOAT, "posterior-scale", &posteriorScale, "divisor for log posterior estimates" },
    { OPT_FLOAT, "posterior-amw", &posteriorAMW, "posterior AM weight" },
    { OPT_FLOAT, "posterior-lmw", &posteriorLMW, "posterior LM weight" },
    { OPT_FLOAT, "posterior-wtw", &posteriorWTW, "posterior word transition weight" },
    { OPT_TRUE, "keep-noise", &keepNoise, "do not eliminate pause and noise tokens" },
    { OPT_STRING, "noise", &noiseTag, "noise tag to skip" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary to skip" },
    { OPT_TRUE, "no-merge", &noMerge, "don't merge hyps for lattice building" },
    { OPT_TRUE, "no-reorder", &noReorder, "don't reorder N-best hyps before rescoring" },
    { OPT_TRUE, "prime-lattice", &primeLattice, "initialize word lattice with WE-minimized hyp" },
    { OPT_TRUE, "prime-with-1best", &primeWith1best, "initialize word lattice with 1-best hyp" },
    { OPT_TRUE, "no-viterbi", &noViterbi, "minimize lattice WE without Viterbi search" },
    { OPT_TRUE, "use-mesh", &useMesh, "align using word mesh (not lattice)" },
    { OPT_STRING, "dictionary", &dictFile, "dictionary to use in mesh alignment" },
    { OPT_FLOAT, "deletion-bias", &deletionBias, "bias factor in favor of deletions" },
    { OPT_TRUE, "dump-posteriors", &dumpPosteriors, "output hyp and word posteriors probs" },
    { OPT_TRUE, "dump-errors", &dumpErrors, "output word error labels" },
    { OPT_STRING, "reference", &refString, "reference words for word error computation" }
};

void
latticeRescore(MultiAlign &lat, NBestList &nbestList)
{
    unsigned totalWords = 0;
    unsigned numHyps = nbestList.numHyps();

    if (!noReorder) {
	nbestList.reweightHyps(rescoreLMW, rescoreWTW);
	nbestList.sortHyps();
    }

    nbestList.computePosteriors(posteriorLMW, posteriorWTW, posteriorScale,
								posteriorAMW);

    unsigned howmany = (maxRescore > 0) ? maxRescore : numHyps;
    if (howmany > numHyps) {
	howmany = numHyps;
    }

    Prob totalPost = 0.0;
    VocabIndex *primeWords = 0;

    /* 
     * Prime lattice with a "good hyp" to improve alignments
     */
    if (primeLattice && !noMerge && lat.isEmpty()) {
	primeWords = new VocabIndex[maxWordsPerLine + 1];
	assert(primeWords != 0);

	if (primeWith1best) {
	    /*
	     * prime with 1-best hyp
	     */
	    nbestList.reweightHyps(rescoreLMW, rescoreWTW);

	    /*
	     * locate best hyp
	     */
	    VocabIndex *bestHyp;
	    LogP bestScore;
	    for (unsigned i = 0; i < numHyps; i ++) {
		NBestHyp &hyp = nbestList.getHyp(i);

		if (i == 0 || hyp.totalScore > bestScore) {
		    bestHyp = hyp.words;
		    bestScore = hyp.totalScore;
		}
	    }

	    Vocab::copy(primeWords, bestHyp);
	} else {
	    /*
	     * prime with WE-minimized hyp -- slow!
	     */
	    double subs, inss, dels;
	    (void)nbestList.minimizeWordError(primeWords, maxWordsPerLine,
				    subs, inss, dels, maxRescore, postPrune);
	}

	lat.addWords(primeWords, primePosterior);
    }

    /*
     * Incorporate hyps into lattice
     */
    for (unsigned i = 0; i < numHyps; i ++) {
	NBestHyp &hyp = nbestList.getHyp(i);

	totalWords += Vocab::length(hyp.words);

	/*
	 * If merging is turned off or the lattice is empty (only
	 * initial/final nodes) we add fresh path to it.
	 * Otherwise merge using string alignment.
	 */
	if (noMerge || lat.isEmpty()) {
	    lat.addWords(hyp.words, hyp.posterior);
	} else {
	    lat.alignWords(hyp.words, hyp.posterior);
	}

	/*
	 * Ignore hyps whose cummulative posterior mass is below threshold
	 */
	totalPost += hyp.posterior;
	if (postPrune > 0.0 && totalPost > 1.0 - postPrune) {
	    break;
	}
    }

    /*
     * Remove posterior mass due to priming
     */
    if (primeWords) {
	lat.addWords(primeWords, - primePosterior);
	delete [] primeWords;
    }

    if (dumpPosteriors) {
	/*
	 * Dump hyp posteriors, followed by word posteriors
	 */
	for (unsigned i = 0; i < howmany; i ++) {
	    NBestHyp &hyp = nbestList.getHyp(i);

	    unsigned hypLength = Vocab::length(hyp.words);

	    Prob posteriors[hypLength];

	    lat.alignWords(hyp.words, 0.0, posteriors);

	    cout << hyp.posterior;
	    for (unsigned j = 0; j < hypLength; j ++) {
		cout << " " << posteriors[j];
	    }
	    cout << endl;
	}
    } else if (!dumpErrors) {
	/*
	 * Recover best hyp from lattice
	 */
	VocabIndex bestWords[maxWordsPerLine + 1];

	double subs, inss, dels;

	unsigned flags;
	if (noViterbi) {
	    flags |= WORDLATTICE_NOVITERBI;
	}
	 
	double errors =
		lat.minimizeWordError(bestWords, maxWordsPerLine,
				      subs, inss, dels, flags, deletionBias);

	cout << (lat.vocab.use(), bestWords) << endl;

	if (debug >= DEBUG_ERRORS) {
	    cerr << "err " << errors << " sub " << subs
		 << " ins " << inss << " del " << dels << endl;
	}

	if (debug >= DEBUG_POSTERIORS) {
	    unsigned numWords = Vocab::length(bestWords);
	    Prob posteriors[numWords];

	    lat.alignWords(bestWords, 0.0, posteriors);

	    cerr << "post";
	    for (unsigned j = 0; j < numWords; j ++) {
		cerr << " " << posteriors[j];
	    }
	    cerr << endl;
	}
    }
}

void
wordErrorRescore(NBestList &nbestList)
{
    unsigned numHyps = nbestList.numHyps();
    unsigned howmany = (maxRescore > 0) ? maxRescore : numHyps;
    if (howmany > numHyps) {
	howmany = numHyps;
    }

    if (!noReorder) {
	nbestList.reweightHyps(rescoreLMW, rescoreWTW);
	nbestList.sortHyps();
    }

    nbestList.computePosteriors(posteriorLMW, posteriorWTW, posteriorScale,
								posteriorAMW);

    if (dumpPosteriors) {
	/*
	 * Dump hyp posteriors
	 */
	for (unsigned i = 0; i < howmany; i ++) {
	    cout << nbestList.getHyp(i).posterior << endl;
	}
    } else if (!dumpErrors) {
	VocabIndex bestWords[maxWordsPerLine + 1];

	double subs, inss, dels;
	double errors = nbestList.minimizeWordError(bestWords, maxWordsPerLine,
				    subs, inss, dels, maxRescore, postPrune);

	cout << (nbestList.vocab.use(), bestWords) << endl;

	if (debug >= DEBUG_ERRORS) {
	    cerr << "err " << errors << " sub " << subs
		 << " ins " << inss << " dels " << dels << endl;
	}
    }
}

void
computeWordErrors(NBestList &nbestList, VocabIndex *reference)
{
    unsigned numHyps = nbestList.numHyps();
    unsigned howmany = (maxRescore > 0) ? maxRescore : numHyps;
    if (howmany > numHyps) {
	howmany = numHyps;
    }

    for (unsigned i = 0; i < howmany; i ++) {
	unsigned sub, ins, del;
	WordAlignType alignment[Vocab::length(nbestList.getHyp(i).words) +
				Vocab::length(reference) + 1];

	unsigned numErrors = wordError(reference, nbestList.getHyp(i).words,
						    sub, ins, del, alignment);

	cout << numErrors;
	for (unsigned j = 0; alignment[j] != END_ALIGN; j ++) {
	    cout << " " << ((alignment[j] == INS_ALIGN) ? "INS" :
	    			(alignment[j] == DEL_ALIGN) ? "DEL" :
				(alignment[j] == SUB_ALIGN) ? "SUB" : "CORR");
	}
	cout << endl;
    }
}

int
main (int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (primeWith1best) {
	primeLattice = 1;
    }

    Vocab vocab;
    NullLM nullLM(vocab);

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

    /*
     * Default weights for posterior computation are same as for rescoring
     */
    if (posteriorLMW == undefinedWeight) {
	posteriorLMW = rescoreLMW;
    }
    if (posteriorWTW == undefinedWeight) {
	posteriorWTW = rescoreWTW;
    }

    Vocab dictVocab;
    VocabMultiMap dictionary(vocab, dictVocab);
    DictionaryAbsDistance wordDistance(vocab, dictionary);

    MultiAlign *lat;
    
    if (useMesh) {
	if (dictFile) {
	    /* 
	     * read dictionary to be help in word alignment
	     */
	    File file(dictFile, "r");

	    if (!dictionary.read(file)) {
		cerr << "format error in dictionary file\n";
		exit(1);
	    }

	    lat = new WordMesh(vocab, &wordDistance);
	} else {
	    lat = new WordMesh(vocab);
	}
    } else {
	lat = new WordLattice(vocab);
    }
    assert(lat != 0);

    if (readFile) {
	File file(readFile, "r");

	if (!lat->read(file)) {
	    cerr << "format error in lattice file\n";
	    exit(1);
	}
    }

    /*
     * Read reference words
     */
    VocabIndex reference[maxWordsPerLine + 1];

    if (refString) {
	VocabString refWords[maxWordsPerLine + 1];
	unsigned numWords =
		    Vocab::parseWords(refString, refWords, maxWordsPerLine);
        if (numWords == maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " reference words\n";
	    exit(1);
	}

	vocab.addWords(refWords, reference, maxWordsPerLine + 1);
    } else {
	if (dumpErrors) {
	    cerr << "cannot compute word errors without reference\n";
	    exit(1);
	}
    }

    /*
     * Read single nbest list
     */
    NBestList nbestList(vocab, maxNbest, multiwords);
    nbestList.debugme(debug);

    if (rescoreFile) {
	File input(rescoreFile, "r");

	if (!nbestList.read(input)) {
	    cerr << "format error in nbest list\n";
	    exit(1);
	}

	/*
	 * Remove pauses and noise from nbest hyps since these would
	 * confuse the inter-hyp alignments.
	 */
	if (!keepNoise) {
	    nbestList.removeNoise(nullLM);
	}

	if (nbestErrorFile) {
	    /*
	     * Compute nbest error relative to reference list
	     */
	    NBestList refList(vocab);

	    File input(nbestErrorFile, "r");

	    if (!refList.read(input)) {
		cerr << "format error in reference nbest list\n";
		exit(1);
	    }

	    /*
	     * Remove pauses and noise from nbest hyps since these would
	     * confuse the inter-hyp alignments.
	     */
	    if (!keepNoise) {
		refList.removeNoise(nullLM);
	    }

	    for (unsigned h = 0; h < refList.numHyps(); h ++) {
		unsigned sub, ins, del;
		unsigned err =
		    nbestList.wordError(refList.getHyp(h).words, sub, ins, del);
		cout << err
		     << " sub " << sub 
		     << " ins " << ins
		     << " del " << del << endl;
	    }
	} else if (werRescore) {
	    /*
	     * Word error rescoring
	     */
	    wordErrorRescore(nbestList);
	} else {
	    /*
	     * Lattice building (and rescoring)
	     */
	    latticeRescore(*lat, nbestList);
	}

	if (dumpErrors) {
	    computeWordErrors(nbestList, reference);
	}

	if (writeNbestFile) {
	    File output(writeNbestFile, "w");

	    nbestList.write(output);
	}
    }
    
    /*
     * Compute word error of lattice relative to reference hyps
     */
    if (latticeErrorFile) {
	NBestList refList(vocab);

	File input(latticeErrorFile, "r");

	if (!refList.read(input)) {
	    cerr << "format error in reference nbest list\n";
	    exit(1);
	}

	/*
	 * Remove pauses and noise from nbest hyps since these would
	 * confuse the inter-hyp alignments.
	 */
	if (!keepNoise) {
	    refList.removeNoise(nullLM);
	}

	for (unsigned h = 0; h < refList.numHyps(); h ++) {
	    unsigned sub, ins, del;
	    unsigned err =
			lat->wordError(refList.getHyp(h).words, sub, ins, del);
	    cout << err
		 << " sub " << sub 
		 << " ins " << ins
		 << " del " << del << endl;
	}
    }

    /*
     * If reference words are known, record them in alignment
     */
    if (refString) {
	lat->alignReference(reference);
    }
    
    if (writeFile) {
	File file(writeFile, "w");

	lat->write(file);
    }

    /*
     * Read list of nbest filenames
     */
    if (nbestFiles) {
	File file(nbestFiles, "r");

	char *line;
	while (line = file.getline()) {
	    char *fname = strtok(line, " \t\n");
	    if (fname) {
		MultiAlign *lat;

		if (useMesh) {
		    lat = new WordMesh(vocab);
		} else {
		    lat = new WordLattice(vocab);
		}
		assert(lat != 0);

		NBestList nbestList(vocab, maxNbest, multiwords);
		nbestList.debugme(debug);

		File input(fname, "r");

		if (!nbestList.read(input)) {
		    cerr << "format error in nbest list\n";
		    exit(1);
		}

		/*
		 * Remove pauses and noise from nbest hyps since these would
		 * confuse the inter-hyp alignments.
		 */
		if (!keepNoise) {
		    nbestList.removeNoise(nullLM);
		}

		if (werRescore) {
		    wordErrorRescore(nbestList);
		} else {
		    latticeRescore(*lat, nbestList);
		}

		if (dumpErrors) {
		    computeWordErrors(nbestList, reference);
		}

		delete lat;
	    }
	}
    }

    delete lat;
    exit(0);
}
