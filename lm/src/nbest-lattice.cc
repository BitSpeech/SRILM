/*
 * nbest-lattice --
 *	Build and rerank N-Best lattices
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/nbest-lattice.cc,v 1.37 1999/08/01 09:35:25 stolcke Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include "option.h"
#include "File.h"
#include "Prob.h"
#include "Vocab.h"
#include "NBest.h"
#include "NullLM.h"
#include "WordLattice.h"
#include "WordMesh.h"

#define DEBUG_ERRORS	1

/*
 * Pseudo-posterior used to prime lattice with centroid hyp
 */
const Prob primePosterior = 100.0;

static int debug = 0;
static int werRescore = 0;
static int maxRescore = 0;
static char *vocabFile = 0;
static int tolower = 0;
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
static char *noiseTag = 0;
static int noMerge = 0;
static int noReorder = 0;
static double postPrune = 0.0;
static int primeLattice = 0;
static int noViterbi = 0;
static int useMesh = 0;
static int dumpPosteriors = 0;

static Option options[] = {
    { OPT_INT, "debug", &debug, "debugging level" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_TRUE, "tolower", &tolower, "map vocabulary to lowercase" },
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
    { OPT_INT, "max-nbest", &maxNbest, "maximum number of hyps to consider" },
    { OPT_INT, "max-rescore", &maxRescore, "maximum number of hyps to rescore" },
    { OPT_FLOAT, "posterior-prune", &postPrune, "ignore n-best hyps whose cumulative posterior mass is below threshold" },
    { OPT_FLOAT, "rescore-lmw", &rescoreLMW, "rescoring LM weight" },
    { OPT_FLOAT, "rescore-wtw", &rescoreWTW, "rescoring word transition weight" },
    { OPT_FLOAT, "posterior-scale", &posteriorScale, "divisor for log posterior estimates" },
    { OPT_STRING, "noise", &noiseTag, "noise tag to skip" },
    { OPT_TRUE, "no-merge", &noMerge, "don't merge hyps for lattice building" },
    { OPT_TRUE, "no-reorder", &noReorder, "don't reorder N-best hyps before rescoring" },
    { OPT_TRUE, "prime-lattice", &primeLattice, "initialize word lattice with WE-minimized hyp" },
    { OPT_TRUE, "no-viterbi", &noViterbi, "minimize lattice WE without Viterbi search" },
    { OPT_TRUE, "use-mesh", &useMesh, "align using word mesh (not lattice)" },
    { OPT_TRUE, "dump-posteriors", &dumpPosteriors, "output hyp and word posteriors probs" }

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

    nbestList.computePosteriors(rescoreLMW, rescoreWTW, posteriorScale);

    unsigned howmany = (maxRescore > 0) ? maxRescore : numHyps;
    if (howmany > numHyps) {
	howmany = numHyps;
    }

    Prob totalPost = 0.0;
    VocabIndex *bestWords = 0;

    /* 
     * Prime lattice with the minimal WE hyp from the nbest list
     */
    if (primeLattice && !noMerge && lat.isEmpty()) {
	bestWords = new VocabIndex[maxWordsPerLine + 1];
	assert(bestWords != 0);

	double subs, inss, dels;
	(void)nbestList.minimizeWordError(bestWords, maxWordsPerLine,
				    subs, inss, dels, maxRescore, postPrune);

	lat.addWords(bestWords, primePosterior);
    }

    /*
     * Incorporate hyps into lattice
     */
    for (unsigned i = 0; i < howmany; i ++) {
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
    if (bestWords) {
	lat.addWords(bestWords, - primePosterior);
	delete [] bestWords;
    }

    if (dumpPosteriors) {
	/*
	 * Dump hyp posteriors, followed by word posteriors
	 */
	for (unsigned i = 0; i < howmany; i ++) {
	    NBestHyp &hyp = nbestList.getHyp(i);

	    unsigned hypLength = Vocab::length(hyp.words);

	    Prob *posteriors = new Prob[hypLength];
	    assert(posteriors != 0);

	    lat.alignWords(hyp.words, 0.0, posteriors);

	    cout << hyp.posterior;
	    for (unsigned j = 0; j < hypLength; j ++) {
		cout << " " << posteriors[j];
	    }
	    cout << endl;

	    delete [] posteriors;
	}
    } else {
	/*
	 * Recover best hyp from lattice
	 */
	bestWords = new VocabIndex[maxWordsPerLine + 1];
	assert(bestWords != 0);

	double subs, inss, dels;

	unsigned flags;
	if (noViterbi) {
	    flags |= WORDLATTICE_NOVITERBI;
	}
	 
	double errors = lat.minimizeWordError(bestWords, maxWordsPerLine,
						    subs, inss, dels, flags);

	cout << (lat.vocab.use(), bestWords) << endl;

	if (debug >= DEBUG_ERRORS) {
	    cerr << "err " << errors << " sub " << subs
		 << " ins " << inss << " del " << dels << endl;
	}

	delete [] bestWords;
    }
}

void
wordErrorRescore(NBestList &nbestList)
{
    if (!noReorder) {
	nbestList.reweightHyps(rescoreLMW, rescoreWTW);
	nbestList.sortHyps();
    }

    nbestList.computePosteriors(rescoreLMW, rescoreWTW, posteriorScale);

    if (dumpPosteriors) {
	/*
	 * Dump hyp posteriors
	 */
	for (unsigned i = 0; i < nbestList.numHyps(); i ++) {
	    cout << nbestList.getHyp(i).posterior << endl;
	}
    } else {
	VocabIndex *bestWords = new VocabIndex[maxWordsPerLine + 1];
	assert(bestWords != 0);

	double subs, inss, dels;
	double errors = nbestList.minimizeWordError(bestWords, maxWordsPerLine,
				    subs, inss, dels, maxRescore, postPrune);

	cout << (nbestList.vocab.use(), bestWords) << endl;

	if (debug >= DEBUG_ERRORS) {
	    cerr << "err " << errors << " sub " << subs
		 << " ins " << inss << " dels " << dels << endl;
	}

	delete [] bestWords;
    }
}

int
main (int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

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
    if (noiseTag) {
	nullLM.noiseIndex = vocab.addWord(noiseTag);
    }

    /*
     * Posterior scaling:  if not specified (= 0.0) use LMW for
     * backward compatibility.
     */
    if (posteriorScale == 0.0) {
	posteriorScale = (rescoreLMW == 0.0) ? 1.0 : rescoreLMW;
    }

    MultiAlign *lat;
    
    if (useMesh) {
	lat = new WordMesh(vocab);
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
     * Read single nbest list
     */
    NBestList nbestList(vocab, maxNbest);
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
	nbestList.removeNoise(nullLM);

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
	    refList.removeNoise(nullLM);

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
	refList.removeNoise(nullLM);

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

		NBestList nbestList(vocab, maxNbest);
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
		nbestList.removeNoise(nullLM);

		if (werRescore) {
		    wordErrorRescore(nbestList);
		} else {
		    latticeRescore(*lat, nbestList);
		}

		delete lat;
	    }
	}
    }

    delete lat;
    exit(0);
}
