/*
 * CachedMem.cc --
 *      A simple memory management template class 
 *
 */

#ifndef _CachedMem_cc_
#define _CachedMem_cc_

#ifndef lint
static char CachedMem_Copyright[] = "Copyright (c) 2008-2010 SRI International.  All Rights Reserved.";
static char CachedMem_RcsId[] = "@(#)$Header: /home/srilm/devel/dstruct/src/RCS/CachedMem.cc,v 1.2 2010/08/03 17:14:55 stolcke Exp $";
#endif

#include "CachedMem.h"

#include <iostream>
using namespace std;

#undef INSTANTIATE_CACHEDMEM
#define INSTANTIATE_CACHEDMEM(T) \
  template <> T * CachedMem< T >::__freelist = 0; \
  template <> CachedMemUnit * CachedMem< T >::__alloclist = 0; \
  template <> int CachedMem< T >:: __num_new = 0; \
  template <> int CachedMem< T >:: __num_del = 0; \
  template <> int CachedMem< T >:: __num_chk = 0;

#ifndef __GNUG__
template <class T> 
T * CachedMem<T>::__freelist = 0;
template <class T> 
CachedMemUnit * CachedMem<T>::__alloclist = 0;
template <class T>
int CachedMem<T>::__num_new = 0;
template <class T>
int CachedMem<T>::__num_del = 0;
template <class T>
int CachedMem<T>::__num_chk = 0;
#endif

template <class T> 
const size_t CachedMem<T>::__chunk = 64;

template <class T>
void CachedMem<T>:: freeall ()
{
  while (__alloclist) {
    CachedMemUnit * p = __alloclist->next;
    ::delete [] static_cast<T*>(__alloclist->mem);
    __alloclist = p;

  }
  __freelist = 0;
  
  __num_chk = 0;
  __num_new = 0;
  __num_del = 0;
}

template <class T> 
void CachedMem<T>:: stat() 
{
  cerr << "Number of allocated chunks: " << __num_chk << endl;
  cerr << "Number of \"new\" calls: " << __num_new  << endl;
  cerr << "Number of \"delete\" calls: " << __num_del << endl;
}

template <class T> 
void * CachedMem<T>::operator new (size_t sz)
{

  if (sz != sizeof(T)) 
#ifdef NO_EXCEPTIONS
    return 0;
#else
    throw "CachedMem: size mismatch in operator new";
#endif

  __num_new ++;
  
  if (!__freelist) {
    
    T * array = ::new T [ __chunk ];
    for (size_t i = 0; i < __chunk; i++)
      addToFreelist(&array[i]);    
    
    CachedMemUnit * u = ::new CachedMemUnit;
    u->next = __alloclist;
    u->mem = static_cast<void *> (array);
    __alloclist = u;

    __num_chk ++;
  }

  T * p = __freelist;
  __freelist = __freelist->CachedMem<T>::__next;
  return p;
}

#endif /* _CachedMem_cc_ */

