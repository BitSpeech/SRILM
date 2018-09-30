/*
 * Ngram.cc --
 *	N-gram backoff language models
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,1997 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/Ngram.cc,v 1.68 2000/02/18 05:09:11 stolcke Exp $";
#endif

#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <new.h>

#include "Ngram.h"
#include "File.h"

#ifdef USE_SARRAY

#include "SArray.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_SARRAY(VocabIndex,LogP); 
#endif

#else /* ! USE_SARRAY */

#include "LHash.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(VocabIndex,LogP); 
#endif

#endif /* USE_SARRAY */

#include "Trie.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_TRIE(VocabIndex,BOnode);
#endif

/*
 * Debug levels used
 */
#define DEBUG_ESTIMATE_WARNINGS	1
#define DEBUG_FIXUP_WARNINGS 3
#define DEBUG_PRINT_GTPARAMS 2
#define DEBUG_READ_STATS 1
#define DEBUG_WRITE_STATS 1
#define DEBUG_NGRAM_HITS 2
#define DEBUG_ESTIMATES 4

/* these are the same as in LM.cc */
#define DEBUG_PRINT_SENT_PROBS		1
#define DEBUG_PRINT_WORD_PROBS		2
#define DEBUG_PRINT_PROB_SUMS		3

const LogP LogP_PseudoZero = -99.0;	/* non-inf value used for log 0 */

/*
 * Low level methods to access context (BOW) nodes and probs
 */

void
Ngram::memStats(MemStats &stats)
{
    stats.total += sizeof(*this) - sizeof(contexts);
    contexts.memStats(stats);

    /*
     * The probs tables are not included in the above call,
     * so we have to count those separately.
     */
    VocabIndex context[order + 1];

    for (int i = 1; i <= order; i++) {
	NgramBOsIter iter(*this, context, i - 1);
	BOnode *node;

	while (node = iter.next()) {
	    stats.total -= sizeof(node->probs);
	    node->probs.memStats(stats);
	}
    }
}

Ngram::Ngram(Vocab &vocab, unsigned neworder)
    : LM(vocab), order(neworder), skipOOVs(false), trustTotals(false)
{
    if (order < 1) {
	order = 1;
    }
    dummyIndex = Vocab_None;
}

unsigned 
Ngram::setorder(unsigned neworder)
{
    unsigned oldorder = order;

    if (neworder > 0) {
	order = neworder;
    }

    return oldorder;
}

/*
 * Locate a BOW entry in the n-gram trie
 */
LogP *
Ngram::findBOW(const VocabIndex *context)
{
    BOnode *bonode = contexts.find(context);
    if (bonode) {
	return &(bonode->bow);
    } else {
	return 0;
    }
}

/*
 * Locate a prob entry in the n-gram trie
 */
LogP *
Ngram::findProb(VocabIndex word, const VocabIndex *context)
{
    BOnode *bonode = contexts.find(context);
    if (bonode) {
	return bonode->probs.find(word);
    } else {
	return 0;
    }
}

/*
 * Locate or create a BOW entry in the n-gram trie
 */
LogP *
Ngram::insertBOW(const VocabIndex *context)
{
    Boolean found;
    BOnode *bonode = contexts.insert(context, found);

    if (!found) {
	/*
	 * initialize the index in the BOnode
	 */
	new (&bonode->probs) PROB_INDEX_T<VocabIndex,LogP>(0);
    }
    return &(bonode->bow);
}

/*
 * Locate or create a prob entry in the n-gram trie
 */
LogP *
Ngram::insertProb(VocabIndex word, const VocabIndex *context)
{
    Boolean found;
    BOnode *bonode = contexts.insert(context, found);

    if (!found) {
	/*
	 * initialize the index in the BOnode
	 */
	new (&bonode->probs) PROB_INDEX_T<VocabIndex,LogP>(0);
    }
    return bonode->probs.insert(word);
}

/*
 * Remove a BOW node (context) from the n-gram trie
 */
void
Ngram::removeBOW(const VocabIndex *context)
{
    contexts.removeTrie(context);
}

/*
 * Remove a prob entry from the n-gram trie
 */
void
Ngram::removeProb(VocabIndex word, const VocabIndex *context)
{
    BOnode *bonode = contexts.find(context);

    if (bonode) {
	bonode->probs.remove(word);
    }
}

/*
 * Higher level methods, implemented low-level for efficiency
 */

void *
Ngram::contextID(const VocabIndex *context, unsigned &length)
{
    /*
     * Find the longest BO node that matches a prefix of context.
     * Returns its address as the ID.
     */
    BOtrie *trieNode = &contexts;

    int i = 0;
    while (i < order - 1 && context[i] != Vocab_None) {
	BOtrie *next = trieNode->findTrie(context[i]);
	if (next) {
	    trieNode = next;
	    i ++;
	} else {
	    break;
	}
    }

    length = i;
    return (void *)trieNode;
}

/*
 * Higher level methods (using the lower-level methods)
 */

/*
 * This method implements the backoff algorithm in a straightforward,
 * recursive manner.
 */
LogP
Ngram::wordProbBO(VocabIndex word, const VocabIndex *context, unsigned int clen)
{
    LogP result;
    /*
     * To use the context array for lookup we truncate it to the
     * indicated length, restoring the mangled item on exit.
     */
    VocabIndex saved = context[clen];
    ((VocabIndex *)context)[clen] = Vocab_None;	/* discard const */

    LogP *prob = findProb(word, context);

    if (prob) {
	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "[" << (clen + 1) << "gram]";
	}
	result = *prob;
    } else if (clen > 0) {
	LogP *bow = findBOW(context);

	if (bow) {
	   result = *bow + wordProbBO(word, context, clen - 1);
	} else {
	   result = wordProbBO(word, context, clen - 1);
	}
    } else {
	if (running() && debug(DEBUG_NGRAM_HITS)) {
	    dout() << "[OOV]";
	}
	result = LogP_Zero;
    }
    ((VocabIndex *)context)[clen] = saved;	/* discard const */
    return result;
}

LogP
Ngram::wordProb(VocabIndex word, const VocabIndex *context)
{
    unsigned int clen = Vocab::length(context);

    if (skipOOVs) {
	/*
	 * Backward compatibility with the old broken perplexity code:
	 * return prob 0 if any of the context-words have an unknown
	 * word.
	 */
	if (word == vocab.unkIndex ||
	    order > 1 && context[0] == vocab.unkIndex ||
	    order > 2 && context[2] == vocab.unkIndex)
	{
	    return LogP_Zero;
	}
    }

    /*
     * Perform the backoff algorithm for a context lenth that is 
     * the minimum of what we were given and the maximal length of
     * the contexts stored in the LM
     */
    return wordProbBO(word, context, ((clen > order - 1) ? order - 1 : clen));
}

Boolean
Ngram::read(File &file)
{
    char *line;
    unsigned int maxOrder = 0;	/* maximal n-gram order in this model */
    unsigned int numNgrams[maxNgramOrder + 1];
				/* the number of n-grams for each order */
    unsigned int numRead[maxNgramOrder + 1];
				/* Number of n-grams actually read */
    int state = -1 ;		/* section of file being read:
				 * -1 - pre-header, 0 - header,
				 * 1 - unigrams, 2 - bigrams, ... */
    Boolean warnedAboutUnk = false; /* at most one warning about <unk> */

    for (int i = 0; i <= maxNgramOrder; i++) {
	numNgrams[i] = 0;
	numRead[i] = 0;
    }

    /*
     * The ARPA format implicitly assumes a zero-gram backoff weight of 0.
     * This has to be properly represented in the BOW trie so various
     * recursive operation work correctly.
     */
    VocabIndex nullContext[1];
    nullContext[0] = Vocab_None;
    *insertBOW(nullContext) = LogP_Zero;

    while (line = file.getline()) {
	
	Boolean backslash = (line[0] == '\\');

	switch (state) {

	case -1: 	/* looking for start of header */
	    if (backslash && strncmp(line, "\\data\\", 6) == 0) {
		state = 0;
		continue;
	    }
	    /*
	     * Everything before "\\data\\" is ignored
	     */
	    continue;

	case 0:		/* ngram header */
	    unsigned thisOrder;
	    int nNgrams;

	    if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
		/*
		 * start reading n-grams
		 */
		if (state < 1 || state > maxOrder) {
		    file.position() << "invalid ngram order " << state << "\n";
		    return false;
		}

	        if (debug(DEBUG_READ_STATS)) {
		    dout() << (state <= order ? "reading " : "skipping ")
			   << numNgrams[state] << " "
			   << state << "-grams\n";
		}

		continue;
	    } else if (sscanf(line, "ngram %d=%d", &thisOrder, &nNgrams) == 2) {
		/*
		 * scanned a line of the form
		 *	ngram <N>=<howmany>
		 * now perform various sanity checks
		 */
		if (thisOrder <= 0 || thisOrder > maxNgramOrder) {
		    file.position() << "ngram order " << thisOrder
				    << " out of range\n";
		    return false;
		}
		if (nNgrams < 0) {
		    file.position() << "ngram number " << nNgrams
				    << " out of range\n";
		    return false;
		}
		if (thisOrder > maxOrder) {
		    maxOrder = thisOrder;
		}
		numNgrams[thisOrder] = nNgrams;
		continue;
	    } else {
		file.position() << "unexpected input\n";
		return false;
	    }

	default:	/* reading n-grams, where n == state */

	    if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
		if (state < 1 || state > maxOrder) {
		    file.position() << "invalid ngram order " << state << "\n";
		    return false;
		}

	        if (debug(DEBUG_READ_STATS)) {
		    dout() << (state <= order ? "reading " : "skipping ")
			   << numNgrams[state] << " "
			   << state << "-grams\n";
		}

		/*
		 * start reading more n-grams
		 */
		continue;
	    } else if (backslash && strncmp(line, "\\end\\", 5) == 0) {
		/*
		 * Check that the total number of ngrams read matches
		 * that found in the header
		 */
		for (int i = 0; i <= maxOrder && i <= order; i++) {
		    if (numNgrams[i] != numRead[i]) {
			file.position() << "warning: " << numRead[i] << " "
			                << i << "-grams read, expected "
			                << numNgrams[i] << "\n";
		    }
		}

		return true;
	    } else if (state > order) {
		/*
		 * Save time and memory by skipping ngrams outside
		 * the order range of this model
		 */
		continue;
	    } else {
		VocabString words[1+ maxNgramOrder + 1 + 1];
				/* result of parsing an n-gram line
				 * the first and last elements are actually
				 * numerical parameters, but so what? */
		VocabIndex wids[maxNgramOrder + 1];
				/* ngram translated to word indices */
		LogP prob, bow; /* probability and back-off-weight */

		/*
		 * Parse a line of the form
		 *	<prob>	<w1> <w2> ...	<bow>
		 */
		unsigned int howmany =
				Vocab::parseWords(line, words, state + 3);

		if (howmany < state + 1 || howmany > state + 2) {
		    file.position() << "ngram line has " << howmany
				    << " fields (" << state + 2
				    << " expected)\n";
		    return false;
		}

		/*
		 * Parse prob
		 */
		if (!parseLogP(words[0], prob)) {
		    file.position() << "bad prob \"" << words[0] << "\"\n";
		    return false;
		} else if (prob > LogP_One || prob != prob) {
		    file.position() << "warning: questionable prob \""
				    << words[0] << "\"\n";
		}

		/* 
		 * Parse bow, if any
		 */
		if (howmany == state + 2) {
		    /*
		     * Parsing floats strings is the most time-consuming
		     * part of reading in backoff models.  We therefore
		     * try to avoid parsing bows where they are useless,
		     * i.e., for contexts that are longer than what this
		     * model uses.  We also do a quick sanity check to
		     * warn about non-zero bows in that position.
		     */
		    if (state == maxOrder) {
			if (words[state + 1][0] != '0') {
			    file.position() << "ignoring non-zero bow \""
					    << words[state + 1]
					    << "\" for maximal ngram\n";
			}
		    } else if (state == order) {
			/*
			 * save time and memory by skipping bows that will
			 * never be used as a result of higher-order ngram
			 * skipping
			 */
			;
		    } else if (!parseLogP(words[state + 1], bow)) {
			file.position() << "bad bow \"" << words[state + 1]
					<< "\"\n";
			return false;
		    } else if (bow == LogP_Inf || bow != bow) {
			file.position() << "warning: questionable bow \""
		                    	<< words[state + 1] << "\"\n";
		    }
		}

		/* 
		 * Terminate the words array after the last word,
		 * then translate it to word indices.  We also
		 * reverse the ngram since that's how we'll need it
		 * to index the trie.
		 */
		words[state + 1] = 0;
		vocab.addWords(&words[1], wids, maxNgramOrder);
		Vocab::reverse(wids);
		
		/*
		 * Store bow, if any
		 */
		if (howmany == state + 2 && state < order) {
		    *insertBOW(wids) = bow;
		}

		/*
		 * Save the last word (which is now the first, due to reversal)
		 * then use the first n-1 to index into
		 * the context trie, storing the prob.
		 */
		if (!findBOW(&wids[1])) {
		    file.position() << "warning: no bow for prefix of ngram \""
				    << &words[1] << "\"\n";
		} else {
		    if (!warnedAboutUnk &&
			wids[0] == vocab.unkIndex &&
			prob != LogP_Zero && prob != LogP_PseudoZero &&
		    	!vocab.unkIsWord)
		    {
			file.position() << "warning: non-zero probability for "
			                << vocab.getWord(vocab.unkIndex)
			                << " in closed-vocabulary LM\n";
			warnedAboutUnk = true;
		    }

		    *insertProb(wids[0], &wids[1]) = prob;
		}

		/*
		 * Hey, we're done with this ngram!
		 */
		numRead[state] ++;
	    }
	}
    }

    /*
     * we reached a premature EOF
     */
    file.position() << "reached EOF before \\end\\\n";
    return false;
}

void
Ngram::writeWithOrder(File &file, unsigned int order)
{
    unsigned int i;
    unsigned howmanyNgrams[maxNgramOrder + 1];
    VocabIndex context[maxNgramOrder + 2];
    VocabString scontext[maxNgramOrder + 1];

    if (order > maxNgramOrder) {
	order = maxNgramOrder;
    }

    fprintf(file, "\n\\data\\\n");

    for (i = 1; i <= order; i++ ) {
	howmanyNgrams[i] = numNgrams(i);
	fprintf(file, "ngram %d=%d\n", i, howmanyNgrams[i]);
    }

    for (i = 1; i <= order; i++ ) {
	fprintf(file, "\n\\%d-grams:\n", i);

        if (debug(DEBUG_WRITE_STATS)) {
	    dout() << "writing " << howmanyNgrams[i] << " "
		   << i << "-grams\n";
	}
        
	NgramBOsIter iter(*this, context + 1, i - 1, vocab.compareIndex());
	BOnode *node;

	while (node = iter.next()) {

	    vocab.getWords(context + 1, scontext, maxNgramOrder + 1);
	    Vocab::reverse(scontext);

	    NgramProbsIter piter(*node, vocab.compareIndex());
	    VocabIndex pword;
	    LogP *prob;

	    while (prob = piter.next(pword)) {
		if (file.error()) {
		    return;
		}

		fprintf(file, "%.*lg\t", LogP_Precision,
				(double)(*prob == LogP_Zero ?
						LogP_PseudoZero : *prob));
		Vocab::write(file, scontext);
		fprintf(file, "%s%s", (i > 1 ? " " : ""), vocab.getWord(pword));

		if (i < order) {
		    context[0] = pword;

		    LogP *bow = findBOW(context);
		    if (bow) {
			fprintf(file, "\t%.*lg", LogP_Precision,
					(double)(*bow == LogP_Zero ?
						    LogP_PseudoZero : *bow));
		    }
		}

		fprintf(file, "\n");
	    }
	}
    }

    fprintf(file, "\n\\end\\\n");
}

unsigned int
Ngram::numNgrams(unsigned int order)
{
    if (order < 1) {
	return 0;
    } else {
	unsigned int howmany = 0;

	VocabIndex context[order + 1];

	NgramBOsIter iter(*this, context, order - 1);
	BOnode *node;

	while (node = iter.next()) {
	    howmany += node->probs.numEntries();
	}

	return howmany;
    }
}

/*
 * Estimation
 */

/*
 * XXX: This function is essentially duplicated in several other places,
 * using different count types.  Propate changes to
 *
 *	VarNgram::estimate()
 *	SkipNgram::estimateMstep()
 */
Boolean
Ngram::estimate(NgramStats &stats, unsigned *mincounts, unsigned *maxcounts)
{
    /*
     * If no discount method was specified we do the default, standard
     * thing. Good Turing discounting with the specified min and max counts
     * for all orders.
     */
    Discount *discounts[order];
    unsigned i;
    Boolean error = false;

    for (i = 1; !error & i <= order; i++) {
	discounts[i-1] =
		new GoodTuring(mincounts ? mincounts[i-1] : GT_defaultMinCount,
			       maxcounts ? maxcounts[i-1] : GT_defaultMaxCount);
	/*
	 * Transfer the LMStats's debug level to the newly
	 * created discount objects
	 */
	discounts[i-1]->debugme(stats.debuglevel());

	if (!discounts[i-1]->estimate(stats, i)) {
	    cerr << "failed to estimate GT discount for order " << i + 1
		 << endl;
	    error = true;
	} else if (debug(DEBUG_PRINT_GTPARAMS)) {
		dout() << "Good Turing parameters for " << i << "-grams:\n";
		File errfile(stderr);
		discounts[i-1]->write(errfile);
	}
    }

    if (!error) {
	error = !estimate(stats, discounts);
    }

    for (i = 1; i <= order; i++) {
	delete discounts[i-1];
    }
    return !error;
}

Boolean
Ngram::estimate(NgramStats &stats, Discount **discounts)
{
    /*
     * For all ngrams, compute probabilities and apply the discount
     * coefficients.
     */
    VocabIndex context[order];

    /*
     * Ensure <s> unigram exists (being a non-event, it is not inserted
     * in distributeProb(), yet is assumed by much other software).
     */
    if (vocab.ssIndex != Vocab_None) {
	context[0] = Vocab_None;

	*insertProb(vocab.ssIndex, context) = LogP_Zero;
    }

    for (unsigned i = 1; i <= order; i++) {
	NgramCount *contextCount;
	NgramsIter contextIter(stats, context, i-1);
	unsigned noneventContexts = 0;
	unsigned noneventNgrams = 0;
	unsigned discountedNgrams = 0;

	/*
	 * This enumerates all contexts, i.e., i-1 grams.
	 */
	while (contextCount = contextIter.next()) {
	    /*
	     * Skip contexts ending in </s>.  This typically only occurs
	     * with the doubling of </s> to generate trigrams from
	     * bigrams ending in </s>.
	     * If <unk> is not real word, also skip context that contain
	     * it.
	     */
	    if (i > 1 && context[i-2] == vocab.seIndex ||
	        vocab.isNonEvent(vocab.unkIndex) &&
				 vocab.contains(context, vocab.unkIndex))
	    {
		noneventContexts ++;
		continue;
	    }

	    VocabIndex word[2];	/* the follow word */
	    NgramsIter followIter(stats, context, word, 1);
	    NgramCount *ngramCount;

	    /*
	     * Total up the counts for the denominator
	     * (the lower-order counts may not be consistent with
	     * the higher-order ones, so we can't just use *contextCount)
	     * Only if the trustTotal flag is set do we override this
	     * with the count from the context ngram.
	     */
	    NgramCount totalCount = 0;
	    Count observedVocab = 0;
	    while (ngramCount = followIter.next()) {
		if (vocab.isNonEvent(word[0]) ||
		    ngramCount == 0 ||
		    i == 1 && word[0] == dummyIndex)
		{
		    continue;
		}
		totalCount += *ngramCount;
		observedVocab ++;
	    }
	    if (i > 1 && trustTotals) {
		totalCount = *contextCount;
	    }

	    if (totalCount == 0) {
		continue;
	    }

	    /*
	     * reverse the context ngram since that's how
	     * the BO nodes are indexed.
	     */
	    Vocab::reverse(context);

	    /*
	     * Compute the discounted probabilities
	     * from the counts and store them in the backoff model.
	     */
	retry:
	    followIter.init();
	    Prob totalProb = 0.0;

	    /*
	     * check if discounting is disabled for this round
	     */
	    Boolean noDiscount =
			    (discounts == 0) ||
			    (discounts[i-1] == 0) ||
			    discounts[i-1]->nodiscount();

	    while (ngramCount = followIter.next()) {
		LogP lprob;
		double discount;

		/*
		 * Ignore zero counts.
		 * They are there just as an artifact of the count trie
		 * if a higher order ngram has a non-zero count.
		 */
		if (i > 1 && *ngramCount == 0) {
		    continue;
		}

		if (vocab.isNonEvent(word[0]) || word[0] == dummyIndex) {
		    /*
		     * Discard all pseudo-word probabilities,
		     * except for unigrams.  For unigrams, assign
		     * probability zero.  This will leave them with
		     * prob zero in all case, due to the backoff
		     * algorithm.
		     * Also discard the <unk> token entirely in closed
		     * vocab models, its presence would prevent OOV
		     * detection when the model is read back in.
		     */
		    if (i > 1 || word[0] == vocab.unkIndex || word[0] == dummyIndex) {
			noneventNgrams ++;
			continue;
		    }

		    lprob = LogP_Zero;
		    discount = 1.0;
		} else if (word[0] == dummyIndex) {
		    /*
		     * N-grams marked with dummy tag as the last word
		     * count toward the total for that context, but are
		     * just placeholders for backoff probability mass
		     */
		    discount = 0.0;
		} else {
		    /*
		     * Ths discount array passed may contain 0 elements
		     * to indicate no discounting at this order.
		     */
		    if (noDiscount) {
			discount = 1.0;
		    } else {
			discount =
			    discounts[i-1]->discount(*ngramCount, totalCount,
								observedVocab);
		    }
		    Prob prob = (discount * *ngramCount) / totalCount;
		    lprob = ProbToLogP(prob);
		    totalProb += prob;

		    if (discount != 0.0 && debug(DEBUG_ESTIMATES)) {
			dout() << "CONTEXT " << (vocab.use(), context)
			       << " WORD " << vocab.getWord(word[0])
			       << " NUMER " << *ngramCount
			       << " DENOM " << totalCount
			       << " DISCOUNT " << discount
			       << " LPROB " << lprob
			       << endl;
		    }
		}
		    
		/*
		 * A discount coefficient of zero indicates this ngram
		 * should be omitted entirely (presumably to save space).
		 */
		if (discount == 0.0) {
		    discountedNgrams ++;
		} else {
		    *insertProb(word[0], context) = lprob;
		} 
	    }

	    /*
	     * This is a hack credited to Doug Paul (by Roni Rosenfeld in
	     * his CMU tools).  It may happen that no probability mass
	     * is left after totalling all the explicit probs, typically
	     * because the discount coefficients were out of range and
	     * forced to 1.0.  To arrive at some non-zero backoff mass
	     * we try incrementing the denominator in the estimator by 1.
	     */
	    if (!noDiscount && totalCount > 0 &&
		totalProb > 1.0 - Prob_Epsilon)
	    {
		totalCount ++;

		if (debug(DEBUG_ESTIMATE_WARNINGS)) {
		    cerr << "warning: no backoff probability mass left for \""
			 << (vocab.use(), context)
			 << "\" -- incrementing denominator"
			 << endl;
		}
		goto retry;
	    }

	    /*
	     * Undo the reversal above so the iterator can continue correctly
	     */
	    Vocab::reverse(context);
	}

	if (debug(DEBUG_ESTIMATE_WARNINGS)) {
	    if (noneventContexts > 0) {
		dout() << "discarded " << noneventContexts << " "
		       << i << "-gram contexts containing pseudo-events\n";
	    }
	    if (noneventNgrams > 0) {
		dout() << "discarded " << noneventNgrams << " "
		       << i << "-gram probs predicting pseudo-events\n";
	    }
	    if (discountedNgrams > 0) {
		dout() << "discarded " << discountedNgrams << " "
		       << i << "-gram probs discounted to zero\n";
	    }
	}
    }

    /*
     * With all the probs in place, BOWs are obtained simply by the usual
     * normalization.
     */
    recomputeBOWs();
    fixupProbs();

    return true;
}

/*
 * same as above, but using fractional counts
 */
Boolean
Ngram::estimate(NgramCounts<FloatCount> &stats, Discount **discounts)
{
    /*
     * For all ngrams, compute probabilities and apply the discount
     * coefficients.
     */
    VocabIndex context[order];

    for (unsigned i = 1; i <= order; i++) {
	FloatCount *contextCount;
	NgramCountsIter<FloatCount> contextIter(stats, context, i-1);
	unsigned noneventContexts = 0;
	unsigned noneventNgrams = 0;
	unsigned discountedNgrams = 0;

	/*
	 * This enumerates all contexts, i.e., i-1 grams.
	 */
	while (contextCount = contextIter.next()) {
	    /*
	     * Skip contexts ending in </s>.  This typically only occurs
	     * with the doubling of </s> to generate trigrams from
	     * bigrams ending in </s>.
	     * If <unk> is not real word, also skip context that contain
	     * it.
	     */
	    if (i > 1 && context[i-2] == vocab.seIndex ||
	        vocab.isNonEvent(vocab.unkIndex) &&
				 vocab.contains(context, vocab.unkIndex))
	    {
		noneventContexts ++;
		continue;
	    }

	    VocabIndex word[2];	/* the follow word */
	    NgramCountsIter<FloatCount> followIter(stats, context, word, 1);
	    FloatCount *ngramCount;

	    /*
	     * Total up the counts for the denominator
	     * (the lower-order counts may not be consistent with
	     * the higher-order ones, so we can't just use *contextCount)
	     * Only if the trustTotal flag is set do we override this
	     * with the count from the context ngram.
	     */
	    FloatCount totalCount = 0;
	    Count observedVocab = 0;
	    while (ngramCount = followIter.next()) {
		if (vocab.isNonEvent(word[0])) {
		    continue;
		}
		totalCount += *ngramCount;
		observedVocab ++;
	    }
	    if (i > 1 && trustTotals) {
		totalCount = *contextCount;
	    }

	    if (totalCount == 0) {
		continue;
	    }

	    /*
	     * reverse the context ngram since that's how
	     * the BO nodes are indexed.
	     */
	    Vocab::reverse(context);

	    /*
	     * Compute the discounted probabilities
	     * from the counts and store them in the backoff model.
	     */
	retry:
	    followIter.init();
	    Prob totalProb = 0.0;

	    /*
	     * check if discounting is disabled for this round
	     */
	    Boolean noDiscount =
			    (discounts == 0) ||
			    (discounts[i-1] == 0) ||
			    discounts[i-1]->nodiscount();

	    while (ngramCount = followIter.next()) {
		LogP lprob;
		double discount;

		if (vocab.isNonEvent(word[0])) {
		    /*
		     * Discard all pseudo-word probabilities,
		     * except for unigrams.  For unigrams, assign
		     * probability zero.  This will leave them with
		     * prob zero in all case, due to the backoff
		     * algorithm.
		     * Also discard the <unk> token entirely in closed
		     * vocab models, its presence would prevent OOV
		     * detection when the model is read back in.
		     */
		    if (i > 1 || word[0] == vocab.unkIndex) {
			noneventNgrams ++;
			continue;
		    }

		    lprob = LogP_Zero;
		    discount = 1.0;
		} else {
		    /*
		     * Ths discount array passed may contain 0 elements
		     * to indicate no discounting at this order.
		     */
		    if (noDiscount) {
			discount = 1.0;
		    } else {
			discount =
			    discounts[i-1]->discount(*ngramCount, totalCount,
								observedVocab);
		    }
		    Prob prob = (discount * *ngramCount) / totalCount;
		    lprob = ProbToLogP(prob);
		    totalProb += prob;
		}
		    
		/*
		 * A discount coefficient of zero indicates this ngram
		 * should be omitted entirely (presumably to save space).
		 * Unlike in Ngram::estimate(), we actually need to 
		 * remove the prob entry since it might have been
		 * created by a previous iteration.
		 */
		if (discount == 0.0) {
		    discountedNgrams ++;
		    removeProb(word[0], context);
		} else {
		    *insertProb(word[0], context) = lprob;
		} 
	    }

	    /*
	     * This is a hack credited to Doug Paul (by Roni Rosenfeld in
	     * his CMU tools).  It may happen that no probability mass
	     * is left after totalling all the explicit probs, typically
	     * because the discount coefficients were out of range and
	     * forced to 1.0.  To arrive at some non-zero backoff mass
	     * we try incrementing the denominator in the estimator by 1.
	     */
	    if (!noDiscount && totalCount > 0 &&
		totalProb > 1.0 - Prob_Epsilon)
	    {
		totalCount ++;

		if (debug(DEBUG_ESTIMATE_WARNINGS)) {
		    cerr << "warning: no backoff probability mass left for \""
			 << (vocab.use(), context)
			 << "\" -- incrementing denominator"
			 << endl;
		}
		goto retry;
	    }

	    /*
	     * Undo the reversal above so the iterator can continue correctly
	     */
	    Vocab::reverse(context);
	}

	if (debug(DEBUG_ESTIMATE_WARNINGS)) {
	    if (noneventContexts > 0) {
		dout() << "discarded " << noneventContexts << " "
		       << i << "-gram contexts containing pseudo-events\n";
	    }
	    if (noneventNgrams > 0) {
		dout() << "discarded " << noneventNgrams << " "
		       << i << "-gram probs predicting pseudo-events\n";
	    }
	    if (discountedNgrams > 0) {
		dout() << "discarded " << discountedNgrams << " "
		       << i << "-gram probs discounted to zero\n";
	    }
	}
    }

    /*
     * With all the probs in place, BOWs are obtained simply by the usual
     * normalization.
     */
    recomputeBOWs();
    fixupProbs();

    return true;
}

/*
 * Mixing backoff models
 */
void
Ngram::mixProbs(Ngram &lm1, Ngram &lm2, double lambda)
{
    VocabIndex context[order + 1];

    for (unsigned i = 0; i < order; i++) {
	BOnode *node;
	NgramBOsIter iter1(lm1, context, i);
	
	/*
	 * First, find all explicit ngram probs in lm1, and mix them
	 * with the corresponding probs of lm2 (explicit or backed-off).
	 */
	while (node = iter1.next()) {
	    NgramProbsIter piter(*node);
	    VocabIndex word;
	    LogP *prob1;

	    while (prob1 = piter.next(word)) {
		*insertProb(word, context) =
			MixLogP(*prob1, lm2.wordProb(word, context), lambda);
	    }
	}

	/*
	 * Do the same for lm2, except we dont't need to recompute 
	 * those cases that were already handled above (explicit probs
	 * in both lm1 and lm2).
	 */
	NgramBOsIter iter2(lm2, context, i);

	while (node = iter2.next()) {
	    NgramProbsIter piter(*node);
	    VocabIndex word;
	    LogP *prob2;

	    while (prob2 = piter.next(word)) {
		if (!lm1.findProb(word, context)) {
		    *insertProb(word, context) =
			MixLogP(lm1.wordProb(word, context), *prob2, lambda);
		}
	    }
	}
    }

    recomputeBOWs();
}

/*
 * Compute the numerator and denominator of a backoff weight,
 * checking for sanity.  Returns true if values make sense,
 * and prints a warning if not.
 */
Boolean
Ngram::computeBOW(BOnode *node, const VocabIndex *context, unsigned clen,
				Prob &numerator, Prob &denominator)
{
    NgramProbsIter piter(*node);
    VocabIndex word;
    LogP *prob;

    /*
     * The BOW(c) for a context c is computed to be
     *
     *	BOW(c) = (1 - Sum p(x | c)) /  (1 - Sum p_BO(x | c))
     *
     * where Sum is a summation over all words x with explicit probabilities
     * in context c, p(x|c) is that probability, and p_BO(x|c) is the 
     * probability for that word according to the backoff algorithm.
     */
    numerator = 1.0;
    denominator = 1.0;

    while (prob = piter.next(word)) {
	numerator -= LogPtoProb(*prob);
	if (clen > 0) {
	    denominator -=
		LogPtoProb(wordProbBO(word, context, clen - 1));
	}
    }

    /*
     * Avoid some predictable anomalies due to rounding errors
     */
    if (numerator < 0.0 && numerator > -Prob_Epsilon) {
	numerator = 0.0;
    }
    if (denominator < 0.0 && denominator > -Prob_Epsilon) {
	denominator = 0.0;
    }

    if (numerator < 0.0) {
	cerr << "BOW numerator for context \""
	     << (vocab.use(), context)
	     << "\" is " << numerator << " < 0\n";
	return false;
    } else if (denominator <= 0.0) {
	if (numerator > Prob_Epsilon) {
	    cerr << "BOW denominator for context \""
		 << (vocab.use(), context)
		 << "\" is " << denominator << " <= 0,"
		 << "numerator is " << numerator
		 << endl;
	    return false;
	} else {
	    numerator = 0.0;
	    denominator = 0.0;	/* will give bow = 0 */
	    return true;
	}
    } else {
	return true;
    }
}

/*
 * Renormalize language model by recomputing backoff weights.
 */
void
Ngram::recomputeBOWs()
{
    VocabIndex context[order + 1];

    /*
     * Here it is important that we compute the backoff weights in
     * increasing order, since the higher-order ones refer to the
     * lower-order ones in the backoff algorithm.
     * Note that this will only generate backoff nodes for those
     * contexts that have words with explicit probabilities.  But
     * this is precisely as it should be.
     */
    for (unsigned i = 0; i < order; i++) {
	BOnode *node;
	NgramBOsIter iter1(*this, context, i);
	
	while (node = iter1.next()) {
	    NgramProbsIter piter(*node);

	    double numerator, denominator;

	    if (computeBOW(node, context, i, numerator, denominator)) {
		/*
		 * If unigram probs leave a non-zero probability mass
		 * then we should give that mass to the zero-order (uniform)
		 * distribution for zeroton words.  However, the ARPA
		 * BO format doesn't support a "null context" BOW.
		 * We simluate the intended distribution by spreading the
		 * left-over mass uniformly over all vocabulary items that
		 * have a zero probability.
		 * NOTE: We used to do this only if there was prob mass left,
		 * but some ngram software requires all words to appear as
		 * unigrams, which we achieve by giving them zero probability.
		 */
		if (i == 0 /*&& numerator > 0.0*/) {
		    distributeProb(numerator, context);
		} else if (numerator == 0.0 && denominator == 0) {
		    node->bow = LogP_One;
		} else {
		    node->bow = ProbToLogP(numerator) - ProbToLogP(denominator);
		}
	    } else {
		/*
		 * Dummy value for improper models
		 */
		node->bow = LogP_Zero;
	    }
	}
    }
}

/*
 * Compute the marginal probability of an N-gram context
 *	(This is very similar to sentenceProb(), but doesn't assume sentence
 *	start/end and treat the <s> context specially.)
 */
LogP
Ngram::computeContextProb(const VocabIndex *context)
{
    unsigned clen = Vocab::length(context);

    LogP prob = LogP_One;

    for (unsigned i = clen; i > 0; i --) {
	VocabIndex word = context[i - 1];

	/*
	 * If we're computing the marginal probability of the unigram
	 * <s> context we have to lookup </s> instead since the former
	 * has prob = 0 in an N-gram model.
	 */
	if (i == clen && word == vocab.ssIndex) {
	    word = vocab.seIndex;
	}

	prob += wordProbBO(word, &context[i], clen - i);
    }

    return prob;
}

/*
 * Prune probabilities from model so that the change in training set perplexity
 * is below a threshold.
 * The minorder parameter limits pruning to ngrams of that length and above.
 */
void
Ngram::pruneProbs(double threshold, unsigned minorder)
{
    /*
     * Hack alert: allocate the context buffer for NgramBOsIter, but leave
     * room for a word id to be prepended.
     */
    VocabIndex wordPlusContext[order + 2];
    VocabIndex *context = &wordPlusContext[1];

    for (unsigned i = order - 1; i > 0 && i >= minorder - 1; i--) {
	unsigned prunedNgrams = 0;

	BOnode *node;
	NgramBOsIter iter1(*this, context, i);
	
	while (node = iter1.next()) {
	    LogP bow = node->bow;	/* old backoff weight, BOW(h) */
	    double numerator, denominator;

	    /* 
	     * Compute numerator and denominator of the backoff weight,
	     * so that we can quickly compute the BOW adjustment due to
	     * leaving out one prob.
	     */
	    if (!computeBOW(node, context, i, numerator, denominator)) {
		continue;
	    }

	    /*
	     * Compute the marginal probability of the context, P(h)
	     */
	    LogP contextProb = computeContextProb(context);

	    NgramProbsIter piter(*node);
	    VocabIndex word;
	    LogP *ngramProb;

	    Boolean allPruned = true;

	    while (ngramProb = piter.next(word)) {
		/*
		 * lower-order estimate for ngramProb, P(w|h')
		 */
		LogP backoffProb = wordProbBO(word, context, i - 1);

		/*
		 * Compute BOW after removing ngram, BOW'(h)
		 */
		LogP newBOW =
			ProbToLogP(numerator + LogPtoProb(*ngramProb)) -
			ProbToLogP(denominator + LogPtoProb(backoffProb));

		/*
		 * Compute change in entropy due to removal of ngram
		 * deltaH = - P(H) x
		 *  {P(W | H) [log P(w|h') + log BOW'(h) - log P(w|h)] +
		 *   (1 - \sum_{v,h ngrams} P(v|h)) [log BOW'(h) - log BOW(h)]}
		 *
		 * (1-\sum_{v,h ngrams}) is the probability mass left over from
		 * ngrams of the current order, and is the same as the 
		 * numerator in BOW(h).
		 */
		LogP deltaProb = backoffProb + newBOW - *ngramProb;
		Prob deltaEntropy = - LogPtoProb(contextProb) *
					(LogPtoProb(*ngramProb) * deltaProb +
					 numerator * (newBOW - bow));

		/*
		 * compute relative change in model (training set) perplexity
		 *	(PPL' - PPL)/PPL = PPL'/PPL - 1
		 *	                 = exp(H')/exp(H) - 1
		 *	                 = exp(H' - H) - 1
		 */
		double perpChange = LogPtoProb(deltaEntropy) - 1.0;

		Boolean pruned = threshold > 0 && perpChange < threshold;

		/*
		 * Make sure we don't prune ngrams whose backoff nodes are
		 * needed ...
		 */
		if (pruned) {
		    /*
		     * wordPlusContext[1 ... i-1] was already filled in by
		     * NgramBOIter. Just add the word to complete the ngram.
		     */
		    wordPlusContext[0] = word;

		    if (findBOW(wordPlusContext)) {
			pruned = false;
		    }
		}

		if (debug(DEBUG_ESTIMATES)) {
		    dout() << "CONTEXT " << (vocab.use(), context)
			   << " WORD " << vocab.getWord(word)
			   << " CONTEXTPROB " << contextProb
			   << " OLDPROB " << *ngramProb
			   << " NEWPROB " << (backoffProb + newBOW)
			   << " DELTA-H " << deltaEntropy
			   << " DELTA-LOGP " << deltaProb
			   << " PPL-CHANGE " << perpChange
			   << " PRUNED " << pruned
			   << endl;
		}

		if (pruned) {
		    removeProb(word, context);
		    prunedNgrams ++;
		} else {
		    allPruned = false;
		}
	    }

	    /*
	     * If we removed all ngrams for this context we can 
	     * remove the context itself.
	     * Note: Due to the check above this also means there
	     * are no contexts that extend the current one, so
	     * removing this node won't leave any children orphaned.
	     */
	    if (allPruned) {
		removeBOW(context);
	    }
	}

	if (debug(DEBUG_ESTIMATE_WARNINGS)) {
	    if (prunedNgrams > 0) {
		dout() << "pruned " << prunedNgrams << " "
		       << (i + 1) << "-grams\n";
	    }
	}
    }

    recomputeBOWs();
}

/*
 * Prune low probability N-grams
 *	Removes all N-gram probabilities that are lower than the
 *	corresponding backed-off estimates.  This is a crucial preprocessing
 *	step for N-gram models before conversion to finite-state networks.
 */
void
Ngram::pruneLowProbs(unsigned minorder)
{
    VocabIndex context[order];

    Boolean havePruned;
    
    /*
     * Since pruning changes backoff weights we have to keep pruning
     * interatively as long as N-grams keep being removed.
     */
    do {
	havePruned = false;

	for (unsigned i = minorder - 1; i < order; i++) {
	    unsigned prunedNgrams = 0;

	    BOnode *node;
	    NgramBOsIter iter1(*this, context, i);
	    
	    while (node = iter1.next()) {
		LogP bow = node->bow;	/* old backoff weight, BOW(h) */

		NgramProbsIter piter(*node);
		VocabIndex word;
		LogP *ngramProb;

		while (ngramProb = piter.next(word)) {
		    /*
		     * lower-order estimate for ngramProb, P(w|h')
		     */
		    LogP backoffProb = wordProbBO(word, context, i - 1);

		    if (backoffProb + bow > *ngramProb) {

			if (debug(DEBUG_ESTIMATES)) {
			    dout() << "CONTEXT " << (vocab.use(), context)
				   << " WORD " << vocab.getWord(word)
				   << " LPROB " << *ngramProb
				   << " BACKOFF-LPROB " << (backoffProb + bow)
				   << " PRUNED\n";
			}

			removeProb(word, context);
			prunedNgrams ++;
		    }
		}
	    }

	    if (prunedNgrams > 0) {
		havePruned = true;
		if (debug(DEBUG_ESTIMATE_WARNINGS)) {
		    dout() << "pruned " << prunedNgrams << " "
			   << (i + 1) << "-grams\n";
		}
	    }
	}
	recomputeBOWs();

    } while (havePruned);

    fixupProbs();
}

/*
 * Insert probabilities for all context ngrams
 *
 * 	The ARPA format forces us to have a probability
 * 	for every context ngram.  So we create one if necessary and
 * 	set the probability to the same as what the backoff algorithm
 * 	would compute (so as not to change the distribution).
 */
void
Ngram::fixupProbs()
{
    VocabIndex context[order + 1];

    /*
     * we cannot insert entries into the context trie while we're
     * iterating over it, so we need another trie to collect
     * the affected contexts first. yuck.
     * Note: We use the same trie type as in the Ngram model itself,
     * so we don't need to worry about another template instantiation.
     */
    Trie<VocabIndex,NgramCount> contextsToAdd;

    /*
     * Find the contexts xyz that don't have a probability p(z|xy) in the
     * model already.
     */
    unsigned i;
    for (i = 1; i < order; i++) {
	NgramBOsIter iter(*this, context, i);
	
	while (iter.next()) {
	    /*
	     * For a context abcd we need to create probability entries
	     * p(d|abc), p(c|ab), p(b|a), p(a) if necessary.
	     * If any of these is found in that order we can stop since a
	     * previous pass will already have created the remaining ones.
	     */
	    for (unsigned j = 0; j < i; j++) {
		LogP *prob = findProb(context[j], &context[j+1]);

		if (prob) {
		    break;
		} else {
		    /*
		     * Just store something non-zero to distinguish from
		     * internal nodes that are added implicitly.
		     */
		    *contextsToAdd.insert(&context[j]) = 1;
		}
	    }
	}
    }


    /*
     * Store the missing probs
     */
    for (i = 1; i < order; i++) {
	unsigned numFakes = 0;

	Trie<VocabIndex,NgramCount> *node;
	TrieIter2<VocabIndex,NgramCount> iter(contextsToAdd, context, i);
	

	while (node = iter.next()) {
	    if (node->value()) {
		numFakes ++;

		/*
		 * Note: we cannot combine the two statements below
		 * since insertProb() creates a zero prob entry, which would
		 * prevent wordProbBO() from computing the backed-off 
		 * estimate!
		 */
		LogP backoffProb = wordProbBO(context[0], &context[1], i - 1);
		*insertProb(context[0], &(context[1])) = backoffProb;

		if (debug(DEBUG_FIXUP_WARNINGS)) {
		    dout() << "faking probability for context "
			   << (vocab.use(), context) << endl;
		}
	    }
	}

	if (debug(DEBUG_ESTIMATE_WARNINGS)) {
	    if (numFakes > 0) {
		dout() << "inserted " << numFakes << " redundant " 
		       << i << "-gram probs\n";
	    }
	}
    }
}

/*
 * Redistribute probability mass over ngrams of given context
 */
void
Ngram::distributeProb(Prob mass, VocabIndex *context)
{
    /*
     * First enumerate the vocabulary to count the number of
     * items affected
     */
    unsigned numWords = 0;
    unsigned numZeroProbs = 0;
    VocabIter viter(vocab);
    VocabIndex word;

    while (viter.next(word)) {
	if (!vocab.isNonEvent(word) && word != dummyIndex) {
	    numWords ++;

	    LogP *prob = findProb(word, context);
	    if (!prob || *prob == LogP_Zero) {
		numZeroProbs ++;
	    }
	    /*
	     * create zero probs so we can update them below
	     */
	    if (!prob) {
		*insertProb(word, context) = LogP_Zero;
	    }
	}
    }

    /*
     * If there are no zero-probability words then
     * we add the left-over prob mass to all unigrams.
     * Otherwise do as described above.
     */
    viter.init();

    if (numZeroProbs > 0) {
	if (debug(DEBUG_ESTIMATE_WARNINGS)) {
	    cerr << "warning: distributing " << mass
		 << " left-over probability mass over "
		 << numZeroProbs << " zeroton words" << endl;
	}
	Prob add = mass / numZeroProbs;

	while (viter.next(word)) {
	    if (!vocab.isNonEvent(word) && word != dummyIndex) {
		LogP *prob = insertProb(word, context);
		if (*prob == LogP_Zero) {
		    *prob = ProbToLogP(add);
		}
	    }
	}
    } else {
	if (mass > 0.0 && debug(DEBUG_ESTIMATE_WARNINGS)) {
	    cerr << "warning: distributing " << mass
		 << " left-over probability mass over all "
		 << numWords << " words" << endl;
	}
	Prob add = mass / numWords;

	while (viter.next(word)) {
	    if (!vocab.isNonEvent(word) && word != dummyIndex) {
		LogP *prob = insertProb(word, context);
		*prob = ProbToLogP(LogPtoProb(*prob) + add);
	    }
	}
    }
}

