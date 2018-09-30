/*
 * LMStats.cc --
 *	Generic methods for LM statistics
 *
 */

#ifndef lint
static char LMStats_Copyright[] = "Copyright (c) 1995-2010 SRI International.  All Rights Reserved.";
static char LMStats_RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/LMStats.cc,v 1.15 2010/06/02 05:49:58 stolcke Exp $";
#endif

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif
#include <string.h>

#include "File.h"
#include "LMStats.h"

#include "LHash.cc"
#include "Trie.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(VocabIndex, unsigned int);
#endif

/*
 * Debug levels used
 */

#define DEBUG_PRINT_TEXTSTATS	1

LMStats::LMStats(Vocab &vocab)
    : vocab(vocab), openVocab(true)
{
    addSentStart = true;
    addSentEnd = true;
}

LMStats::~LMStats()
{
}

// parse strings into words and update stats
// (weighted == true indicates each line begins with a count weight)
unsigned int
LMStats::countString(char *sentence, Boolean weighted)
{
    static VocabString words[maxWordsPerLine + 1];
    unsigned int howmany;
    
    howmany = vocab.parseWords(sentence, words, maxWordsPerLine + 1);

    if (howmany == maxWordsPerLine + 1) {
	return 0;
    } else {
	if (weighted) {
	    return countSentence(words + 1, words[0]);
	} else {
	    return countSentence(words);
	}
    }
}

// parse file into sentences and update stats
unsigned int
LMStats::countFile(File &file, Boolean weighted)
{
    unsigned numWords = 0;
    char *line;

    while ((line = file.getline())) {
	unsigned int howmany = countString(line, weighted);

	/*
	 * Since getline() returns only non-empty lines,
	 * a return value of 0 indicates some sort of problem.
	 */
	if (howmany == 0) {
	    file.position() << (weighted ? "illegal count weight or " : "")
			    << "line too long?\n";
	} else {
	    numWords += howmany;
	}
    }
    if (debug(DEBUG_PRINT_TEXTSTATS)) {
	file.position(dout()) << this -> stats;
    }
    return numWords;
}

