/*
 * BTrie.h --
 *	Trie structure with separation of internal and terminal nodes.
 *
 * See Trie.h for interface.
 *
 * Copyright (c) 1997, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/spot71/srilm/devel/dstruct/src/RCS/Trie.h,v 1.13 1996/12/08 09:09:54 stolcke Exp $
 *
 */

#ifndef _BTrie_h_
#define _BTrie_h_

#include <new.h>

#include "MemStats.h"

#undef BTRIE_INDEX_T
#undef BTRIE_ITER_T

#ifdef USE_SARRAY_TRIE

# define BTRIE_INDEX_T	SArray
# define BTRIE_ITER_T	SArrayIter
# include "SArray.h"

#else /* ! USE_SARRAY_TRIE */

# define BTRIE_INDEX_T	LHash
# define BTRIE_ITER_T	LHashIter
# include "LHash.h"

#endif /* USE_SARRAY_TRIE */

template <class KeyT, class DataT> class BTrieIter;	// forward declaration
template <class KeyT, class DataT> class BTrieIter2;	// forward declaration

template <class KeyT, class DataT>
class BTrie
{
    friend class BTrieIter<KeyT,DataT>;
    friend class BTrieIter2<KeyT,DataT>;
public:
    BTrie();
    ~BTrie();

    DataT &value() { return data; };

    DataT *find(const KeyT *keys = 0, Boolean &foundP = _Map::foundP) const
	{ BTrie<KeyT,DataT> *node = findTrie(keys, foundP);
	  return node ? &(node->data) : 0; };
    DataT *find(KeyT key, Boolean &foundP = _Map::foundP) const
	{ BTrie<KeyT,DataT> *node = findTrie(key, foundP);
	  return node ? &(node->data) : 0; };

    DataT *insert(const KeyT *keys = 0, Boolean &foundP = _Map::foundP)
	{ return &(insertTrie(keys, foundP)->data); };
    DataT *insert(KeyT key, Boolean &foundP = _Map::foundP)
	{ return &(insertTrie(key, foundP)->data); };

    DataT *remove(const KeyT *keys = 0, Boolean &foundP = _Map::foundP)
	{ BTrie<KeyT,DataT> *node = removeTrie(keys, foundP);
	  return node ? &(node->data) : 0; };
    DataT *remove(KeyT key, Boolean &foundP = _Map::foundP)
	{ BTrie<KeyT,DataT> *node = removeTrie(key, foundP);
	  return node ? &(node->data) : 0; };

    Trie<KeyT,DataT> *findTrie(const KeyT *keys = 0,
				Boolean &foundP = _Map::foundP) const;
    Trie<KeyT,DataT> *findTrie(KeyT key, Boolean &foundP = _Map::foundP) const
	{ return sub.find(key, foundP); };

    Trie<KeyT,DataT> *insertTrie(const KeyT *keys = 0,
				Boolean &foundP = _Map::foundP);
    Trie<KeyT,DataT> *insertTrie(KeyT key, Boolean &foundP = _Map::foundP)
	{ BTrie<KeyT,DataT> *subtrie = sub.insert(key, foundP);
          if (!foundP) new (&subtrie->sub) KeyT(0);
	  return subtrie; }

    BTrie<KeyT,DataT> *removeTrie(const KeyT *keys = 0,
				Boolean &foundP = _Map::foundP);
    BTrie<KeyT,DataT> *removeTrie(KeyT key, Boolean &foundP = _Map::foundP)
	{ KeyT keys[2]; keys[0] = key, Map_noKey(keys[1]);
	  return removeTrie(keys, foundP); };

    unsigned int numEntries() const { return sub.numEntries(); };

    void dump() const;				/* debugging: dump contents */
    void memStats(MemStats &stats) const;	/* compute memory stats */

private:
    DataT data;					/* data stored at this node */
    TRIE_INDEX_T< KeyT, DataT > subt;		/* index terminal child nodes */
    TRIE_INDEX_T< KeyT, Trie<KeyT,DataT> > subi;/* index internal child nodes */
};

/*
 * Iteration over immediate child nodes
 */
template <class KeyT, class DataT>
class BTrieIter
{
public:
    BTrieIter(const BTrie<KeyT,DataT> &trie, int (*sort)(KeyT,KeyT) = 0)
	: myIter(trie.sub, sort) {};

    void init() { myIter.init(); } ;
    BTrie<KeyT,DataT> *next(KeyT &key) { return myIter.next(key); };

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

#endif /* _BTrie_h_ */
