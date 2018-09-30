/*
 * LMClient.h
 *	Client-side for network-based LM
 *
 * Copyright (c) 2007, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/LMClient.h,v 1.7 2008/01/21 23:03:51 stolcke Exp $
 *
 */

#ifndef _LMClient_h_
#define _LMClient_h_

#include <stdio.h>

#include "LM.h"
#include "Ngram.h"
#include "Array.h"

class LMClient: public LM
{
public:
    LMClient(Vocab &vocab, const char *server, unsigned order = 0, unsigned cacheOrder = 0);
    ~LMClient();


    LogP wordProb(VocabIndex word, const VocabIndex *context);
    void *contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length);
    LogP contextBOW(const VocabIndex *context, unsigned length);

    Boolean addUnkWords() { return true; };	/* Words are implicitly added
						 * to vocab so we can transmit
						 * them over the network */

protected:
    unsigned order;		/* maximum N-gram length */
    char serverHost[256];
    unsigned serverPort;
    FILE *serverFile;

    unsigned cacheOrder;	/* max N-gram length to cache */
    Ngram probCache;		/* cache for wordProb() results  */
    struct _CIC {
	VocabIndex word;
	Array<VocabIndex> context;
	void *id;
	unsigned length;
    } contextIDCache;		/* single-result cache for contextID() */
    struct _CBC {
	Array<VocabIndex> context;
	unsigned length;
	LogP bow;
    } contextBOWCache;		/* single-result cache for contextBOW() */
};

#endif /* _LMClient_h_ */
