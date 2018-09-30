/*
 * CachedMem --
 *      A simple memory management template class 
 *
 *  Copyright (c) 2008 SRI International. All Rights Reserved.
 * 
 */

#ifndef _CachedMem_h_
#define _CachedMem_h_

#include <stdio.h>

struct CachedMemUnit { 
  CachedMemUnit * next;
  void * mem;
};

template <class T> 
class CachedMem {

public:
  virtual ~CachedMem () {};
  void * operator new (size_t);
  void   operator delete (void * p, size_t) {
    if (p) addToFreelist(static_cast<T*>(p));
    __num_del ++;
  }
  static void  freeall();
  static void  stat();

protected:
  T * __next;

private:
  static void addToFreelist(T* p) {
    ((CachedMem<T> *) p)->__next = __freelist;
    __freelist = p;
  }

  static T * __freelist;
  static CachedMemUnit * __alloclist; 
  static const size_t __chunk;

  // statistics 
  static int __num_new;
  static int __num_del;
  static int __num_chk;
  
};

#endif /* _CachedMem_h_ */

