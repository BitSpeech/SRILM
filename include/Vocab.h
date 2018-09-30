/*
 * Vocab.h --
 *	Interface to the Vocab class.
 *
 *
 * SYNOPSIS
 *
 * Vocab represents sets of string tokens as typically used for vocabularies,
 * word class names, etc.  Additionally, Vocab provides a mapping from
 * such string tokens (type VocabString) to integers (type VocabIndex).
 * VocabIndex is typically used to index words in language models to
 * conserve space and speed up comparisons etc.  Thus, Vocab essentially
 * implements a symbol table into which strings can be "interned."
 *
 * INTERFACE
 *
 * VocabIndex(VocabIndex start, VocabIndex end)
 *	Initializes a Vocab and sets the index range.
 *	Indices are allocated starting at start and incremented from there.
 *	No indices are allowed beyond end.
 *	This provides a way to map several distinct Vocabs to disjoint
 *	ranges of integers, and then use them jointly without danger of
 *	confusion.
 *
 * VocabIndex addWord(VocabString token)
 *	Adds a new string to the Vocab, returning the assigned index,
 *	or looks up the index if the token already exists.
 *
 * VocabString getWord(VocabIndex index)
 *	Returns the string token associated with index, or 0 if it none
 *	exists.
 *
 * VocabIndex getIndex(VocabString token)
 *	Returns the index for a string token, or Vocab_None if none exists.
 *
 * void remove(VocabString token)
 * void remove(VocabIndex index)
 *	Deletes an item from the Vocab, either by token or by index.
 *
 * unsigned int numWords()
 *	Returns the number of tokens (and indices) in the Vocab.
 *
 * VocabIndex highIndex()
 *	Returns the highest word index in use, or Vocab_None if 
 *	vocabulary is empty.
 *
 * ITERATION
 *
 * VocabIter implements iterations over Vocabs. 
 *
 * VocabIter(Vocab &vocab)
 *	Creates and initializes an iteration over vocab.
 *
 * void init()
 *	Reset an iteration to the "first" element.
 *
 * VocabString next()
 * VocabString next(VocabIndex &index)
 *	Returns the next Vocab token in an iteration, or 0 if the
 *	iteration is finished.  index is set to the corresponding
 *	index.
 *
 * unsigned int read(File &file)
 *	Read a word list from a file into the Vocab, implicitly performing
 *	an addWord() on each token read.  Returns the number of tokens read.
 *
 * void write(File &file)
 *	Write the current set of word tokes to a file, in random order.
 *
 * NOTE: While an iteration over a Vocab is ongoing, no modifications
 * are allowed to the Vocab, EXCEPT possibly removal of the
 * "current" token/index.
 *
 * An iteration returns the elements of a Vocab in random, but deterministic
 * order. Furthermore, when copied or used in initialization of other objects,
 * VocabIter objects retain the current "position" in an iteration.  This
 * allows nested iterations that enumerate all pairs of distinct elements.
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/Vocab.h,v 1.22 1997/07/18 02:13:12 stolcke Exp $
 *
 */

#ifndef _Vocab_h_
#define _Vocab_h_

#include <iostream.h>

#include "Boolean.h"
#include "File.h"
#include "LHash.h"
#include "Array.h"
#include "MemStats.h"

typedef unsigned int	VocabIndex;
/* typedef unsigned short	VocabIndex; */
typedef const char	*VocabString;

const unsigned int	maxWordLength = 256;

const VocabIndex	Vocab_None = (VocabIndex)-1;

const VocabString	Vocab_Unknown = "<unk>";
const VocabString	Vocab_SentStart = "<s>";
const VocabString	Vocab_SentEnd = "</s>";
const VocabString	Vocab_Pause = "-pau-";

typedef int (*VocabIndexComparator)(VocabIndex, VocabIndex);
typedef int (*VocabIndicesComparator)(const VocabIndex *, const VocabIndex *);

class Vocab
{
    friend class VocabIter;

public:
    Vocab(VocabIndex start = 0, VocabIndex end = 0x7fffffff);
    virtual ~Vocab() {};

    virtual VocabIndex addWord(VocabString name);
    virtual VocabString getWord(VocabIndex index) const;
    virtual VocabIndex getIndex(VocabString name,
				    VocabIndex unkIndex = Vocab_None) const;
    virtual void remove(VocabString name);
    virtual void remove(VocabIndex index);
    virtual unsigned int numWords() const { return byName.numEntries(); };
    virtual VocabIndex highIndex() const;

    /*
     * Special (pseudo-) vocabulary tokens
     */
    VocabIndex unkIndex;		/* <unk> index */
    VocabIndex ssIndex;			/* <s> index */
    VocabIndex seIndex;			/* </s> index */
    VocabIndex pauseIndex;		/* -pau- index */

    Boolean unkIsWord;			/* consider <unk> a regular word */

    Boolean toLower;			/* map word strings to lowercase */

    /*
     * Some Vocab tokens/indices are "pseudo words", i.e., they don't
     * get probabilities since they can only occur in contexts.
     */
    virtual Boolean isNonEvent(VocabString word) const	/* pseudo-word? */
	{ return isNonEvent(getIndex(word)); };
    virtual Boolean isNonEvent(VocabIndex word) const	/* non-event? */
	{ return (word == ssIndex) || 
		 (word == pauseIndex) ||
		 !unkIsWord && (word == unkIndex); };

    /*
     * Utilities for handling Vocab sequences
     */
    virtual unsigned int getWords(const VocabIndex *wids,
			  VocabString *words, unsigned int max) const;
    virtual unsigned int addWords(const VocabString *words,
			  VocabIndex *wids, unsigned int max);
    virtual unsigned int getIndices(const VocabString *words,
			    VocabIndex *wids, unsigned int max,
			    VocabIndex unkIndex = Vocab_None) const;
    static unsigned int parseWords(char *line,
				   VocabString *words, unsigned int max);

    static unsigned int length(const VocabIndex *words);
    static unsigned int length(const VocabString *words);
    static Boolean contains(const VocabIndex *words, VocabIndex word);
    static VocabIndex *reverse(VocabIndex *words);
    static VocabString *reverse(VocabString *words);
    static void write(File &file, const VocabString *words);

    /*
     *  Comparison of Vocabs and their sequences
     */
    static int compare(VocabIndex word1, VocabIndex word2);
				/* order on word indices induced by Vocab */
    static int compare(VocabString word1, VocabString word2)
	{ return strcmp(word1, word2); };
    static int compare(const VocabIndex *word1, const VocabIndex *word2);
				/* order on word index sequences */
    static int compare(const VocabString *word1, const VocabString *word2);

    VocabIndexComparator compareIndex() const;
    VocabIndicesComparator compareIndices() const;

    /*
     * Miscellaneous
     */
    virtual unsigned int read(File &file);
    virtual void write(File &file, Boolean sorted = true) const;
    virtual void use() const { outputVocab = (Vocab *)this; }; // discard const

    virtual void memStats(MemStats &stats) const;

    static Vocab *outputVocab;  /* implicit parameter to operator<< */

protected:
    LHash<VocabString,VocabIndex> byName;
    Array<VocabString> byIndex;
    VocabIndex nextIndex;
    VocabIndex maxIndex;

    static Vocab *compareVocab;	/* implicit parameter to compare() */
};

ostream &operator<< (ostream &, const VocabString *words);
ostream &operator<< (ostream &, const VocabIndex *words);

class VocabIter
{
public:
    VocabIter(const Vocab &vocab, Boolean sorted = false);
    void init();
    VocabString next() { VocabIndex index; return next(index); };
    VocabString next(VocabIndex &index);
private:
    LHashIter<VocabString,VocabIndex> myIter;
};

#endif /* _Vocab_h_ */
