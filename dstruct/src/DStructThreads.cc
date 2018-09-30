/*
 * DStructThreads.cc
 *
 * Provide mechanisms for freeing thread-specific resources when a thread
 * terminates long before the process.
 *
 * Copyright (c) 2012, SRI International.  All Rights Reserved.
 */

#include "DStructThreads.h"

#include "BlockMalloc.h"
#include "tserror.h"

#if !defined(NO_TLS)
void 
DStructThreads::freeThread() {
    BM_freeThread();

    srilm_tserror_freeThread();
}
#endif
