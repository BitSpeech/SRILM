/*
 * Trie.cc --
 *	The trie class implementation.
 *
 */

#ifndef _Trie_cc_
#define _Trie_cc_

#ifndef lint
static char Trie_Copyright[] = "Copyright (c) 1995-1998 SRI International.  All Rights Reserved.";
static char Trie_RcsId[] = "@(#)$Header: /home/srilm/devel/dstruct/src/RCS/Trie.cc,v 1.14 1998/01/18 05:33:31 stolcke Exp $";
#endif

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream.h>
#include <new.h>

#include "Trie.h"

#undef INSTANTIATE_TRIE

#ifdef USE_SARRAY_TRIE

# include "SArray.cc"

# define INSTANTIATE_TRIE(KeyT,DataT) \
	typedef Trie<KeyT,DataT> TrieType; \
	INSTANTIATE_SARRAY(KeyT, TrieType ); \
	template class Trie< KeyT,DataT>; \
	template class TrieIter<KeyT,DataT>; \
	template class TrieIter2<KeyT,DataT>

#else /* ! USE_SARRAY_TRIE */

# include "LHash.cc"

# define INSTANTIATE_TRIE(KeyT,DataT) \
	typedef Trie<KeyT,DataT> TrieType; \
	INSTANTIATE_LHASH(KeyT, TrieType ); \
	template class Trie< KeyT,DataT>; \
	template class TrieIter<KeyT,DataT>; \
	template class TrieIter2<KeyT,DataT>

#endif /* USE_SARRAY_TRIE */
   
template <class KeyT, class DataT>
Trie<KeyT,DataT>::Trie()
    : sub(0)
{
    /*
     * Data starts out zero-initialized for convenience
     */
    memset(&data, 0, sizeof(data));
}

template <class KeyT, class DataT>
Trie<KeyT,DataT>::~Trie()
{
    TrieIter<KeyT,DataT> iter(*this);
    KeyT key;
    Trie<KeyT,DataT> *node;

    /*
     * destroy all subtries recursively
     */
    while (node = iter.next(key)) {
	node->~Trie();
    }
}

/*
 * Dump contents of Trie to cerr, using indentation to reflect node depth
 */
template <class KeyT, class DataT>
void
Trie<KeyT,DataT>::dump() const
{
    static int indent = 0;
    TrieIter<KeyT,DataT> iter(*this);
    KeyT key;
    Trie<KeyT,DataT> *node;

#ifdef DUMP_VALUES
    cerr << "Value = " << data << endl;
#endif
    indent += 5;
    while (node = iter.next(key)) {
	for (unsigned i = 0; i < indent; i ++) {
	    cerr << " ";
	}
	cerr << "Key = " << key << endl;
	node->dump();
    }
    indent -= 5;
}

/*
 * Compute memory stats for Trie, including all children and grand-children
 */
template <class KeyT, class DataT>
void
Trie<KeyT,DataT>::memStats(MemStats &stats) const
{
    stats.total += sizeof(*this) - sizeof(sub);
    sub.memStats(stats);

    TrieIter<KeyT,DataT> iter(*this);
    KeyT key;
    Trie<KeyT,DataT> *node;

    while (node = iter.next(key)) {
	stats.total -= sizeof(*node);
	node->memStats(stats);
    }
}

/* 
 * The following methods implement find(), insert(), remove()
 * by straightforward recursive decent.  Recoding this to use iteration
 * should give marginal speedups.
 */

template <class KeyT, class DataT>
Trie<KeyT,DataT> *
Trie<KeyT,DataT>::findTrie(const KeyT *keys, Boolean &foundP) const
{
    if (keys == 0 || Map_noKeyP(keys[0])) {
	foundP = true;
	return (Trie<KeyT,DataT> *)this;	// discard const
    } else {
	Trie<KeyT,DataT> *subtrie = sub.find(keys[0]);

	if (subtrie == 0) {
	    foundP = false;
	    return 0;
	} else {
	    return subtrie->findTrie(keys + 1, foundP);
	}
    }
}

template <class KeyT, class DataT>
Trie<KeyT,DataT> *
Trie<KeyT,DataT>::insertTrie(const KeyT *keys, Boolean &foundP)
{
    if (keys == 0 || Map_noKeyP(keys[0])) {
	foundP = true;
	return this;
    } else {
	Trie<KeyT,DataT> *subtrie = sub.insert(keys[0]);

	return subtrie->insertTrie(keys + 1, foundP);
    }
}

template <class KeyT, class DataT>
Trie<KeyT,DataT> *
Trie<KeyT,DataT>::removeTrie(const KeyT *keys, Boolean &foundP)
{
    if (keys == 0 || Map_noKeyP(keys[0])) {
	foundP = false;
	return 0;
    } else if (Map_noKeyP(keys[1])) {
	Trie<KeyT,DataT> *subtrie = sub.remove(keys[0], foundP);

	if (foundP) {
	    /*
	     * XXX: Need to call deallocator explicitly
	     */
	    subtrie->sub.clear(0);
	    return subtrie;
	} else {
	    return 0;
	}
    } else {
	Trie<KeyT,DataT> *subtrie = sub.find(keys[0], foundP);

        if (!foundP) {
	    return 0;
	} else {
	    return subtrie->removeTrie(keys + 1, foundP);
	}
    }
}

/*
 * Iteration over all nodes at a given level in the trie
 */

template <class KeyT, class DataT>
TrieIter2<KeyT,DataT>::TrieIter2(const Trie<KeyT,DataT> &trie,
			         KeyT *keys, unsigned int level,
				 int (*sort)(KeyT,KeyT))
    : myTrie(trie), keys(keys), level(level), sort(sort),
      myIter(trie.sub, sort), subIter(0), done(false)
{
    /*
     * Set the next keys[] entry now to terminate the sequence.
     * keys[0] gets set later by next()
     */
    if (level == 0) {
	Map_noKey(keys[0]);
    } else if (level == 1) {
	Map_noKey(keys[1]);
    }
}

template <class KeyT, class DataT>
TrieIter2<KeyT,DataT>::~TrieIter2()
{
    delete subIter;
}

template <class KeyT, class DataT>
void
TrieIter2<KeyT,DataT>::init()
{
    delete subIter;
    subIter = 0;
    myIter.init();
    done = false;
}

template <class KeyT, class DataT>
Trie<KeyT,DataT> *
TrieIter2<KeyT,DataT>::next()
{
    if (level == 0) {
	/*
	 * Level enumerators exactly one node -- the trie root.
	 * NOTE: This recursion could be simplified to deal only
	 * with level == 0 as a termination case, but then
	 * a level 1 iteration would be rather expensive (creating
	 * a new iterator for each child).
	 */
	if (done) {
	    return 0;
	} else {
	    done = true;
	    return (Trie<KeyT,DataT> *)&myTrie;		// discard const
	}
    } else if (level == 1) {
	/*
	 * Just enumerate all children.
	 */
	return myIter.next(keys[0]);
    } else {
	/*
	 * Iterate over children until one of its children
	 * is found, via a recursive iteration.
	 */
	while (1) {
	    if (subIter == 0) {
		/*
		 * Previous children's children iteration exhausted.
		 * Advance to next child of our own.
		 */
		Trie<KeyT,DataT> *subTrie = myIter.next(keys[0]);
		if (subTrie == 0) {
		    /*
		     * No more children -- done.
		     */
		    return 0;
		} else {
		    /*
		     * Create the recursive iterator when 
		     * starting with a child ....
		     */
		    subIter = new TrieIter2<KeyT,DataT>(*subTrie, keys + 1,
							level - 1, sort);
		    assert(subIter != 0);
		}
	    }
	    Trie<KeyT,DataT> *next = subIter->next();
	    if (next == 0) {
		/*
		 * ... and destroy it when we're done with it.
		 */
		delete subIter;
		subIter = 0;
	    } else {
		return next;
	    }
	}
    }
}

#endif /* _Trie_cc_ */
