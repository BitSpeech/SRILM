/*
 * Trie.h --
 *	Trie structures.
 *
 * Trie<KeyT,DataT> is a template container class that implements a mapping
 * from sequences of _keys_ (type KeyT *) to data items or _values_
 * (type DataT).  It is built as an extension of Map and has a similar
 * interface.
 * Most functions accept either a single KeyT (a sequence of length 1),
 * or variable-length vector of KeyT elements.  In the latter case,
 * the KeyT * argument is terminated by a key element for which
 * Map_noKeyP() holds true.  For convenience, the KeyT * sequence can be
 * the null pointer to refer to the current Trie node.  This is also the default
 * argument value.  For example, find() will return a pointer to the data
 * stored at the current node.
 *
 * DataT *find(const KeyT *keys, Boolean &foundP)
 * DataT *find(KeyT key, Boolean &foundP)
 *	Returns a pointer to the data item found under key, or null if
 *	the key is not in the trie.
 *	With this and the other functions, the foundP argument is optional
 *	and returns whether the key was found.
 *
 * DataT *insert(const KeyT *keys, Boolean &foundP)
 * DataT *insert(KeyT key, Boolean &foundP)
 *	Returns a pointer to the data item for key, creating a new
 *	entry if necessary (indicated by foundP == false).
 *	New data items are zero-initialized.
 *
 * DataT *remove(const KeyT *keys, Boolean &foundP)
 * DataT *remove(KeyT key, Boolean &foundP)
 *	Deletes the entry associated with key from the trie, returning
 *	a pointer to the previously stored value, if any.
 *
 * Trie *findTrie(const KeyT *keys, Boolean &foundP)
 * Trie *findTrie(KeyT key, Boolean &foundP)
 *	Returns a pointer to the trie node found under key, or null if
 *	the key is no in the trie.
 *
 * Trie *insertTrie(const KeyT *keys, Boolean &foundP)
 * Trie *insertTrie(KeyT key, Boolean &foundP)
 *	Returns a pointer to the trie node found for key, creating a new
 *	node if necessary (indicated by foundP == false).
 *	The data for the new node is zero-initialized, and has no
 *	child nodes.
 *
 * voild clear()
 *	Removes all entries.
 *
 * unsigned int numEntries()
 *	Returns the current number of keys (i.e., entries) in the trie.
 *
 * DataT &value()
 *	Return the current data item (by reference, not pointer!).
 *
 * The DataT * pointers returned by find(), insert() and remove() are
 * valid only until the next operation on the Trie object.  It is left
 * to the user to assign actual values by dereferencing the pointers 
 * returned.  The main benefit is that only one key lookup is needed
 * for a find-and-change operation.
 *
 * TrieIter<KeyT,DataT> provids iterators over the child nodes of one
 * Trie node.
 *
 * TrieIter(Trie<KeyT,DataT> &trie)
 *	Creates and initializes an iteration over trie.
 *
 * void init()
 *	Reset an interation to the first element.
 *
 * Trie<KeyT,DataT> *next(KeyT &key)
 *	Steps the iteration and returns a pointer to the next subtrie,
 *	or null if the iteration is finished.  key is set to the associated
 *	Key value.
 *
 * Note that the iterator returns pointers to the Trie nodes, not the
 * stored data items.  Those can be accessed with value(), see above.
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/dstruct/src/RCS/Trie.h,v 1.20 2009/08/31 20:10:10 stolcke Exp $
 *
 */

#ifndef _Trie_h_
#define _Trie_h_

#ifdef PRE_ISO_CXX
# include <new.h>
#else
# include <new>
#endif

#include "MemStats.h"

#undef TRIE_INDEX_T
#undef TRIE_ITER_T

#ifdef USE_SARRAY_TRIE

# define TRIE_INDEX_T	SArray
# define TRIE_ITER_T	SArrayIter
# include "SArray.h"

#else /* ! USE_SARRAY_TRIE */

# define TRIE_INDEX_T	LHash
# define TRIE_ITER_T	LHashIter
# include "LHash.h"

#endif /* USE_SARRAY_TRIE */

template <class KeyT, class DataT> class TrieIter;	// forward declaration
template <class KeyT, class DataT> class TrieIter2;	// forward declaration

template <class KeyT, class DataT>
class Trie
{
    friend class TrieIter<KeyT,DataT>;
    friend class TrieIter2<KeyT,DataT>;
public:
    Trie(unsigned size = 0);
    ~Trie();

    DataT &value() { return data; };

    DataT *find(const KeyT *keys = 0, Boolean &foundP = _Map::foundP) const
	{ Trie<KeyT,DataT> *node = findTrie(keys, foundP);
	  return node ? &(node->data) : 0; };
    DataT *find(KeyT key, Boolean &foundP = _Map::foundP) const
	{ Trie<KeyT,DataT> *node = findTrie(key, foundP);
	  return node ? &(node->data) : 0; };

    DataT *insert(const KeyT *keys = 0, Boolean &foundP = _Map::foundP)
	{ return &(insertTrie(keys, foundP)->data); };
    DataT *insert(KeyT key, Boolean &foundP = _Map::foundP)
	{ return &(insertTrie(key, foundP)->data); };

    DataT *remove(const KeyT *keys = 0, Boolean &foundP = _Map::foundP)
	{ Trie<KeyT,DataT> *node = removeTrie(keys, foundP);
	  return node ? &(node->data) : 0; };
    DataT *remove(KeyT key, Boolean &foundP = _Map::foundP)
	{ Trie<KeyT,DataT> *node = removeTrie(key, foundP);
	  return node ? &(node->data) : 0; };

    Trie<KeyT,DataT> *findTrie(const KeyT *keys = 0,
				Boolean &foundP = _Map::foundP) const;
    Trie<KeyT,DataT> *findTrie(KeyT key, Boolean &foundP = _Map::foundP) const
	{ return sub.find(key, foundP); };

    Trie<KeyT,DataT> *insertTrie(const KeyT *keys = 0,
				Boolean &foundP = _Map::foundP);
    Trie<KeyT,DataT> *insertTrie(KeyT key, Boolean &foundP = _Map::foundP)
	{ Trie<KeyT,DataT> *subtrie = sub.insert(key, foundP);
          if (!foundP) new (&subtrie->sub) KeyT(0);
	  return subtrie; }

    Trie<KeyT,DataT> *removeTrie(const KeyT *keys = 0,
				Boolean &foundP = _Map::foundP);
    Trie<KeyT,DataT> *removeTrie(KeyT key, Boolean &foundP = _Map::foundP)
	{ KeyT keys[2]; keys[0] = key, Map_noKey(keys[1]);
	  return removeTrie(keys, foundP); };

    void clear() { sub.clear(0); };

    unsigned int numEntries() const { return sub.numEntries(); };

    void dump() const;				/* debugging: dump contents */
    void memStats(MemStats &stats) const;	/* compute memory stats */

private:
    TRIE_INDEX_T< KeyT, Trie<KeyT,DataT> > sub
#ifdef USE_PACKED_TRIE
						__attribute__ ((packed))
#endif
    					      ;	/* LHash of child nodes */
    DataT data;					/* data stored at this node */
};

/*
 * Iteration over immediate child nodes
 */
template <class KeyT, class DataT>
class TrieIter
{
public:
    TrieIter(const Trie<KeyT,DataT> &trie, int (*sort)(KeyT,KeyT) = 0)
	: myIter(trie.sub, sort) {};

    void init() { myIter.init(); } ;
    Trie<KeyT,DataT> *next(KeyT &key) { return myIter.next(key); };

private:
    TRIE_ITER_T< KeyT, Trie<KeyT,DataT> > myIter;
};

/*
 * Iteration over all nodes at a given depth in the trie
 */
template <class KeyT, class DataT>
class TrieIter2
{
public:
    TrieIter2(const Trie<KeyT,DataT> &trie, KeyT *keys, unsigned int level,
						int (*sort)(KeyT,KeyT) = 0);
    ~TrieIter2();

    void init();			/* re-initialize */
    Trie<KeyT,DataT> *next();		/* next element -- sets keys */

private:
    const Trie<KeyT,DataT> &myTrie;	/* Node being iterated over */
    KeyT *keys;				/* array passed to hold keys */
    int level;				/* depth of nodes enumerated */
    int (*sort)(KeyT,KeyT);		/* key comparison function for sort */
    TRIE_ITER_T< KeyT, Trie<KeyT,DataT> > myIter;
					/* iterator over the immediate
					 * child nodes */
    TrieIter2<KeyT,DataT> *subIter;	/* recursive iteration over 
					 * children's children etc. */
    Boolean done;			/* flag for level=0 iterator */
};

#endif /* _Trie_h_ */
