/*
 * HiddenNgram.cc --
 *	N-gram model with hidden between-word events
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1999, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/HiddenNgram.cc,v 1.8 2000/01/13 04:06:34 stolcke Exp $";
#endif

#include <iostream.h>
#include <stdlib.h>

#include "HiddenNgram.h"
#include "Trellis.cc"
#include "LHash.cc"
#include "Array.cc"

#define DEBUG_PRINT_WORD_PROBS          2	/* from LM.cc */
#define DEBUG_NGRAM_HITS		2	/* from Ngram.cc */
#define DEBUG_TRANSITIONS		4

const VocabString noHiddenEvent = "*noevent*";

/*
 * LM code 
 */

HiddenNgram::HiddenNgram(Vocab &vocab, SubVocab &hiddenVocab, unsigned order,
							    Boolean notHidden)
    : Ngram(vocab, order), hiddenVocab(hiddenVocab),
      trellis(maxWordsPerLine + 2 + 1), notHidden(notHidden), savedLength(0)
{
    /*
     * Make sure the hidden events are subset of base vocabulary 
     */
    assert(&vocab == &hiddenVocab.baseVocab());

    /*
     * Make sure noevent token is not used in LM, and add to the event
     * vocabulary.
     */
    assert(hiddenVocab.getIndex(noHiddenEvent) == Vocab_None);
    noEventIndex = hiddenVocab.addWord(noHiddenEvent);
}

HiddenNgram::~HiddenNgram()
{
}

void *
HiddenNgram::contextID(const VocabIndex *context, unsigned &length)
{
    /*
     * Due to the DP algorithm, we always use the full context
     * (don't inherit Ngram::contextID()).
     */
    return LM::contextID(context, length);
}

Boolean
HiddenNgram::isNonWord(VocabIndex word)
{
    /*
     * hidden events are not words: duh!
     */
    return LM::isNonWord(word) || hiddenVocab.getWord(word) != 0;
}

/*
 * Compute the prefix probability of word string (in reversed order)
 * taking hidden events into account.
 * This is done by dynamic programming, filling in the trellis[]
 * array from left to right.
 * Entry trellis[i][state].prob is the probability that of the first
 * i words while being in state.
 * The total prefix probability is the column sum in trellis[],
 * summing over all states.
 * For efficiency reasons, we specify the last word separately.
 * If context == 0, reuse the results from the previous call.
 */
LogP
HiddenNgram::prefixProb(VocabIndex word, const VocabIndex *context,
					    LogP &contextProb, TextStats &stats)
{
    /*
     * pos points to the column currently being computed (we skip over the
     *     initial <s> token)
     * prefix points to the tail of context that is used for conditioning
     *     the current word.
     */
    unsigned pos;
    int prefix;

    Boolean wasRunning = running(false);

    if (context == 0) {
	/*
	 * Reset the computation to the last iteration in the loop below
	 */
	pos = prevPos;
	prefix = 0;
	context = prevContext;

	trellis.init(pos);
    } else {
	unsigned len = Vocab::length(context);
	assert(len <= maxWordsPerLine);

	/*
	 * Save these for possible recomputation with different
	 * word argument, using same context
	 */
	prevContext = context;
	prevPos = 0;

	/*
	 * Initialization:
	 * The 0 column corresponds to the <s> prefix, and we are in the
	 * no-event state.
	 */
	VocabIndex initialContext[2];
	if (len > 0 && context[len - 1] == vocab.ssIndex) {
	    initialContext[0] = context[len - 1];
	    initialContext[1] = Vocab_None;
	    prefix = len - 1;
	} else {
	    initialContext[0] = Vocab_None;
	    prefix = len;
	}

	/*
	 * Start the DP from scratch if the context has less than two items
	 * (including <s>).  This prevents the trellis from accumulating states
	 * over multiple sentences (which computes the right thing, but
	 * slowly).
	 */
	if (len > 1 &&
	    savedLength > 0 && savedContext[0] == initialContext[0])
	{
	    /*
	     * Skip to the position where the saved
	     * context diverges from the new context.
	     */
	    for (pos = 1;
		 pos < savedLength && prefix > 0 &&
		     context[prefix - 1] == savedContext[pos];
		 pos ++, prefix --)
	    {
		prevPos = pos;
	    }

	    savedLength = pos;
	    trellis.init(pos);
	} else {
	    /*
	     * Start a DP from scratch
	     */
	    trellis.clear();
	    trellis.setProb(initialContext, LogP_One);
	    trellis.step();

	    savedContext[0] = initialContext[0];
	    savedLength = 1;
	    pos = 1;
	}
    }

    LogP logSum = LogP_Zero;

    for ( ; prefix >= 0; pos++, prefix--) {
	/*
	 * Keep track of the fact that at least one state has positive
	 * probability, needed for handling of OOVs below.
	 */
	Boolean havePosProb = false;

        VocabIndex currWord;
	
	if (prefix == 0) {
	    currWord = word;
	} else {
	    currWord = context[prefix - 1];

	    /*
	     * Cache the context words for later shortcut processing
	     */
	    savedContext[savedLength ++] = currWord;
	}

	const VocabIndex *currContext = &context[prefix];

        /*
         * Set up new context to transition to
         * (allow enough room for one hidden event per word)
         */
        VocabIndex newContext[2 * maxWordsPerLine + 1];

	/*
	 * Iterate over all contexts for the previous position in trellis
	 */
	TrellisIter<VocabContext> prevIter(trellis, pos - 1);

	VocabContext prevContext;
	LogP prevProb;

	while (prevIter.next(prevContext, prevProb)) {
            /*
             * Set up the extended context.  Allow room for adding 
             * the current word and a new hidden event.
             */
            unsigned i;
            for (i = 0; i < 2 * maxWordsPerLine && 
                                    prevContext[i] != Vocab_None; i ++)
            {
                newContext[i + 2] = prevContext[i];
            }
            newContext[i + 2] = Vocab_None;

            /*
             * Iterate over all hidden events
             */
	    VocabIter eventIter(hiddenVocab);
            VocabIndex currEvent;

            while (eventIter.next(currEvent)) {
                /*
                 * Prepend current event and word to context
                 */ 
		VocabIndex *startNewContext;
		if (currEvent == noEventIndex) {
		    startNewContext = &newContext[1];
		} else {
		    newContext[1] = currEvent;      
		    startNewContext = newContext;
		}
		startNewContext[0] = currWord;

                /*
                 * Event probability
                 */
                LogP eventProb = (currEvent == noEventIndex) ? LogP_One
				   : Ngram::wordProb(currEvent, prevContext);

		/*
		 * Transition prob out of previous context to current word.
		 * Put underlying Ngram in "running" state (for debugging etc.)
		 * only when processing a "no-event" context.
		 */
		if (prefix == 0 && currEvent == noEventIndex) {
		    running(wasRunning);
		}
		LogP wordProb = Ngram::wordProb(currWord, &startNewContext[1]);

		if (prefix == 0 && currEvent == noEventIndex) {
		    running(false);
		}

		if (wordProb != LogP_Zero) {
		    havePosProb = true;
		}

                /*
                 * Truncate context to what is actually used by LM,
                 * but keep at least one word so we can recover words later.
                 */
                unsigned usedLength;
                Ngram::contextID(startNewContext, usedLength);
                if (usedLength == 0) {
                    usedLength = 1;
                }

                VocabIndex truncatedContextWord = startNewContext[usedLength];
                startNewContext[usedLength] = Vocab_None;
	   
                if (debug(DEBUG_TRANSITIONS)) {
                    cerr << "POSITION = " << pos
                         << " FROM: " << (vocab.use(), prevContext)
                         << " TO: " << (vocab.use(), startNewContext)
                         << " WORD = " << vocab.getWord(currWord)
                         << " EVENTPROB = " << eventProb
                         << " WORDPROB = " << wordProb
                         << endl;
                }

		/*
		 * For efficiency reasons we don't update the trellis
		 * when at the final word.  In that case we just record
		 * the total probability.
		 */
		if (prefix > 0) {
		    trellis.update(prevContext, startNewContext,
							eventProb + wordProb);
		} else {
		    logSum = AddLogP(logSum, prevProb + eventProb + wordProb);
		}

                /*
                 * Restore newContext
                 */
                startNewContext[usedLength] = truncatedContextWord;
            }
	}

	/*
	 * Set noevent state probability to the previous total prefix
	 * probability if the current word had probability zero in all
	 * states, and we are not yet at the end of the prefix.
	 * This allows us to compute conditional probs based on
	 * truncated contexts, and to compute the total sentence probability
	 * leaving out the OOVs, as required by sentenceProb().
	 */
	if (prefix > 0 && !havePosProb) {
	    VocabIndex emptyContext[3];
	    emptyContext[0] = noEventIndex;
	    emptyContext[1] = currWord;
	    emptyContext[2] = Vocab_None;

	    trellis.init(pos);
	    trellis.setProb(emptyContext, trellis.sumLogP(pos - 1));

	    if (currWord == vocab.unkIndex) {
		stats.numOOVs ++;
	    } else {
	        stats.zeroProbs ++;
	    }
	}
	
	trellis.step();
	prevPos = pos;
    }

    running(wasRunning);
    
    if (prevPos > 0) {
	contextProb = trellis.sumLogP(prevPos - 1);
    } else { 
	contextProb = LogP_One;
    }
    return logSum;
}

/*
 * The conditional word probability is computed as
 *	p(w1 .... wk)/p(w1 ... w(k-1)
 */
LogP
HiddenNgram::wordProb(VocabIndex word, const VocabIndex *context)
{
    if (notHidden) {
	/*
	 * In "nothidden" we assume that we are processing a token stream
	 * that contains overt event tokens.  Hence we give event tokens
	 * probability zero (so they don't contribute to the perplexity),
	 * and we scale non-event token probabilities by
	 *		1/(1-sum of all event probs)
	 */
	if (hiddenVocab.getWord(word) != 0) {
	    if (running() && debug(DEBUG_NGRAM_HITS)) {
		dout() << "[event]";
	    }
	    return LogP_Zero;
	} else {
	    LogP totalWordProb = LogP_One;

	    VocabIter eventIter(hiddenVocab);
	    VocabIndex event;

	    Boolean wasRunning = running(false);
	    while (eventIter.next(event)) {
		totalWordProb = SubLogP(totalWordProb,
					    Ngram::wordProb(event, context));
	    }
	    running(wasRunning);

	    return Ngram::wordProb(word, context) - totalWordProb;
	}
    } else {
	/*
	 * Standard hidden event mode: do that dynamic programming thing...
	 */
	LogP cProb;
	TextStats stats;
	LogP pProb = prefixProb(word, context, cProb, stats);
	return pProb - cProb;
    }
}

LogP
HiddenNgram::wordProbRecompute(VocabIndex word, const VocabIndex *context)
{
    if (notHidden) {
	return wordProb(word, context);
    } else {
	LogP cProb;
	TextStats stats;
	LogP pProb = prefixProb(word, 0, cProb, stats);
	return pProb - cProb;
    }
}

/*
 * Sentence probabilities from indices
 *	This version computes the result directly using prefixProb to
 *	avoid recomputing prefix probs for each prefix.
 */
LogP
HiddenNgram::sentenceProb(const VocabIndex *sentence, TextStats &stats)
{
    unsigned int len = vocab.length(sentence);
    LogP totalProb;

    /*
     * The debugging machinery is not duplicated here, so just fall back
     * on the general code for that.
     */
    if (notHidden || debug(DEBUG_PRINT_WORD_PROBS)) {
	totalProb = Ngram::sentenceProb(sentence, stats);
    } else {
	VocabIndex reversed[len + 2 + 1];

	/*
	 * Contexts are represented most-recent-word-first.
	 * Also, we have to prepend the sentence-begin token,
	 * and append the sentence-end token.
	 */
	len = prepareSentence(sentence, reversed, len);

	/*
	 * Invalidate cache (for efficiency only)
	 */
	savedLength = 0;

	LogP contextProb;
	totalProb = prefixProb(reversed[0], reversed + 1, contextProb, stats);

	/* 
	 * OOVs and zeroProbs are updated by prefixProb()
	 */
	stats.numSentences ++;
	stats.prob += totalProb;
	stats.numWords += len;
    }

    return totalProb;
}

