/*
 * Array.h --
 *	Extensible array class
 *
 * Copyright (c) 1995,1997 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/dstruct/src/RCS/Array.h,v 1.9 1997/12/31 23:00:58 stolcke Exp $
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
    ~Array() { delete [] _data; } ;

    DataT &operator[](int index)
    	{ int offset = index - _base; assert(offset >= 0);
	  if (offset >= _size) {
	    _size = offset + 1;
	    if (offset >= alloc_size) { alloc(offset); }
	  }
	  return _data[offset];
	};

    Array<DataT> & Array<DataT>::operator= (const Array<DataT> &other);

    DataT *data() const { return _data - _base; };
    int base() const { return _base; }
    unsigned int size() const { return _size; }

    void memStats(MemStats &stats) const;

private:
    unsigned int _size;		/* used size */
    int _base;
    DataT *_data;
    unsigned int alloc_size;	/* allocated size */
    void alloc(unsigned int size);
};

#endif /* _Array_h_ */
