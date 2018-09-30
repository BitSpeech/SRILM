/*
 * RefList.h --
 *	Lists of words strings identified by string ids
 *
 * Copyright (c) 1998-2003 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/RefList.h,v 1.5 2006/01/05 20:21:27 stolcke Exp $
 *
 */

#ifndef _RefList_h_
#define _RefList_h_

#include <iostream>
using namespace std;

#include "Boolean.h"
#include "File.h"
#include "Vocab.h"
#include "LHash.h"
#include "Array.h"

typedef const char *RefString;

RefString idFromFilename(const char *filename);

class RefList 
{
public:
    RefList(Vocab &vocab, Boolean haveIDs = true);
    ~RefList();

    Boolean read(File &file, Boolean addWords = false);
    Boolean write(File &file);

    VocabIndex *findRef(RefString id);
    VocabIndex *findRefByNumber(unsigned id);

private:
    Vocab &vocab;
    Boolean haveIDs;				// whether refs have string IDs
    Array<VocabIndex *> refarray;		// by number
    LHash<RefString, VocabIndex *> reflist;	// by ID
};

#endif /* _RefList_h_ */
