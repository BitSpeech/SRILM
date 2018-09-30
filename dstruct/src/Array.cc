/*
 * Array.cc --
 *	Extensible array implementation
 *
 */

#ifndef _Array_cc_
#define _Array_cc_

#ifndef lint
static char Array_Copyright[] = "Copyright (c) 1995-2005 SRI International.  All Rights Reserved.";
static char Array_RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/dstruct/src/Array.cc,v 1.12 2011/07/19 16:41:57 stolcke Exp $";
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Array.h"

#undef INSTANTIATE_ARRAY
#define INSTANTIATE_ARRAY(DataT) \
	template class Array<DataT>

/*
 * extend the size of an Array to accomodate size elements
 * 	Note we want to zero-initialize the data elements by default,
 *	so we call their initializers with argument 0.  This means
 *	that all data type used as arguments to the Array template
 *	need to provide an initializer that accepts 0 as a single argument.
 */
template <class DataT>
void
Array<DataT>::alloc(unsigned int size)
{
    unsigned int newSize = size + 1 + alloc_size/2;
    DataT *newData = new DataT[newSize];
    assert(newData != 0);

#ifdef ZERO_INITIALIZE
    memset(newData, 0, newSize * sizeof(DataT));
#endif

    for (unsigned i = 0; i < alloc_size; i++) {
	newData[i] = _data[i];
    }

    delete [] _data;

    _data = newData;
    alloc_size = newSize;
}

template <class DataT>
void
Array<DataT>::memStats(MemStats &stats) const
{
    size_t mySize = alloc_size * sizeof(_data[0]);

    stats.total += mySize;
    stats.wasted += (alloc_size - _size) * sizeof(_data[0]);

    stats.allocStats[mySize > MAX_ALLOC_STATS ?
			    MAX_ALLOC_STATS : mySize] += 1;
}

template <class DataT>
Array<DataT> &
Array<DataT>::operator= (const Array<DataT> &other)
{
#ifdef DEBUG
    cerr << "warning: Array::operator= called\n";
#endif

    if (&other == this) {
	return *this;
    }

    delete [] _data;

    _base = other._base;
    _size = other._size;
    alloc_size = other.alloc_size;

    _data = new DataT[alloc_size];
    assert(_data != 0);

#ifdef ZERO_INITIALIZE
    memset(_data, 0, alloc_size * sizeof(DataT));
#endif

    for (unsigned i = 0; i < alloc_size; i++) {
	_data[i] = other._data[i];
    }

    return *this;
}

#endif /* _Array_cc_ */
