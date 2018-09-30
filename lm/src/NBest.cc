/*
 * NBest.cc --
 *	N-best hypotheses and lists
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-1998 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/NBest.cc,v 1.34 1999/01/13 19:24:33 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "NBest.h"
#include "WordAlign.h"

#include "Array.cc"
#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_ARRAY(NBestHyp);
#endif

#define DEBUG_PRINT_RANK	1

NBestHyp::NBestHyp()
{
    words = 0;
    acousticScore = languageScore = totalScore = 0.0;
    posterior = 0.0;
    numWords = numErrors = 0;
}

NBestHyp::~NBestHyp()
{
    delete [] words;
}

NBestHyp &
NBestHyp::operator= (const NBestHyp &other)
{
    // cerr << "warning: NBestHyp::operator= called\n";

    if (&other == this) {
	return *this;
    }

    delete [] words;

    acousticScore = other.acousticScore;
    languageScore = other.languageScore;
    totalScore = other.totalScore;

    numWords = other.numWords;

    if (other.words) {
	unsigned actualNumWords = Vocab::length(other.words) + 1;

	words = new VocabIndex[actualNumWords];
	assert(words != 0);

	for (unsigned i = 0; i < actualNumWords; i++) {
	    words[i] = other.words[i];
	}
    } else {
	words = 0;
    }
    return *this;
}

/*
 * N-Best Hyoptheses
 */

Boolean
NBestHyp::parse(char *line, Vocab &vocab, unsigned decipherFormat,
							    LogP acousticOffset)
{
    const unsigned maxFieldsPerLine = 11 * maxWordsPerLine + 4;
			    /* NBestList2.0 format uses 11 fields per word */

    static VocabString wstrings[maxFieldsPerLine];
    static VocabIndex wids[maxWordsPerLine + 1];

    unsigned actualNumWords =
		Vocab::parseWords(line, wstrings, maxFieldsPerLine);

    if (actualNumWords == maxFieldsPerLine) {
	cerr << "more than " << maxFieldsPerLine << " fields per line\n";
	return false;
    }

    /*
     * We accept three formats for N-best hyps.
     * - The Decipher NBestList1.0 format, which has one combined bytelog score
     *	 in parens preceding the hyp.
     * - The Decipher NBestList2.0 format, where each word is followed by
     *	  ( st: <starttime> et: <endtime> g: <grammar_score> a: <ac_score> )
     * - Our own format, which has acoustic score, LM score, and number of
     *   words followed by the hyp.
     * If (decipherFormat > 0) only the specified Decipher format is accepted.
     */

    if (decipherFormat == 1 || 
	decipherFormat == 0 && wstrings[0][0] == '(')
    {
	actualNumWords --;

	if (actualNumWords > maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " words in hyp\n";
	    return false;
	}

	/*
	 * Parse the first word as a score (in parens)
	 */
	double score;

	if (sscanf(wstrings[0], "(%lf)", &score) != 1)
	{
	    cerr << "bad Decipher score: " << wstrings[0] << endl;
	    return false;
	}

	/*
	 * Save score
	 */
	totalScore = acousticScore = BytelogToLogP(score);
	languageScore = 0.0;

	/* 
	 * Map words to indices
	 */
	vocab.addWords(wstrings + 1, wids, maxWordsPerLine + 1);
    } else if (decipherFormat == 2) {
	if ((actualNumWords - 1) % 11) {
	    cerr << "badly formatted hyp\n";
	    return false;
	}

	actualNumWords = (actualNumWords - 1)/11;

	if (actualNumWords > maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " words in hyp\n";
	    return false;
	}

	/*
	 * Parse the first word as a score (in parens)
	 */
	double score;

	if (sscanf(wstrings[0], "(%lf)", &score) != 1)
	{
	    cerr << "bad Decipher score: " << wstrings[0] << endl;
	    return false;
	}

	/*
	 * Parse remaining line into words and scores
	 */
	double acScore = 0.0;
	double lmScore = 0.0;

	for (unsigned i = 0; i < actualNumWords; i ++) {
	    wids[i] = vocab.addWord(wstrings[1 + 11 * i]);

	    acScore += atof(wstrings[1 + 11 * i + 9]);
	    lmScore += atof(wstrings[1 + 11 * i + 7]);
	}
	wids[actualNumWords] = Vocab_None;

	/*
	 * Save scores
	 */
	totalScore = BytelogToLogP(score);
	acousticScore = BytelogToLogP(acScore);
	languageScore = BytelogToLogP(lmScore);

	if (score != acScore + lmScore) {
	    cerr << "acoustic and language model scores don't add up ("
		 << acScore << " + " << lmScore << " != " << score << ")\n";
	}

    } else {
	actualNumWords -= 3;

	if (actualNumWords > maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " words in hyp\n";
	    return false;
	}

	/*
	 * Parse the first three columns as numbers
	 */
	if (sscanf(wstrings[0], "%f", &acousticScore) != 1)
	{
	    cerr << "bad acoustic score: " << wstrings[0] << endl;
	    return false;
	}
	if (sscanf(wstrings[1], "%f", &languageScore) != 1)
	{
	    cerr << "bad LM score: " << wstrings[1] << endl;
	    return false;
	}
	if (sscanf(wstrings[2], "%u", &numWords) != 1)
	{
	    cerr << "bad word count: " << wstrings[2] << endl;
	    return false;
	}

	/*
	 * Set the total score to the acoustic score so 
	 * decipherFix() with a null language model leaves everything
	 * unchanged.
	 */
	totalScore = acousticScore;

	/* 
	 * Map words to indices
	 */
	vocab.addWords(wstrings + 3, wids, maxWordsPerLine + 1);
    }

    /*
     * Apply acoustic normalization in effect
     */
    acousticScore -= acousticOffset;
    totalScore -= acousticOffset;

    /*
     * Copy words to nbest list, including end marker
     */
    words = new VocabIndex[actualNumWords + 1];
    assert(words != 0);

    for (unsigned i = 0; i <= actualNumWords; i++) {
	words[i] = wids[i];
    }

    return true;
}

void
NBestHyp::write(File &file, Vocab &vocab, Boolean decipherFormat,
						LogP acousticOffset)
{
    if (decipherFormat) {
	fprintf(file, "(%g)", LogPtoBytelog(totalScore + acousticOffset));
    } else {
	fprintf(file, "%g %g %d", acousticScore + acousticOffset,
					      languageScore, numWords);
    }

    for (unsigned i = 0; words[i] != Vocab_None; i++) {
	fprintf(file, " %s", vocab.getWord(words[i]));
    }

    fprintf(file, "\n");
}

void
NBestHyp::rescore(LM &lm, double lmScale, double wtScale)
{
    TextStats stats;

    /*
     * LM score is recomputed,
     * numWords is set to take non-word tokens into account
     */
    languageScore = lmScale * lm.sentenceProb(words, stats);
    numWords = stats.numWords;

    /*
     * Note: In the face of zero probaility words we do NOT
     * set the LM probability to zero.  These cases typically
     * reflect a vocabulary mismatch between the rescoring LM
     * and the recognizer, and it is more useful to rescore based on
     * the known words alone.  The warning hopefull will cause
     * someone to asssess the problem.
     */
    if (stats.zeroProbs > 0) {
	cerr << "warning: hyp contains zero prob words: "
	     << (lm.vocab.use(), words) << endl;
    }

    if (stats.numOOVs > 0) {
	cerr << "warning: hyp contains OOV words: "
	     << (lm.vocab.use(), words) << endl;
    }

    totalScore = acousticScore +
			    languageScore +
			    wtScale * numWords;
}

void
NBestHyp::reweight(double lmScale, double wtScale)
{
    totalScore = acousticScore +
			    lmScale * languageScore +
			    wtScale * numWords;
}

void
NBestHyp::decipherFix(LM &lm, double lmScale, double wtScale)
{
    TextStats stats;

    /*
     * LM score is recomputed,
     * numWords is set to take non-word tokens into account
     */
    languageScore = lmScale * lm.sentenceProb(words, stats);
    numWords = stats.numWords;

    /*
     * Arguably a bug, but Decipher actually applies WTW to pauses.
     * So we have to do the same when subtracting the non-acoustic
     * scores below.
     */
    unsigned numAllWords = Vocab::length(words);

    if (stats.zeroProbs > 0) {
	cerr << "warning: hyp contains zero prob words: "
	     << (lm.vocab.use(), words) << endl;
	languageScore = LogP_Zero;
    }

    if (stats.numOOVs > 0) {
	cerr << "warning: hyp contains OOV words: "
	     << (lm.vocab.use(), words) << endl;
	languageScore = LogP_Zero;
    }

    acousticScore = totalScore -
			    languageScore -
			    wtScale * numAllWords;
}


/*
 * N-Best lists
 */

unsigned NBestList::initialSize = 100;

NBestList::NBestList(Vocab &vocab, unsigned maxSize)
    : vocab(vocab), _numHyps(0),
      hypList(0, initialSize), maxSize(maxSize),
      acousticOffset(0.0)
{
}

/* 
 * Compute memory usage
 */
void
NBestList::memStats(MemStats &stats)
{
    stats.total += sizeof(*this) - sizeof(hypList);
    hypList.memStats(stats);

    /*
     * Add space taken up by hyp strings
     */
    for (unsigned h = 0; h < _numHyps; h++) {
	stats.total += Vocab::length(hypList[h].words) + 1;
    }
}

static int
compareHyps(const void *h1, const void *h2)
{
    LogP score1 = ((NBestHyp *)h1)->totalScore;
    LogP score2 = ((NBestHyp *)h2)->totalScore;
    
    return score1 > score2 ? -1 :
		score1 < score2 ? 1 : 0;
}

void
NBestList::sortHyps()
{
    /*
     * Sort the underlying array in place, in order of descending scores
     */
    qsort(hypList.data(), _numHyps, sizeof(NBestHyp), compareHyps);
}

static const char fileMagic[] = "NBestList1.0";
static const char fileMagic2[] = "NBestList2.0";

Boolean
NBestList::read(File &file)
{
    char *line = file.getline();
    unsigned decipherFormat = 0;

    /*
     * If the first line contains the Decipher magic string
     * we enforce Decipher format for the entire N-best list.
     */
    if (line != 0) {
	if (strncmp(line, fileMagic, sizeof(fileMagic) - 1) == 0) {
	    decipherFormat = 1;
	    line = file.getline();
	} else if (strncmp(line, fileMagic2, sizeof(fileMagic2) - 1) == 0) {
	    decipherFormat = 2;
	    line = file.getline();
	}
    }

    unsigned int howmany = 0;

    while (line && (maxSize == 0 || howmany < maxSize)) {
	if (! hypList[howmany++].parse(line, vocab, decipherFormat,
						    acousticOffset)) {
	    file.position() << "bad n-best hyp\n";
	    return false;
	}

	line = file.getline();
    }

    _numHyps = howmany;

    return true;
}

Boolean
NBestList::write(File &file, Boolean decipherFormat, unsigned numHyps)
{
    if (decipherFormat) {
	fprintf(file, "%s\n", fileMagic);
    }

    for (unsigned h = 0;
	 h < _numHyps && (numHyps == 0 || h < numHyps);
	 h++)
    {
	hypList[h].write(file, vocab, decipherFormat, acousticOffset);
    }

    return true;
}

/*
 * Recompute total scores by recomputing LM scores and adding them to the
 * acoustic scores including a word transition penalty.
 */
void
NBestList::rescoreHyps(LM &lm, double lmScale, double wtScale)
{
    for (unsigned h = 0; h < _numHyps; h++) {
	hypList[h].rescore(lm, lmScale, wtScale);
    }
}

/*
 * Recompute total hyp scores using new scaling constants.
 */
void
NBestList::reweightHyps(double lmScale, double wtScale)
{
    for (unsigned h = 0; h < _numHyps; h++) {
	hypList[h].reweight(lmScale, wtScale);
    }
}

/*
 * Compute posterior probabilities
 */
void
NBestList::computePosteriors(double lmScale, double wtScale, double postScale)
{
    /*
     * First compute the numerators for the posteriors
     */
    Prob totalNumerator = 0.0;
    LogP scoreOffset;

    unsigned h;
    for (h = 0; h < _numHyps; h++) {
	NBestHyp &hyp = hypList[h];

	/*
	 * This way of computing the total score differs from 
	 * hyp.reweight() in that we're scaling back the acoustic
	 * scores, rather than scaling up the LM scores.
	 *
	 * Store the score back into the nbest list so we can
	 * sort on it later.
	 *
	 * The posterior weight is a parameter that controls the
	 * peakedness of the posterior distribution.
	 */
	LogP totalScore = (hyp.acousticScore +
			    lmScale * hyp.languageScore +
			    wtScale * hyp.numWords) /
			 postScale;

	/*
	 * To prevent underflow when converting LogP's to Prob's, we 
	 * subtract off the LogP of the first hyp.
	 * This is equivalent to a constant factor on all Prob's, which
	 * cancels in the normalization.
	 */
	if (h == 0) {
	    scoreOffset = totalScore;
	    totalScore = 0.0;
	} else {
	    totalScore -= scoreOffset;
	}

	hyp.posterior = LogPtoProb(totalScore);

	totalNumerator += hyp.posterior;
    }

    /*
     * Normalize posteriors
     */
    for (h = 0; h < _numHyps; h++) {
	NBestHyp &hyp = hypList[h];

	hyp.posterior /= totalNumerator;
    }
}

/*
 * Recompute acoustic scores by subtracting recognizer LM scores
 * from totals.
 */
void
NBestList::decipherFix(LM &lm, double lmScale, double wtScale)
{
    for (unsigned h = 0; h < _numHyps; h++) {
	hypList[h].decipherFix(lm, lmScale, wtScale);
    }
}

/*
 * Remove noise and pause words from hyps
 */
void
NBestList::removeNoise(LM &lm)
{
    for (unsigned h = 0; h < _numHyps; h++) {
	lm.removeNoise(hypList[h].words);
    }
}

/*
 * Normalize acoustic scores so that maximum is 0
 */
void
NBestList::acousticNorm()
{
    unsigned h;
    LogP maxScore;

    /*
     * Find maximum acoustic score
     */
    for (h = 0; h < _numHyps; h++) {
	if (h == 0 || hypList[h].acousticScore > maxScore) {
	    maxScore = hypList[h].acousticScore;
	}
    }

    /* 
     * Normalize all scores
     */
    for (h = 0; h < _numHyps; h++) {
	hypList[h].acousticScore -= maxScore;
	hypList[h].totalScore -= maxScore;
    }

    acousticOffset = maxScore;
}

/*
 * Restore acoustic scores to their un-normalized values
 */
void
NBestList::acousticDenorm()
{
    for (unsigned h = 0; h < _numHyps; h++) {
	hypList[h].acousticScore += acousticOffset;
	hypList[h].totalScore -= acousticOffset;
    }

    acousticOffset = 0.0;
}

/*
 * compute minimal word error of all hyps in the list
 * (and set hyp error counts)
 */
unsigned
NBestList::wordError(const VocabIndex *words,
				unsigned &sub, unsigned &ins, unsigned &del)
{
    unsigned minErr = (unsigned)(-1);

    for (unsigned h = 0; h < _numHyps; h++) {
	unsigned s, i, d;
	unsigned werr = ::wordError(hypList[h].words, words, s, i, d);

	if (h == 0 || werr < minErr) {
	    minErr = werr;
	    sub = s;
	    ins = i;
	    del = d;
	}

	hypList[h].numErrors = werr;
    }

    if (_numHyps == 0) {
	/* 
	 * If the n-best lists is empty we count all reference words as deleted.
	 */
	minErr = del = Vocab::length(words);
	sub = 0;
	ins = 0;
    }

    return minErr;
}

/*
 * Return hyp with minimum expected word error
 */
double
NBestList::minimizeWordError(VocabIndex *words, unsigned length,
				double &subs, double &inss, double &dels,
				unsigned maxRescore, Prob postPrune)
{
    /*
     * Compute expected word errors
     */
    double bestError;
    unsigned bestHyp;

    unsigned howmany = (maxRescore > 0) ? maxRescore : _numHyps;
    if (howmany > _numHyps) {
	howmany = _numHyps;
    }

    for (unsigned i = 0; i < howmany; i ++) {
	NBestHyp &hyp = getHyp(i);

	double totalErrors = 0.0;
	double totalSubs = 0.0;
	double totalInss = 0.0;
	double totalDels = 0.0;
	Prob totalPost = 0.0;

	for (unsigned j = 0; j < _numHyps; j ++) {
	    NBestHyp &otherHyp = getHyp(j);

	    if (i != j) {
		unsigned sub, ins, del;
		totalErrors += otherHyp.posterior *
			::wordError(hyp.words, otherHyp.words, sub, ins, del);
		totalSubs += otherHyp.posterior * sub;
		totalInss += otherHyp.posterior * ins;
		totalDels += otherHyp.posterior * del;
	    }

	    /*
	     * Optimization: if the partial accumulated error exceeds the
	     * current best error then this cannot be a new best.
	     */
	    if (i > 0 && totalErrors > bestError) {
		break;
	    }

	    /*
	     * Ignore hyps whose cummulative posterior mass is below threshold
	     */
	    totalPost += otherHyp.posterior;
	    if (postPrune > 0.0 && totalPost > 1.0 - postPrune) {
		break;
	    }
	}

	if (i == 0 || totalErrors < bestError) {
	    bestHyp = i;
	    bestError = totalErrors;
	    subs = totalSubs;
	    inss = totalInss;
	    dels = totalDels;
	}
    }

    if (debug(DEBUG_PRINT_RANK)) {
	cerr << "best hyp = " << bestHyp
	     << " post = " << getHyp(bestHyp).posterior
	     << " wer = " << bestError << endl;
    }

    if (howmany > 0) {
	for (unsigned j = 0; j < length; j ++) {
	    words[j] = getHyp(bestHyp).words[j];

	    if (words[j] == Vocab_None) break;
	}

	return bestError;
    } else {
	if (length > 0) {
	    words[0] = Vocab_None;
	}

	return 0.0;
    }
}

