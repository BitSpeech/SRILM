/*
 * lattice-tool --
 *	Manipulate word lattices
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1997-2003 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lattice/src/RCS/lattice-tool.cc,v 1.54 2003/02/19 06:00:36 stolcke Exp $";
#endif

#include <stdio.h>
#include <math.h>
#include <locale.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#include "option.h"
#include "MultiwordVocab.h"
#include "Lattice.h"
#include "MultiwordLM.h"
#include "Ngram.h"
#include "ClassNgram.h"
#include "SimpleClassNgram.h"
#include "BayesMix.h"
#include "RefList.h"

#define DebugPrintFunctionality 1	// same as in Lattice.cc

static int compactTrigram = 0; 
static int oldTrigram = 0; 
static int base = 0; 
static int connectivity = 0; 
static int compactPause = 0; 
static int nodeEntropy = 0; 
static int computePosteriors = 0;
static char *writePosteriors = 0;
static double posteriorScale = 8.0;
static double posteriorPruneThreshold = 0.0;
static int reduceBeforePruning = 0;
static int noPause = 0; 
static int loopPause = 0;
static int overwrite = 0; 
static int simpleReduction = 0; 
static int overlapBase = 0;
static double overlapRatio = 0.0;
static int dag = 0; 
static char *lmFile  = 0;
static char *vocabFile = 0;
static int limitVocab = 0;
static char *classesFile = 0;
static int simpleClasses = 0;
static char *mixFile  = 0;
static char *mixFile2 = 0;
static char *mixFile3 = 0;
static char *mixFile4 = 0;
static char *mixFile5 = 0;
static char *mixFile6 = 0;
static char *mixFile7 = 0;
static char *mixFile8 = 0;
static char *mixFile9 = 0;
static double mixLambda = 0.5;
static double mixLambda2 = 0.0;
static double mixLambda3 = 0.0;
static double mixLambda4 = 0.0;
static double mixLambda5 = 0.0;
static double mixLambda6 = 0.0;
static double mixLambda7 = 0.0;
static double mixLambda8 = 0.0;
static double mixLambda9 = 0.0;
static char *inLattice = 0; 
static char *inLattice2 = 0; 
static char *inLatticeList = 0; 
static char *outLattice = 0; 
static char *outLatticeDir = 0;
static char *refFile = 0; 
static char *refList = 0; 
static char *noiseVocabFile = 0;
static char *indexName = 0; 
static char *operation = 0; 
static int order = 3;
static int simpleReductionIter = 0; 
static int preReductionIter = 0; 
static int postReductionIter = 0; 
static int collapseSameWords = 0;
static int debug = 0;
static int writeInternal = 0;
static int maxTime = 0;
static int maxNodes = 0;
static int splitMultiwords = 0;
static int toLower = 0;
static int useMultiwordLM = 0;

static Option options[] = {
    { OPT_TRUE, "in-lattice-dag", &dag, "input lattices are defined in a directed acyclic graph" },
    { OPT_STRING, "in-lattice", &inLattice, "input lattice for lattice operation including expansion or bigram weight substitution" },
    { OPT_STRING, "in-lattice2", &inLattice2, "a second input lattice for lattice operation" },
    { OPT_STRING, "in-lattice-list", &inLatticeList, "input lattice list for expansion or bigram weight substitution" },
    { OPT_STRING, "out-lattice", &outLattice, "resulting output lattice" },
    { OPT_STRING, "out-lattice-dir", &outLatticeDir, "resulting output lattice dir" },
    { OPT_STRING, "lm", &lmFile, "LM used for expansion or weight substitution" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_TRUE, "limit-vocab", &limitVocab, "limit LM reading to specified vocabulary" },
    { OPT_STRING, "classes", &classesFile, "class definitions" },
    { OPT_TRUE, "simple-classes", &simpleClasses, "use unique class model" },
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
    { OPT_INT, "order", &order, "ngram order used for expansion or bigram weight substitution" },
    { OPT_STRING, "ref-list", &refList, "reference file used for computing WER (lines starting with utterance id)" }, 
    { OPT_STRING, "ref-file", &refFile, "reference file used for computing WER (utterances in same order in lattice list)" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary for WER computation" },
    { OPT_TRUE, "overwrite", &overwrite, "overwrite existing output lattice dir" }, 
    { OPT_TRUE, "reduce", &simpleReduction, "reduce bigram lattice(s) using the simple algorithm" },
    { OPT_INT, "reduce-iterate", &simpleReductionIter, "reduce input lattices iteratively" },
    { OPT_INT, "pre-reduce-iterate", &preReductionIter, "reduce pause-less lattices iteratively" },
    { OPT_INT, "post-reduce-iterate", &postReductionIter, "reduce output lattices iteratively" },
    { OPT_TRUE, "reduce-before-pruning", &reduceBeforePruning, "apply posterior pruning after lattice reduction" },
    { OPT_FLOAT, "overlap-ratio", &overlapRatio, "if two incoming/outgoing node sets of two given nodes with the same lable overlap beyong this ratio, they are merged" }, 
    { OPT_INT, "overlap-base", &overlapBase, "use the smaller (0) incoming/outgoing node set to compute overlap ratio, or the larger (1) set to compute the overlap ratio" },
    { OPT_TRUE, "compact-expansion", &compactTrigram, "use compact trigram expansion algorithm (sharing bigram nodes)" },
    { OPT_TRUE, "topo-compact-expansion", &compactTrigram, "(same as above, for backward compatibility)" },
    { OPT_TRUE, "old-expansion", &oldTrigram, "use old trigram expansion algorithm (without bigram sharing)" },
    { OPT_TRUE, "multiwords", &useMultiwordLM, "use multiword wrapper LM" },
    { OPT_TRUE, "split-multiwords", &splitMultiwords, "split multiwords into separate nodes" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lower case" },
    { OPT_STRING, "operation", &operation, "conventional lattice operations, including \"concatenate\" and \"or\"" }, 
    { OPT_TRUE, "connectivity", &connectivity, "check the connectivity of given lattices" }, 
    { OPT_TRUE, "compute-node-entropy", &nodeEntropy, "compute the node entropy of given lattices" }, 
    { OPT_TRUE, "compute-posteriors", &computePosteriors, "compute the node posteriors of given lattices" }, 
    { OPT_STRING, "write-posteriors", &writePosteriors, "write posterior lattice format to this file" }, 
    { OPT_FLOAT, "posterior-prune", &posteriorPruneThreshold, "posterior node pruning threshold" }, 
    { OPT_FLOAT, "posterior-scale", &posteriorScale, "posterior scaling factor" }, 
    { OPT_STRING, "index-name", &indexName, "print a list of node index-name pairs to this file" },
    { OPT_TRUE, "no-pause", &noPause, "output lattices with no pauses" },
    { OPT_TRUE, "compact-pause", &compactPause, "output lattices with compact pauses" },
    { OPT_TRUE, "loop-pause", &loopPause, "output lattices with loop pauses" },
    { OPT_TRUE, "collapse-same-words", &collapseSameWords, "collapse nodes with same words" },
    { OPT_INT, "debug", &debug, "debug level" },
    { OPT_TRUE, "write-internal", &writeInternal, "write out internal node numbering" },
    { OPT_UINT, "max-time", &maxTime, "maximum no. of seconds allowed per lattice" },
    { OPT_UINT, "max-nodes", &maxNodes, "maximum no. of nodes allowed in expanded lattice (without pauses)" },
};

void splitMultiwordNodes(Lattice &lat, MultiwordVocab &vocab, LM &lm)
{
    VocabIndex emptyContext[1];
    emptyContext[0] = Vocab_None;

    if (debug >= DebugPrintFunctionality) {
      cerr << "splitting multiword nodes\n";
    }

    unsigned numNodes = lat.getNumNodes(); 

    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = lat.sortNodes(sortedNodes);

    for (unsigned i = 0; i < numReachable; i++) {
      NodeIndex nodeIndex = sortedNodes[i];
      LatticeNode *node = lat.findNode(nodeIndex); 

      VocabIndex oneWord[2];
      oneWord[0] = node->word;
      oneWord[1] = Vocab_None;
      VocabIndex expanded[maxWordsPerLine + 1];

      unsigned expandedLength =
		vocab.expandMultiwords(oneWord, expanded, maxWordsPerLine);

      /*
       * We don't split multiwords that are in the LM
       */
      if (expandedLength > 1 &&
	  lm.wordProb(node->word, emptyContext) == LogP_Zero)
      {
	// change orignal node to emit first component word
	node->word = expanded[0];

	NodeIndex prevNodeIndex = nodeIndex;
	NodeIndex firstNewIndex;
	
	// create new nodes for all subsequent word components, and
	// string them together with zero weight transitions
	for (unsigned i = 1; i < expandedLength; i ++) {
	    NodeIndex newNodeIndex = lat.dupNode(expanded[i]);

	    // delay inserting the first new transition to not interfere
	    // with removal of old links below
	    if (prevNodeIndex == nodeIndex) {
		firstNewIndex = newNodeIndex;
	    } else {
	        LatticeTransition trans;
	        lat.insertTrans(prevNodeIndex, newNodeIndex, trans);
	    }
	    prevNodeIndex = newNodeIndex;
	}

	// node may have moved since others were added!!!
        node = lat.findNode(nodeIndex); 

	// copy original outgoing transitions onto final new node
        TRANSITER_T<NodeIndex,LatticeTransition>
				transIter(node->outTransitions);
	NodeIndex toNodeIndex;
        while (LatticeTransition *trans = transIter.next(toNodeIndex)) {
	    // prevNodeIndex still has the last of the newly created nodes
	    lat.insertTrans(prevNodeIndex, toNodeIndex, *trans);
	    lat.removeTrans(nodeIndex, toNodeIndex);
	}

	// now insert new transition out of original node
	{
	    LatticeTransition trans;
	    lat.insertTrans(nodeIndex, firstNewIndex, trans);
	}
      }
    }
}

void latticeOps(char *inLat1, char *inLat2, char *outLat)
{
    Vocab vocab;
    vocab.toLower = true; 

    if (!operation) { 
      cerr << "Fatal Error (latticeOps): no operation is specified!\n"
	   << "          Allowable operations: and, or, ...\n";
      exit (-1);
    }

    Lattice lat(vocab, outLat ? outLat : "NONAME"); 
    lat.debugme(debug);
    {
      File file1(inLat1, "r");
      Lattice lat1(vocab); 
      lat1.readPFSGs(file1);

      File file2(inLat2, "r");
      Lattice lat2(vocab); 
      lat2.readPFSGs(file2);

      if (!strcmp(operation, LATTICE_OR)) {
	lat.latticeOr(&lat1, &lat2);
      } else if (!strcmp(operation, LATTICE_CONCATE)) {
	lat.latticeCat(&lat1, &lat2);
      } else {
	cerr << "latticeOps: unknown operation (" << operation
	     << ")\n     Allowable operations are and, or, ...\n";
	exit (-1); 
      }
    }

    if (outLattice) {
      File file(outLat, "w");
      if (writeInternal) {
        lat.writePFSG(file);
      } else {
        lat.writeCompactPFSG(file);
      }
    }
}

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

void processLattice(char *inLat, char *outLat, LM &lm, MultiwordVocab &vocab, 
		    SubVocab &noiseWords, VocabIndex *refIndices  = 0)
{
    Lattice lat(vocab, outLat ? outLat : "NONAME"); 
    lat.debugme(debug);
    {

      // the new proc is not done yet. Always using the original readPFSG
      dag = 0;
      File file(inLat, "r");
      if (!dag) {
	// cerr << " Using old proc\n";
	lat.readPFSGs(file);
      } else {
	// cerr << " Using new proc\n";
	lat.readRecPFSGs(file);
      }
    }

    if (maxTime) {
	alarm(maxTime);
	if (setjmp(thisContext)) {
	    cerr << "WARNING: processing lattice " << inLat
	         << " aborted after " << maxTime << " seconds\n";
	    return;
	}
	signal(SIGALRM, (sighandler_t)catchAlarm);
    }

    if (splitMultiwords) {
	splitMultiwordNodes(lat, vocab, lm);
    }

    if (posteriorPruneThreshold > 0 && !reduceBeforePruning) {
	if (!lat.prunePosteriors(posteriorPruneThreshold, posteriorScale)) {
	    cerr << "WARNING: posterior pruning of lattice " << inLat
	         << " failed\n";
	    alarm(0);
	    return;
        } 
    }

    if (writePosteriors) {
      File file(writePosteriors, "w");

      lat.writePosteriors(file, posteriorScale);
    } else if (computePosteriors) {
      lat.computePosteriors(posteriorScale);
    }

    if (connectivity) {
      if (!lat.areConnected(lat.getInitial(), lat.getFinal())) {
	cerr << "WARNING: lattice " << inLat << " is not connected\n";
	alarm(0);
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
	    cerr << "FATAL ERROR: reference is missing for lattice "
		 << inLat << endl;
	  }
    }

    if (simpleReduction || simpleReductionIter) {
      cerr << "reducing input lattices (overlap ratio = "
	   << overlapRatio << ")\n"; 

      if (overlapRatio == 0.0) { 
        // if reduceBeforePruning is specified then merged transitions probs
	// should be addded, not maxed
	lat.simplePackBigramLattice(simpleReductionIter, reduceBeforePruning); 
      } else {
	lat.approxRedBigramLattice(simpleReductionIter, overlapBase, 
				   overlapRatio);
      }
    }

    if (posteriorPruneThreshold > 0 && reduceBeforePruning) {
	if (!lat.prunePosteriors(posteriorPruneThreshold, posteriorScale)) {
	    cerr << "WARNING: posterior pruning of lattice " << inLat
	         << " failed\n";
	    alarm(0);
	    return;
        } 
    }

    if (maxNodes > 0 && lat.getNumNodes() > maxNodes) {
      cerr << "WARNING: processing lattice " << inLat
	   << " aborted -- too many nodes after reduction: "
	   << lat.getNumNodes() << endl;
      alarm(0);
      return;
    }

    if (lmFile) {
      // remove pause and NULL nodes prior to LM application,
      // so each word has a proper predecessor
      // (This can be skipped for unigram LMs, unless we're explicitly
      // asked to elininate pauses.)
      if (order > 1 || noPause) {
	lat.removeAllXNodes(vocab.pauseIndex);
	lat.removeAllXNodes(Vocab_None);
      
	/*
	 * attempt further reduction on pause-less lattices
	 */
	if (preReductionIter) {
	  cerr << "reducing pause-less lattices (overlap ratio = "
	       << overlapRatio << ")\n"; 

	  File f(stderr);
	  if (overlapRatio == 0.0) { 
	    lat.simplePackBigramLattice(simpleReductionIter); 
	  } else {
	    lat.approxRedBigramLattice(simpleReductionIter, overlapBase, 
				       overlapRatio);
	  }
	}
      }

      Boolean status;

      switch (order) {
      case 1:
      case 2: 
	status = lat.replaceWeights(lm); 
	break;
      default:
	// trigram expansion
	if (compactTrigram) {
	  status = lat.expandToCompactTrigram(*(Ngram *)&lm, maxNodes); 
	} else if (oldTrigram) {
	  status = lat.expandToTrigram(lm, maxNodes); 
	} else {
	  // general LM expansion
	  status = lat.expandToLM(lm, maxNodes); 
	}
	break;
      }

      if (!status) {
        cerr << "WARNING: expansion of lattice " << inLat << " failed\n";
	alarm(0);
	return;
      }

      if (!noPause && order > 1) {
	if (compactPause) {
  	  lat.recoverCompactPauses(loopPause);
	} else {
	  lat.recoverPauses(loopPause);
	}
      }

      /*
       * attempt further reduction on output lattices
       */
      if (postReductionIter) {
        cerr << "reducing output lattices (overlap ratio = "
	     << overlapRatio << ")\n"; 

	if (overlapRatio == 0.0) { 
	  lat.simplePackBigramLattice(simpleReductionIter); 
	} else {
	  lat.approxRedBigramLattice(simpleReductionIter, overlapBase, 
				     overlapRatio);
	}
      }
    }

    if (collapseSameWords) {
	lat.collapseSameWordNodes(noiseWords);
    }

    // kill alarm timer -- we're done
    alarm(0);

    if (outLattice || outLatticeDir) {
      File file(outLat, "w");
      if (writeInternal) {
        lat.writePFSG(file);
      } else {
        lat.writeCompactPFSG(file);
      }
    }
}

LM *
makeMixLM(const char *filename, Vocab &vocab, SubVocab *classVocab,
			    unsigned order, LM *oldLM, double lambda)
{
    File file(filename, "r");

    /*
     * create class-ngram if -classes were specified, otherwise a regular ngram
     */
    Ngram *lm = (classVocab != 0) ?
		  (simpleClasses ?
			new SimpleClassNgram(vocab, *classVocab, order) :
		  	new ClassNgram(vocab, *classVocab, order)) :
		  new Ngram(vocab, order);
    assert(lm != 0);

    lm->debugme(debug);

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

    if (oldLM) {
	LM *newLM = new BayesMix(vocab, *lm, *oldLM, 0, lambda);
	assert(newLM != 0);

	newLM->debugme(debug);

	return newLM;
    } else {
	return lm;
    }
}

int 
main (int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");
    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (!inLattice && !inLatticeList) {
        cerr << "need to specify at least one input file!\n";
	return 0; 
    }

    if ((classesFile || mixFile || useMultiwordLM) && compactTrigram) {
      cerr << "cannot use class-ngram LM, mixture LM, or multiword LM wrapper for compact lattice expansion\n";
      exit(2);
    }

    /*
     * Use multiword vocab in case we need it for -multiwords processing
     */
    MultiwordVocab vocab;
    vocab.toLower = toLower ? true : false;

    /*
     * Read predefined vocabulary (required by -limit-vocab)
     */
    if (vocabFile) {
	File file(vocabFile, "r");
	vocab.read(file);
    }

    Ngram *ngram;

    /*
     * create base N-gram model (either word or class-based)
     */
    SubVocab *classVocab = 0;
    if (classesFile) {
        classVocab = new SubVocab(vocab);
	assert(classVocab != 0);

	if (simpleClasses) {
	    ngram = new SimpleClassNgram(vocab, *classVocab, order);
	} else {
	    ngram = new ClassNgram(vocab, *classVocab, order);

	    if (order > 2 && !oldTrigram) {
		cerr << "warning: general class LM does not allow efficient lattice expansion; consider using -simple-classes\n";
	    }
	}
    } else {
	ngram = new Ngram(vocab, order);
    }
    assert(ngram != 0);

    ngram->debugme(debug); 
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

    SubVocab noiseVocab(vocab);
    // -pau- is always ignored in WER computation
    noiseVocab.addWord(vocab.pauseIndex);
    // read additional "noise" words to be ignored in WER computation
    if (noiseVocabFile) {
	File file(noiseVocabFile, "r");
	noiseVocab.read(file);
    }

    /*
     * Build the LM used for lattice expansion
     */
    LM *useLM = ngram;

    if (mixFile) {
	/*
	 * create a Bayes mixture LM 
	 */
	double mixLambda1 = 1.0 - mixLambda - mixLambda2 - mixLambda3 -
				mixLambda4 - mixLambda5 - mixLambda6 -
				mixLambda7 - mixLambda8 - mixLambda9;


	useLM = makeMixLM(mixFile, vocab, classVocab, order, useLM,
				mixLambda1/(mixLambda + mixLambda1));

	if (mixFile2) {
	    useLM = makeMixLM(mixFile2, vocab, classVocab, order, useLM,
				mixLambda2/(mixLambda + mixLambda1 +
							mixLambda2));
	}
	if (mixFile3) {
	    useLM = makeMixLM(mixFile3, vocab, classVocab, order, useLM,
				mixLambda3/(mixLambda + mixLambda1 +
						mixLambda2 + mixLambda3));
	}
	if (mixFile4) {
	    useLM = makeMixLM(mixFile4, vocab, classVocab, order, useLM,
				mixLambda4/(mixLambda + mixLambda1 +
					mixLambda2 + mixLambda3 + mixLambda4));
	}
	if (mixFile5) {
	    useLM = makeMixLM(mixFile5, vocab, classVocab, order, useLM,
								mixLambda5);
	}
	if (mixFile6) {
	    useLM = makeMixLM(mixFile6, vocab, classVocab, order, useLM,
								mixLambda6);
	}
	if (mixFile7) {
	    useLM = makeMixLM(mixFile7, vocab, classVocab, order, useLM,
								mixLambda7);
	}
	if (mixFile8) {
	    useLM = makeMixLM(mixFile8, vocab, classVocab, order, useLM,
								mixLambda8);
	}
	if (mixFile9) {
	    useLM = makeMixLM(mixFile9, vocab, classVocab, order, useLM,
								mixLambda9);
	}
    }

    /*
     * create multiword wrapper around LM so far, if requested
     */
    if (useMultiwordLM) {
	useLM = new MultiwordLM(vocab, *useLM);
	assert(useLM != 0);
    } 

    if (inLattice) { 
      if (inLattice2) { 
	latticeOps(inLattice, inLattice2, outLattice);
	return 0;
      } 
      processLattice(inLattice, outLattice, *useLM, vocab, noiseVocab); 
      return 0;
    } 

    if (inLatticeList) {
        if (outLatticeDir) {
	  if (mkdir(outLatticeDir, 0777) < 0) {
	    if (errno == EEXIST) {
	      if (!overwrite) {
		cerr << "Dir " << outLatticeDir
		     << " already exists, please give another one\n";
		exit(1);
	      }
	    } else {
		perror(outLatticeDir);
		exit(1);
	    }
	  }
	}
	
	FILE *refFP = 0;
	if (refFile) {
	  refFP = fopen(refFile, "r"); 
	}

	RefList reflist(vocab);
	if (refList) {
	  File file1(refList, "r"); 
	  reflist.read(file1, true); 	// add ref words to vocabulary
	}

	File listOfFiles(inLatticeList, "r"); 
	char fileName[maxWordsPerLine]; 
	int flag; 
	char *wholeName = 0; 
	char buffer[1024]; 
	while ((flag = fscanf(listOfFiles, " %1024s", buffer)) == 1) {

	  cerr << "processing file " << buffer << "\n"; 

	  if (wholeName) {
	    free((char *)wholeName); }
	  wholeName = strdup(buffer);
	  assert(wholeName != 0);
	  
	  char *sentid = strrchr(wholeName, '/');
	  if (sentid != NULL) {  
	    strcpy(wholeName, sentid+1); 
	  }

	  if (outLatticeDir) {
	    sprintf(fileName, "%s/%s", outLatticeDir, wholeName);
	    cerr << "     to be dumped to " << fileName << "\n"; 
	  } else {
	    fileName[0] = '\0';
	  }

	  VocabIndex *refVocabIndex;
	  if (refList) {
	    refVocabIndex = reflist.findRef(idFromFilename(wholeName));

	  } else if (refFile) {
	    char refLine[maxWordsPerLine];
	    fgets(refLine, sizeof(refLine), refFP); 
	  
	    if (refLine) {
	      VocabIndex refIndices[maxWordsPerLine];
	      VocabString ref[maxWordsPerLine];

	      (void)vocab.parseWords(refLine, ref, maxWordsPerLine);
	      vocab.addWords(ref, refIndices, maxWordsPerLine);

	      refVocabIndex = &refIndices[0];
	    } else { continue; }
	  } else {
	    refVocabIndex = 0; 
	  }
	  processLattice(buffer, fileName, *useLM, vocab, noiseVocab,
								refVocabIndex); 
	}
	if (wholeName) {
	  free((char *)wholeName); }
    }

    exit(0);
}
