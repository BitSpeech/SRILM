/*
 * nbest-lattice --
 *	Build and rerank N-Best lattices and confusion networks
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2001 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/nbest-lattice.cc,v 1.61 2001/10/31 07:00:31 stolcke Exp $";
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
#include "RefList.h"
#include "zio.h"

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
static int toLower = 0;
static int multiwords = 0;
static char *readFile = 0;
static char *writeFile = 0;
static char *writeDir = 0;
static char *rescoreFile = 0;
static int computeNbestError = 0;
static int computeLatticeError = 0;
static char *nbestFiles = 0;
static char *writeNbestFile = 0;
static int writeDecipherNbest = 0;
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
static char *refFile = 0;
static int dumpErrors = 0;
static int recordHypIDs = 0;
static int nbestBacktrace = 0;
static int noRescore = 0;

static Option options[] = {
    { OPT_UINT, "debug", &debug, "debugging level" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_TRUE, "multiwords", &multiwords, "split multiwords in N-best hyps" },
    { OPT_TRUE, "wer", &werRescore, "optimize expected WER using N-best list" },
    { OPT_FALSE, "lattice-wer", &werRescore, "optimize expected WER using lattice" },
    { OPT_STRING, "read", &readFile, "lattice file to read" },
    { OPT_STRING, "write", &writeFile, "lattice file to write" },
    { OPT_STRING, "write-dir", &writeDir, "lattice directory to write to" },

    { OPT_STRING, "rescore", &rescoreFile, "hyp stream input file to rescore" },
    { OPT_TRUE, "nbest-error", &computeNbestError, "compute n-best error" },
    { OPT_TRUE, "lattice-error", &computeLatticeError, "compute lattice error" },
    { OPT_STRING, "nbest", &rescoreFile, "same as -rescore" },
    { OPT_STRING, "write-nbest", &writeNbestFile, "output n-best list" },
    { OPT_TRUE, "decipher-nbest", &writeDecipherNbest, "output Decipher n-best format" },
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
    { OPT_TRUE, "nbest-backtrace", &nbestBacktrace, "read backtrace info from N-best lists" },
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
    { OPT_TRUE, "record-hyps", &recordHypIDs, "record hyp IDs in lattice" },
    { OPT_TRUE, "no-rescore", &noRescore, "suppress lattice rescoring" },
    { OPT_STRING, "reference", &refString, "reference words" },
    { OPT_STRING, "refs", &refFile, "reference transcript file" }
};

void
latticeRescore(const char *sentid, MultiAlign &lat, NBestList &nbestList)
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
	HypID hypID = i;
	HypID *hypIDPtr = recordHypIDs ? &hypID : 0;

	/*
	 * Check for overflow in the hypIDs
	 */
	if (recordHypIDs && ((unsigned)hypID != i || hypID == refID)) {
	    cerr << "Sorry, too many hypotheses in N-best list "
		 << (sentid ? sentid : "") << endl;
	    exit(2);
	}

	totalWords += Vocab::length(hyp.words);

	/*
	 * If merging is turned off or the lattice is empty (only
	 * initial/final nodes) we add fresh path to it.
	 * Otherwise merge using string alignment.
	 */
	
	if (noMerge || lat.isEmpty()) {
	    if (hyp.wordInfo) {
		lat.addWords(hyp.wordInfo, hyp.posterior, hypIDPtr);
	    } else {
		lat.addWords(hyp.words, hyp.posterior, hypIDPtr);
	    }
	} else {
	    if (hyp.wordInfo) {
		lat.alignWords(hyp.wordInfo, hyp.posterior, 0, hypIDPtr);
	    } else {
		lat.alignWords(hyp.words, hyp.posterior, 0, hypIDPtr);
	    }
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

	    if (sentid) cout << sentid << ":" << i << " ";
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

	if (sentid) cout << sentid << " ";
	cout << (lat.vocab.use(), bestWords) << endl;

	if (debug >= DEBUG_ERRORS) {
	    if (sentid) cerr << sentid << " ";
	    cerr << "err " << errors << " sub " << subs
		 << " ins " << inss << " del " << dels << endl;
	}

	if (debug >= DEBUG_POSTERIORS) {
	    unsigned numWords = Vocab::length(bestWords);
	    Prob posteriors[numWords];

	    lat.alignWords(bestWords, 0.0, posteriors);

	    if (sentid) cerr << sentid << " ";
	    cerr << "post";
	    for (unsigned j = 0; j < numWords; j ++) {
		cerr << " " << posteriors[j];
	    }
	    cerr << endl;
	}
    }
}

void
wordErrorRescore(const char *sentid, NBestList &nbestList)
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
	    if (sentid) cout << sentid << ":" << i << " ";
	    cout << nbestList.getHyp(i).posterior << endl;
	}
    } else if (!dumpErrors) {
	VocabIndex bestWords[maxWordsPerLine + 1];

	double subs, inss, dels;
	double errors = nbestList.minimizeWordError(bestWords, maxWordsPerLine,
				    subs, inss, dels, maxRescore, postPrune);

	if (sentid) cout << sentid << " ";
	cout << (nbestList.vocab.use(), bestWords) << endl;

	if (debug >= DEBUG_ERRORS) {
	    if (sentid) cerr << sentid << " ";
	    cerr << "err " << errors
		 << " sub " << subs
		 << " ins " << inss
		 << " del " << dels << endl;
	}
    }
}

void
computeWordErrors(const char *sentid, NBestList &nbestList,
						const VocabIndex *reference)
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

	if (sentid) cout << sentid << ":" << i << " ";
	cout << numErrors;
	for (unsigned j = 0; alignment[j] != END_ALIGN; j ++) {
	    cout << " " << ((alignment[j] == INS_ALIGN) ? "INS" :
	    			(alignment[j] == DEL_ALIGN) ? "DEL" :
				(alignment[j] == SUB_ALIGN) ? "SUB" : "CORR");
	}
	cout << endl;
    }
}

/*
 * Process a single N-best list
 */
void
processNbest(NullLM &nullLM, const char *sentid, const char *nbestFile,
			VocabMultiMap &dictionary,
			const VocabIndex *reference, const char *outLattice)
{
    Vocab &vocab = nullLM.vocab;
    MultiAlign *lat;
    DictionaryAbsDistance wordDistance(vocab, dictionary);
    
    if (useMesh) {
	if (dictFile) {
	    lat = new WordMesh(vocab, &wordDistance);
	} else {
	    lat = new WordMesh(vocab);
	}
    } else {
	lat = new WordLattice(vocab);
    }
    assert(lat != 0);

    /*
     * Read preexisting lattice if specified
     */
    if (readFile) {
	File file(readFile, "r");

	if (!lat->read(file)) {
	    cerr << "format error in lattice file\n";
	    exit(1);
	}
    }

    /*
     * Process nbest list
     */
    if (nbestFile) {
	NBestList nbestList(vocab, maxNbest, multiwords, nbestBacktrace);
	nbestList.debugme(debug);

	{
	    File input(nbestFile, "r");

	    if (!nbestList.read(input)) {
		cerr << "format error in nbest list\n";
		exit(1);
	    }
	}

	/*
	 * Remove pauses and noise from nbest hyps since these would
	 * confuse the inter-hyp alignments.
	 */
	if (!keepNoise) {
	    nbestList.removeNoise(nullLM);
	}

	/*
	 * Compute nbest error relative to reference
	 */
	if (computeNbestError) {
	    unsigned sub, ins, del;

	    unsigned err = nbestList.wordError(reference, sub, ins, del);
	    if (sentid) cout << sentid << " ";
	    cout << err
		 << " sub " << sub 
		 << " ins " << ins
		 << " del " << del
		 << " words " << Vocab::length(reference) << endl;
	} else if (werRescore) {
	    /*
	     * Word error rescoring
	     */
	    wordErrorRescore(sentid, nbestList);
	} else if (!noRescore) {
	    /*
	     * Lattice building (and rescoring)
	     */
	    latticeRescore(sentid, *lat, nbestList);
	}

	if (dumpErrors) {
	    computeWordErrors(sentid, nbestList, reference);
	}

	if (writeNbestFile) {
	    File output(writeNbestFile, "w");

	    nbestList.write(output, writeDecipherNbest);
	}
    }
    
    /*
     * Compute word error of lattice relative to reference hyps
     */
    if (computeLatticeError) {
	unsigned sub, ins, del;
	unsigned err = lat->wordError(reference, sub, ins, del);

	if (sentid) cout << sentid << " ";
	cout << err
	     << " sub " << sub 
	     << " ins " << ins
	     << " del " << del
	     << " words " << Vocab::length(reference) << endl;
    }

    /*
     * If reference words are known, record them in alignment
     */
    if (reference && !computeNbestError) {
	lat->alignReference(reference);
    }
    
    if (outLattice) {
	File file(outLattice, "w");

	lat->write(file);
    }

    delete lat;
}

/*
 * Locate utterance id in filename
 */
char *
sentidFromFilename(const char *filename)
{
    const char *id = strrchr(filename, '/');
    if (id) {
	id += 1;
    } else {
	id = filename;
    }

    char *suffix = strchr(id, '.');
    if (suffix) *suffix = '\0';

    char *result = strdup(id);
    assert(result != 0);

    if (suffix) *suffix = '.';

    return result;
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

    /* 
     * Read optional dictionary to help in word alignment
     */
    if (dictFile) {
	File file(dictFile, "r");

	if (!dictionary.read(file)) {
	    cerr << "format error in dictionary file\n";
	    exit(1);
	}
    }

    /*
     * Read reference words
     */
    VocabIndex *reference = 0;

    if (refString) {
	reference = new VocabIndex[maxWordsPerLine + 1];
	assert(reference != 0);

	VocabString refWords[maxWordsPerLine + 1];
	unsigned numWords =
		    Vocab::parseWords(refString, refWords, maxWordsPerLine);
        if (numWords == maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " reference words\n";
	    exit(1);
	}

	vocab.addWords(refWords, reference, maxWordsPerLine + 1);
    } else if (rescoreFile) {
	if (dumpErrors || computeNbestError || computeLatticeError) {
	    cerr << "cannot compute errors without reference\n";
	    exit(1);
	}
    }

    /*
     * Process single nbest file
     */
    if (rescoreFile) {
	processNbest(nullLM, 0, rescoreFile, dictionary, reference, writeFile);
    } else if (!nbestFiles) {
	/*
	 * If neither -nbest nor -nbest-files was specified
	 * do lattice processing only.
	 */
	processNbest(nullLM, 0, 0, dictionary, reference, writeFile);
    }

    /*
     * Read list of nbest filenames
     */
    if (nbestFiles) {
	RefList refs(vocab);

	if (refFile) {
	    File file(refFile, "r");
	    refs.read(file, true);	 // add reference words to vocabulary
	} else {
	    if (dumpErrors || computeNbestError || computeLatticeError) {
		cerr << "cannot compute errors without reference\n";
		exit(1);
	    }
	}
		

	File file(nbestFiles, "r");
	char *line;
	while (line = file.getline()) {
	    char *fname = strtok(line, " \t\n");
	    if (!fname) continue;

	    char *sentid = sentidFromFilename(fname);

	    VocabIndex *reference = 0;

	    if (refFile) {
		reference = refs.findRef(sentid);
		if (!reference) {
		    cerr << "no reference for " << sentid << endl;
		    if (dumpErrors || computeNbestError || computeLatticeError)
		    {
			free(sentid);
			continue;
		    }
		}
	    }

	    if (writeDir) {
		char writeName[strlen(writeDir) + 1 + strlen(sentid) 
						+ strlen(GZIP_SUFFIX) + 1];
		sprintf(writeName, "%s/%s%s", writeDir, sentid, GZIP_SUFFIX);

		processNbest(nullLM, sentid, fname, dictionary, reference,
								    writeName);
	    } else {
		processNbest(nullLM, sentid, fname, dictionary, reference, 0);
	    }

	    free(sentid);
	}
    }

    exit(0);
}
