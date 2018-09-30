/*
 * SArray.h --
 *	Maps based based on sorted arrays.
 *
 * SArray<KeyT,DataT> implements Map<KeyT,DataT> using a single, sorted
 * array of key-value pairs.  This is very space-efficient and lookups
 * take logarithmic time.  However, insertion and deletions are linear
 * in the size of the array.
 *
 * SArrayIter<KeyT,DataT> implements iteration over the entries of the
 * Map object.
 *
 * Copyright (c) 1995-1998 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/dstruct/src/RCS/SArray.h,v 1.31 2007/07/16 23:41:39 stolcke Exp $
 *
 */

#ifndef _SArray_h_
#define _SArray_h_

#include "Map.h"

template <class KeyT, class DataT> class SArray;	// forward declaration
template <class KeyT, class DataT> class SArrayIter;	// forward declaration

template <class KeyT, class DataT>
class SArrayBody
{
    friend class SArray<KeyT,DataT>;
    friend class SArrayIter<KeyT,DataT>;

    unsigned deleted:1;			/* signals deletions to iterator */
    unsigned maxEntries:31;		/* total allocated entries */

    MapEntry<KeyT,DataT> data[1];	/* sorted array of key-value pairs */
};

template <class KeyT, class DataT>
class SArray
#ifdef USE_VIRTUAL
	 : public Map<KeyT,DataT>
#endif
{
    friend class SArrayIter<KeyT,DataT>;

public:
    SArray(unsigned size = 0);
    ~SArray();

    SArray(const SArray<KeyT,DataT> &source);
    SArray<KeyT,DataT> &operator= (const SArray<KeyT,DataT> &source);

    DataT *find(KeyT key, Boolean &foundP = _Map::foundP) const;
    KeyT getInternalKey(KeyT key, Boolean &foundP = _Map::foundP) const;
    DataT *insert(KeyT key, Boolean &foundP = _Map::foundP);
    DataT *remove(KeyT key, Boolean &foundP = _Map::foundP);
    void clear(unsigned size = 0);
    void setsize(unsigned size = 0);
    unsigned numEntries() const;

    void dump() const;			/* debugging: dump contents to cerr */
    void memStats(MemStats &stats) const;	/* compute memory usage */

protected:
    void *body;				/* handle to the above -- this keeps
					 * the size of an empty table to
					 * one word */
    void alloc(unsigned size);		/* allocate data array */
    Boolean locate(KeyT key, unsigned &index) const;
					/* locate key in data */
    static DataT *removedData;		/* temporary buffer for removed item */
};

template <class KeyT, class DataT>
class SArrayIter
{
    typedef int (*compFnType)(KeyT, KeyT);

public:
    SArrayIter(const SArray<KeyT,DataT> &sarray, int (*sort)(KeyT, KeyT) = 0);
    ~SArrayIter();

    void init();
    DataT *next(KeyT &key);

private:
    SArrayBody<KeyT,DataT> *mySArrayBody;
					/* map data being iterated over */
    unsigned current;			/* current index into data
					 * or sortedIndex */
    unsigned numEntries;		/* number of entries */
    int (*sortFunction)(KeyT, KeyT);	/* key sorting function,
					 * or 0 for random order */
    KeyT *sortedKeys;			/* array of sorted keys */
    void sortKeys();			/* initialize sortedKeys */
    static int compareIndex(const void *idx1, const void *idx2);
					/* callback function for qsort() */
};

/*
 * Key Comparison functions
 */
template <class KeyT>
inline int
SArray_compareKey(KeyT key1, KeyT key2)
{
    return (key1 - key2);
}

inline int
SArray_compareKey(const char *key1, const char *key2)
{
    return strcmp(key1, key2);
}

#endif /* _SArray_h_ */

