/*
 * NBest.cc --
 *	N-best hypotheses and lists
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2001 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/NBest.cc,v 1.41 2001/08/07 06:55:57 stolcke Exp $";
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

const NBestTimestamp frameLength = 0.01; // quantization unit of word timemarks

/*
 * N-best word backtrace information
 */

const unsigned phoneStringLength = 100;
const char *phoneSeparator = ":";	 // used for phones & phoneDurs strings

NBestWordInfo::NBestWordInfo()
    : word(Vocab_None), phones(0), phoneDurs(0)
{
}

NBestWordInfo::~NBestWordInfo()
{
    if (phones) free(phones);
    if (phoneDurs) free(phoneDurs);
}

NBestWordInfo &
NBestWordInfo::operator= (const NBestWordInfo &other)
{
    if (&other == this) {
	return *this;
    }

    if (phones) free(phones);
    if (phoneDurs) free(phoneDurs);

    word = other.word;
    start = other.start;
    duration = other.duration;
    acousticScore = other.acousticScore;
    languageScore = other.languageScore;

    if (!other.phones) {
	phones = 0;
    } else {
	phones = strdup(other.phones);
	assert(phones != 0);
    }

    if (!other.phoneDurs) {
	phoneDurs = 0;
    } else {
	phoneDurs = strdup(other.phoneDurs);
	assert(phoneDurs != 0);
    }

    return *this;
}

void
NBestWordInfo::write(File &file)
{
    fprintf(file, "%lg %lg %lg %lg %s %s",
			(double)start, (double)duration,
			(double)acousticScore, (double)languageScore,
			phones ? phones : phoneSeparator,
			phoneDurs ? phoneDurs : phoneSeparator);
}

Boolean
NBestWordInfo::parse(const char *s)
{
    double sTime, dur, aScore, lScore;
    char phs[phoneStringLength], phDurs[phoneStringLength];

    if (sscanf(s, "%lg %lg %lg %lg %100s %100s",
			&sTime, &dur, &aScore, &lScore, phs, phDurs) != 6)
    {
	return false;
    } else {
	start = sTime;
	duration = dur;
	acousticScore = aScore;
	languageScore = lScore;

	if (strcmp(phs, phoneSeparator) == 0) {
	    phones = 0;
	} else {
	    phones = strdup(phs);
	    assert(phones != 0);
	}

	if (strcmp(phDurs, phoneSeparator) == 0) {
	    phoneDurs = 0;
	} else {
	    phoneDurs = strdup(phDurs);
	    assert(phoneDurs != 0);
	}

	return true;
    }
}

void
NBestWordInfo::invalidate()
{
    duration = 0.0;
}

Boolean
NBestWordInfo::valid() const
{
    return (duration > 0);
}

void
NBestWordInfo::merge(const NBestWordInfo &other)
{
    /*
     * let the "other" word information supercede our own if it has
     * higher duration-normalized acoustic likelihood
     */
    if (other.acousticScore/other.duration > acousticScore/duration)
    {
	*this = other;
    }
}

/*
 * N-Best hypotheses
 */

NBestHyp::NBestHyp()
{
    words = 0;
    wordInfo = 0;
    acousticScore = languageScore = totalScore = 0.0;
    posterior = 0.0;
    numWords = numErrors = 0;
}

NBestHyp::~NBestHyp()
{
    delete [] words;
    delete [] wordInfo;
}

NBestHyp &
NBestHyp::operator= (const NBestHyp &other)
{
    // cerr << "warning: NBestHyp::operator= called\n";

    if (&other == this) {
	return *this;
    }

    delete [] words;
    delete [] wordInfo;

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

	if (other.wordInfo) {
	    wordInfo = new NBestWordInfo[actualNumWords];
	    assert(wordInfo != 0);

	    for (unsigned i = 0; i < actualNumWords; i++) {
		wordInfo[i] = other.wordInfo[i];
	    }
	} else {
	    wordInfo = 0;
	}

    } else {
	words = 0;
	wordInfo = 0;
    }

    return *this;
}

/*
 * N-Best Hypotheses
 */

const char multiwordSeparator = '_';

static Boolean
addPhones(char *old, const char *ph) 
{
    unsigned oldLen = strlen(old);
    unsigned newLen = strlen(ph);

    if (oldLen + 1 + newLen + 1 > phoneStringLength) {
	return false;
    } else {
	if (oldLen > 0) {
	    old[oldLen ++] = phoneSeparator[0];
	}
	strcpy(&old[oldLen], ph);

	return true;
    }
}

Boolean
NBestHyp::parse(char *line, Vocab &vocab, unsigned decipherFormat,
		    LogP acousticOffset, Boolean multiwords, Boolean backtrace)
{
    const unsigned maxFieldsPerLine = 11 * maxWordsPerLine + 4;
			    /* NBestList2.0 format uses 11 fields per word */

    static VocabString wstrings[maxFieldsPerLine];
    static VocabString justWords[maxFieldsPerLine + 1];
    Array<NBestWordInfo> backtraceInfo;

    unsigned actualNumWords =
		Vocab::parseWords(line, wstrings, maxFieldsPerLine);

    if (actualNumWords == maxFieldsPerLine) {
	cerr << "more than " << maxFieldsPerLine << " fields per line\n";
	return false;
    }

    /*
     * We don't do multiword splitting with backtraces -- that would require
     * a dictionary (see external split-nbest-words script).
     */
    if (backtrace) {
	multiwords = false;
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
	/*
	 * These formats don't support backtrace info
	 */
	backtrace = false;

	actualNumWords --;

	if (actualNumWords > maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " words in hyp\n";
	    return false;
	}

	/*
	 * Parse the first word as a score (in parens)
	 */
	int score;

	if (sscanf(wstrings[0], "(%d)", &score) != 1)
	{
	    cerr << "bad Decipher score: " << wstrings[0] << endl;
	    return false;
	}

	/*
	 * Save score
	 */
	totalScore = acousticScore = BytelogToLogP(score);
	languageScore = 0.0;

	Vocab::copy(justWords, &wstrings[1]);

    } else if (decipherFormat == 2) {
	if ((actualNumWords - 1) % 11) {
	    cerr << "badly formatted hyp\n";
	    return false;
	}

	unsigned numTokens = (actualNumWords - 1)/11;

	if (numTokens > maxWordsPerLine) {
	    cerr << "more than " << maxWordsPerLine << " tokens in hyp\n";
	    return false;
	}

	/*
	 * Parse the first word as a score (in parens)
	 */
	int score;

	if (sscanf(wstrings[0], "(%d)", &score) != 1)
	{
	    cerr << "bad Decipher score: " << wstrings[0] << endl;
	    return false;
	}

	/*
	 * Parse remaining line into words and scores
	 *	skip over phone and state backtrace tokens, which can be
	 *	identified by noting that their times are contained within
	 *	the word duration.
	 */
	Bytelog acScore = 0;
	Bytelog lmScore = 0;

	NBestTimestamp prevEndTime = -1.0;	/* end time of last token */
	NBestWordInfo *prevWordInfo = 0;

	char phones[phoneStringLength];
	char phoneDurs[phoneStringLength];

	actualNumWords = 0;
	for (unsigned i = 0; i < numTokens; i ++) {

	    const char *token = wstrings[1 + 11 * i];
	    NBestTimestamp startTime = atof(wstrings[1 + 11 * i + 3]);
	    NBestTimestamp endTime = atof(wstrings[1 + 11 * i + 5]);

	    if (startTime > prevEndTime) {
		int acWordScore = atol(wstrings[1 + 11 * i + 9]);
		int lmWordScore = atol(wstrings[1 + 11 * i + 7]);

		justWords[actualNumWords] = token;

		if (backtrace) {
		    NBestWordInfo winfo;
		    winfo.word = Vocab_None;
		    winfo.start = startTime;
		    /*
		     * NB: "et" in nbest backtrace is actually the START time
		     * of the last frame
		     */
		    winfo.duration = endTime - startTime + frameLength;
		    winfo.acousticScore = BytelogToLogP(acWordScore);
		    winfo.languageScore = BytelogToLogP(lmWordScore);
		    winfo.phones = winfo.phoneDurs = 0;

		    backtraceInfo[actualNumWords] = winfo;

		    /*
		     * prepare for collecting phone backtrace info
		     */
		    prevWordInfo = &backtraceInfo[actualNumWords];
		    phones[0] = phoneDurs[0] = '\0';
		}

		acScore += acWordScore;
		lmScore += lmWordScore;

		actualNumWords ++;

		prevEndTime = endTime;
	    } else {
		/*
		 * check if this token refers to an HMM state, i.e., if
		 * if matches the pattern /-[0-9]$/
		 */
		char *hyphen = strrchr(token, '-');
		if (hyphen != 0 &&
		    hyphen[1] >= '0' && hyphen[1] <= '9' &&
		    hyphen[2] == '\0')
		{
		    continue;
		}

		/*
		 * a phone token: if we're extracting backtrace information,
		 * get phone identity and duration and store in word Info
		 */
		if (prevWordInfo) {
		    char *lbracket = strchr(token, '[');
		    const char *phone = lbracket ? lbracket + 1 : token;
		    char *rbracket = strrchr(phone, ']');
		    if (rbracket) *rbracket = '\0';
		    addPhones(phones, phone);

		    char phoneDur[20];
		    sprintf(phoneDur, "%d",
			    (int)((endTime - startTime)/frameLength + 0.5) + 1);
		    addPhones(phoneDurs, phoneDur);

		    if (endTime == prevEndTime) {
			prevWordInfo->phones = strdup(phones);
			assert(prevWordInfo->phones != 0);

			prevWordInfo->phoneDurs = strdup(phoneDurs);
			assert(prevWordInfo->phoneDurs != 0);
		    }
		}
	    }
	}
	justWords[actualNumWords] = 0;

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

	Vocab::copy(justWords, &wstrings[3]);
    }

    /*
     * Apply acoustic normalization in effect
     */
    acousticScore -= acousticOffset;
    totalScore -= acousticOffset;

    /*
     * Adjust number of words for multiwords if appropriate
     */
    if (multiwords) {
	for (unsigned j = 0; justWords[j] != 0; j ++) {
	    const char *cp = justWords[j];

	    for (cp = strchr(cp, multiwordSeparator);
		 cp != 0;
		 cp = strchr(cp + 1, multiwordSeparator))
	    {
		actualNumWords ++;
	    }
	}
    }

    /*
     * Copy words to nbest list
     */
    delete [] words;
    words = new VocabIndex[actualNumWords + 1];
    assert(words != 0);

    /*
     * Map word strings to indices
     */
    if (!multiwords) {
	vocab.addWords(justWords, words, actualNumWords + 1);

	if (backtrace) {
	    delete [] wordInfo;
	    wordInfo = new NBestWordInfo[actualNumWords + 1];

	    for (unsigned j = 0; j < actualNumWords; j ++) {
		wordInfo[j] = backtraceInfo[j];
		wordInfo[j].word = words[j];
	    }
	    wordInfo[actualNumWords].word = Vocab_None;
	} else {
	    wordInfo = 0;
	}
    } else {
	unsigned i = 0;
	for (unsigned j = 0; justWords[j] != 0; j ++) {
	    char *start = (char *)justWords[j];
	    char *cp;

	    while (cp = strchr(start, multiwordSeparator)) {
		*cp = '\0';
		words[i++] = vocab.addWord(start);
		*cp = multiwordSeparator;
		start = cp + 1;
	    }

	    words[i++] = vocab.addWord(start);
	}
	words[i] = Vocab_None;
    }

    return true;
}

void
NBestHyp::write(File &file, Vocab &vocab, Boolean decipherFormat,
						LogP acousticOffset)
{
    if (decipherFormat) {
	fprintf(file, "(%d)", (int)LogPtoBytelog(totalScore + acousticOffset));
    } else {
	fprintf(file, "%g %g %d", acousticScore + acousticOffset,
					      languageScore, numWords);
    }

    for (unsigned i = 0; words[i] != Vocab_None; i++) {
	/*
	 * Write backtrace information if known and Decipher format is desired
	 */
	if (decipherFormat && wordInfo) {
	    fprintf(file, " %s ( st: %.2f et: %.2f g: %d a: %d )", 
			   vocab.getWord(wordInfo[i].word),
			   wordInfo[i].start,
			   wordInfo[i].start+wordInfo[i].duration - frameLength,
			   (int)LogPtoBytelog(wordInfo[i].languageScore),
			   (int)LogPtoBytelog(wordInfo[i].acousticScore));
	} else {
	    fprintf(file, " %s", vocab.getWord(words[i]));
	}
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
NBestHyp::reweight(double lmScale, double wtScale, double amScale)
{
    totalScore = amScale * acousticScore +
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

NBestList::NBestList(Vocab &vocab, unsigned maxSize,
				    Boolean multiwords, Boolean backtrace)
    : vocab(vocab), _numHyps(0),
      hypList(0, initialSize), maxSize(maxSize), multiwords(multiwords),
      backtrace(backtrace), acousticOffset(0.0)
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
	unsigned numWords = Vocab::length(hypList[h].words);
	stats.total += (numWords + 1) * sizeof(VocabIndex);
	if (hypList[h].wordInfo) {
	    stats.total += (numWords + 1) * sizeof(NBestWordInfo);
	}
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
	if (strncmp(line, nbest1Magic, sizeof(nbest1Magic) - 1) == 0) {
	    decipherFormat = 1;
	    line = file.getline();
	} else if (strncmp(line, nbest2Magic, sizeof(nbest2Magic) - 1) == 0) {
	    decipherFormat = 2;
	    line = file.getline();
	}
    }

    unsigned int howmany = 0;

    while (line && (maxSize == 0 || howmany < maxSize)) {
	if (! hypList[howmany++].parse(line, vocab, decipherFormat,
					acousticOffset, multiwords, backtrace))
	{
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
	fprintf(file, "%s\n", backtrace ? nbest2Magic : nbest1Magic);
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
NBestList::reweightHyps(double lmScale, double wtScale, double amScale)
{
    for (unsigned h = 0; h < _numHyps; h++) {
	hypList[h].reweight(lmScale, wtScale, amScale);
    }
}

/*
 * Compute posterior probabilities
 */
void
NBestList::computePosteriors(double lmScale, double wtScale,
					    double postScale, double amScale)
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
	LogP totalScore = (amScale * hyp.acousticScore +
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
    NBestWordInfo endOfHyp;
    endOfHyp.word = Vocab_None;

    for (unsigned h = 0; h < _numHyps; h++) {
	lm.removeNoise(hypList[h].words);

	// remove corresponding tokens from wordInfo array
	if (hypList[h].wordInfo) {

	    unsigned k = 0;	// index into wordInfo
	    for (unsigned i = 0; hypList[h].words[i] != Vocab_None; i ++) {

		// find corresponding word in wordInfo
		while (hypList[h].wordInfo[k].word != hypList[h].words[i]) {
		    k ++;
		}
		// copy it to target position
		if (i != k) {
		    hypList[h].wordInfo[i] = hypList[h].wordInfo[k];
		    hypList[h].wordInfo[k] = endOfHyp;
		}
	    }
	}
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

