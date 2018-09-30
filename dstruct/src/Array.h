/*
 * Array.h --
 *	Extensible array class
 *
 * Copyright (c) 1995-2010 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/CVS/srilm/dstruct/src/Array.h,v 1.21 2010/06/02 04:57:29 stolcke Exp $
 *
 */

#ifndef _Array_h_
#define _Array_h_

#include <assert.h>

#include "MemStats.h"

template <class DataT>
class Array
{
public:
    Array(int base = 0, unsigned int size = 0)
	: _base(base), _size(size), _data(0), alloc_size(0)
	{ if (size > 0) { alloc(size-1); } };
    Array(Array<DataT> &source)
	: _base(source._base), _size(0), _data(0), alloc_size(0)
    	{ *this = source; };

    ~Array() { delete [] _data; };

    DataT &operator[](long index)
    	{ unsigned long offset = index - _base; assert((long)offset >= 0);
	  if (offset >= _size) {
	    _size = offset + 1;
	    if (offset >= alloc_size) { alloc(offset); }
	  }
	  return _data[offset];
	};
    /* these are redundant, but work around problems in MS Visual C++ */
    DataT &operator[] (unsigned long long index) { return (*this)[(long) index]; };
    DataT &operator[] (long long index) { return (*this)[(long) index]; };
    DataT &operator[] (unsigned long index) { return (*this)[(long) index]; };
    DataT &operator[] (unsigned index) { return (*this)[(long) index]; };
    DataT &operator[] (int index) { return (*this)[(long) index]; };
    DataT &operator[] (unsigned short index) { return (*this)[(long) index]; };
    DataT &operator[] (short index) { return (*this)[(long) index]; };

    Array<DataT> & operator= (const Array<DataT> &other);

    operator DataT* () { return data(); };
    operator const DataT* () const { return data(); };

    DataT *data() const { return _data - _base; };
    int base() const { return _base; }
    unsigned int size() const { return _size; }

    void memStats(MemStats &stats) const;

protected:
    int _base;
    unsigned int _size;		/* used size */
    DataT *_data;
    unsigned int alloc_size;	/* allocated size */
    void alloc(unsigned int size);
};

/*
 * An optimized version of Array for when the size never changes
 */
template <class DataT>
class StaticArray: public Array<DataT>
{
public:
    StaticArray(unsigned size)
	: Array<DataT>(0, size) { };
    StaticArray(int base, unsigned size)
	: Array<DataT>(base, size) { };

    DataT &operator[](int index)	// dispense with index range check
    	{ return Array<DataT>::_data[index - Array<DataT>::_base]; };
    /* these are redundant, but work around problems in MS Visual C++ */
    DataT &operator[](unsigned index) { return (*this)[(int)index]; };
    DataT &operator[](unsigned short index) { return (*this)[(int)index]; };
    DataT &operator[](short index) { return (*this) [(int)index]; };
};

/*
 * Macro defining a linear array sized at run-time:
 * gcc and icc allow this as part of the language, but many other C++ compilers
 * don't (Sun Studio, MS Visual C).
 */
#if !defined(DEBUG) && defined(__GNUC__) && (!defined(__INTEL_COMPILER) || __INTEL_COMPILER >=900)
# define makeArray(T, A, n)		T A[n]
#else
# define makeArray(T, A, n)		StaticArray<T> A(n)
#endif

#endif /* _Array_h_ */
