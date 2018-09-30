/*
 * LHash.h --
 *	Hash table with linear search.
 * 
 * LHash<KeyT,DataT> implements Map<KeyT,DataT> as a hash table with
 * open addressing, i.e., entries are stored at the computed hash index,
 * or in the next free entry, which is located with linear search.
 * Lookups, insertions, and deletions are all done in linear expected
 * time, with a constant factor that depends on the occupancy rate of
 * the table.  No space is wasted with pointers or bucket structures,
 * but the size of the table is at least the number of entries rounded to
 * the next power of 2.
 *
 * LHashIter<KeyT,DataT> performs iterations (in random key order) over
 * the entries in a hash table.
 *
 * Copyright (c) 1995-1998 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/dstruct/src/RCS/LHash.h,v 1.27 1998/01/18 05:33:13 stolcke Exp $
 *
 */

#ifndef _LHash_h_
#define _LHash_h_

#include "Map.h"

/* 
 * These constants are used below to pack the number of entries 
 * and the total allocated size into a single word.
 */
#define LHASH_MAXBIT_NBITS		5
#define LHASH_MAXENTRY_NBITS		27

const unsigned LHash_maxBitLimit = ((1<<LHASH_MAXBIT_NBITS)-1);
const unsigned LHash_maxEntriesLimit = ((1<<LHASH_MAXENTRY_NBITS)-1);

template <class KeyT, class DataT> class LHash;		// forward declaration
template <class KeyT, class DataT> class LHashIter;	// forward declaration

template <class KeyT, class DataT>
class LHashBody
{
    friend class LHash<KeyT,DataT>;
    friend class LHashIter<KeyT,DataT>;

    unsigned maxBits:LHASH_MAXBIT_NBITS;	/* number of bits in hash code
				     	 	 *  = log2 (maxEntries) */
    unsigned nEntries:LHASH_MAXENTRY_NBITS;	/* number of entries */
    struct entry {
	KeyT key;
	DataT value;
    }		data[1];	/* hashed array of key-value pairs */
};

/*
 * If we define the class a a subclass of Map<KeyT,DataT> we
 * incur and extra word of memory per instance for the virtual function
 * table pointer.
 */
template <class KeyT, class DataT>
class LHash
#ifdef USE_VIRTUAL
	 : public Map<KeyT,DataT>
#endif
{
    friend class LHashIter<KeyT,DataT>;

public:
    LHash(unsigned size = 0);
    ~LHash();

    LHash(const LHash<KeyT,DataT> &source);
    LHash<KeyT,DataT> &operator= (const LHash<KeyT,DataT> &source);

    DataT *find(KeyT key, Boolean &foundP = _Map::foundP) const;
    KeyT getInternalKey(KeyT key, Boolean &foundP = _Map::foundP) const;
    DataT *insert(KeyT key, Boolean &foundP = _Map::foundP);
    DataT *remove(KeyT key, Boolean &foundP = _Map::foundP);
    void clear(unsigned size = 0);
    unsigned numEntries() const
	{ return body ? ((LHashBody<KeyT,DataT> *)body)->nEntries : 0; } ;
    void dump() const; 			/* debugging: dump contents to cerr */

    void memStats(MemStats &stats) const;

private:
    void *body;				/* handle to the above -- this keeps
					 * the size of an empty table to
					 * one word */
    Boolean locate(KeyT key, unsigned &index) const;
					/* locate key in *data */
    void alloc(unsigned size);		/* initialization */
    static DataT *removedData;		/* temporary buffer for removed item */
};

template <class KeyT, class DataT>
class LHashIter
{
    typedef int (*compFnType)(KeyT, KeyT);

public:
    LHashIter(const LHash<KeyT,DataT> &lhash, int (*sort)(KeyT, KeyT) = 0);
    ~LHashIter();

    void init();
    DataT *next(KeyT &key);

private:
    LHashBody<KeyT,DataT> *myLHashBody;	/* data being iterated over */
    unsigned current;			/* current index into data
					 * or sortedKeys */
    unsigned numEntries;		/* number of entries */
    int (*sortFunction)(KeyT, KeyT);	/* key sorting function,
					 * or 0 for random order */
    KeyT *sortedKeys;			/* array of sorted keys,
					   or 0 for random order */
    void sortKeys();			/* initialize sortedKeys */
    static int compareIndex(const void *idx1, const void *idx2);
					/* callback function for qsort() */
};

#endif /* _LHash_h_ */

