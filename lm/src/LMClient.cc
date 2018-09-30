/*
 * LMClient.cc --
 *	Client-side for network-based LM
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2007-2008 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/LMClient.cc,v 1.15 2009/10/15 18:58:36 stolcke Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if !defined(_MSC_VER) && !defined(WIN32)
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#if __INTEL_COMPILER == 700
// old Intel compiler cannot deal with optimized byteswapping functions
#undef htons
#undef ntohs
#endif

#endif  /* !_MSC_VER && !WIN32 */

#include "LMClient.h"
#include "RemoteLM.h"
#include "Array.cc"

LMClient::LMClient(Vocab &vocab, const char *server,
		   unsigned order, unsigned cacheOrder)
    : LM(vocab), order(order), serverFile(0),
      cacheOrder(cacheOrder), probCache(vocab, cacheOrder)
{
#ifdef SOCK_STREAM
    if (server == 0) {
	strcpy(serverHost, "localhost");
    	serverPort = SRILM_DEFAULT_PORT;
    } else {
	unsigned i = sscanf(server, "%u@%255s", &serverPort, serverHost);

	if (i == 2) {
	    // we have server port and hostname
	    ;
	} else if (i == 1) {
	    // we use localhost as the hostname
	    strcpy(serverHost, "localhost");
	} else if (sscanf(server, "%64s", serverHost) == 1) {
	    // we use a default port number
	    serverPort = SRILM_DEFAULT_PORT;
	} else {
	    strcpy(serverHost, "localhost");
	    serverPort = SRILM_DEFAULT_PORT;
	}
    }

    struct hostent *host;
    struct in_addr serverAddr;
    struct sockaddr_in sockName;

    /*
     * Get server address either by ip number or by name
     */
    if (isdigit(*serverHost)) {
        serverAddr.s_addr = inet_addr(serverHost);
    } else {
	host = gethostbyname(serverHost);

	if (host == 0) {
	    cerr << "server host " << serverHost << " not found\n";
	    return;
	}

	memcpy(&serverAddr, host->h_addr, sizeof(host->h_addr));
    }

    memset(&sockName, 0, sizeof(sockName));

    sockName.sin_family = AF_INET;
    sockName.sin_addr = serverAddr;
    sockName.sin_port = htons(serverPort);

    /*
     * Create, then connect socket to the server
     */
    int serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
    	cerr << "server " << serverPort << "@" << serverHost
	     << ": " << strerror(errno) << endl;
	exit(1);
    }

    if (connect(serverSocket, (struct sockaddr *)&sockName, sizeof(sockName)) < 0) {
    	cerr << "server " << serverPort << "@" << serverHost
	     << ": " << strerror(errno) << endl;
	close(serverSocket);
	exit(1);
    }

    /*
     * Wrap the socket in a FILE objects
     */
    serverFile = fdopen(serverSocket, "r+");
    if (!serverFile) {
        cerr << "server " << serverPort << "@" << serverHost
	     << ": " << strerror(errno) << endl;
	close(serverSocket);
	exit(1);
    }

    /*
     * Read the banner line from the server
     */
    char msg[REMOTELM_MAXRESULTLEN];

    int msglen = recv(fileno(serverFile), msg, sizeof(msg)-1, 0);
    if (msglen < 0) {
	cerr << "server " << serverPort << "@" << serverHost
	     << ": could not read banner\n";
	fclose(serverFile);
	serverFile = 0;
	exit(1);
    } else if (debug(1)) {
	msg[msglen] = '\0';
	cerr << "server " << serverPort << "@" << serverHost
	     << ": " << msg;
    }

    /*
     * Switch to version 2 protocol
     */
    fprintf(serverFile, "%s\n", REMOTELM_VERSION2);
    fflush(serverFile);

    msglen = recv(fileno(serverFile), msg, sizeof(msg)-1, 0);
    if (msglen < 0 || strncmp(msg, REMOTELM_OK, sizeof(REMOTELM_OK)-1) != 0) {
	cerr << "server " << serverPort << "@" << serverHost
	     << ": protocol version 2 not supported\n";
	fclose(serverFile);
	serverFile = 0;
	exit(1);
    }

    /*
     * Initialize contextID() cache
     */
    contextIDCache.word = Vocab_None;
    contextIDCache.context[0] = Vocab_None;
    contextIDCache.id = 0;
    contextIDCache.length = 0;

    /*
     * Initialize contextBOW() cache
     */
    contextBOWCache.context[0] = Vocab_None;
    contextBOWCache.length = 0;
    contextBOWCache.bow = LogP_One;
#else
    cerr << "LMClient not supported\n";
    exit(1);
#endif /* SOCK_STREAM */
}

LMClient::~LMClient()
{
    if (serverFile != 0) {
    	fclose(serverFile);	// this will close(serverSocket)
    }
}

LogP
LMClient::wordProb(VocabIndex word, const VocabIndex *context)
{
#ifdef SOCK_STREAM
    if (serverFile == 0) {
    	exit(1);
    }

    unsigned clen = Vocab::length(context);

    /*
     * Limit context length as requested
     */
    if (order > 0 && clen > order - 1) {
    	clen = order - 1;
    }

    LogP *cachedProb = 0;

    /*
     * If this n-gram is cacheable, see if we already have it
     */
    if (clen < cacheOrder) {
	TruncatedContext usedContext(context, clen);
	cachedProb = probCache.insertProb(word, usedContext);

	if (*cachedProb != 0.0) {
	    return *cachedProb;
	}
    }

    fprintf(serverFile, "%s ", REMOTELM_WORDPROB);
    for (int i = clen - 1; i >= 0; i --) {
    	fprintf(serverFile, "%s ", vocab.getWord(context[i]));
    }
    fprintf(serverFile, "%s\n", vocab.getWord(word));
    fflush(serverFile);

    char buffer[REMOTELM_MAXRESULTLEN];

    int msglen = recv(fileno(serverFile), buffer, sizeof(buffer)-1, 0);

    if (msglen < 0) {
	cerr << "server " << serverPort << "@" << serverHost
	     << ": " << strerror(errno) << endl;
	exit(1);
    } else {
	buffer[msglen] = '\0';

	LogP lprob;

	if (strncmp(buffer, REMOTELM_OK, sizeof(REMOTELM_OK)-1) == 0 &&
	    parseLogP(buffer + sizeof(REMOTELM_OK), lprob))
    	{
	    /*
	     * Save new probability in cache
	     */
	    if (cachedProb != 0) {
		*cachedProb = lprob;
	    }
	    return lprob;
	} else {
	    cerr << "server " << serverPort << "@" << serverHost
		 << ": unexpected return: " << buffer;
	    exit(1);
	}
    }
#else 
    exit(1);
#endif /* SOCK_STREAM */
}

void *
LMClient::contextID(VocabIndex word, const VocabIndex *context,
							unsigned &length)
{
#ifdef SOCK_STREAM
    if (serverFile == 0) {
    	exit(1);
    }

    unsigned clen = Vocab::length(context);

    /*
     * Limit context length as requested
     */
    if (order > 0 && clen > order - 1) {
    	clen = order - 1;
    }

    TruncatedContext usedContext(context, clen);

    /*
     * If this context is cacheable, see if we already have it from the
     * immediately prior call
     */

    if (clen < cacheOrder) {
	Vocab::compareVocab = 0;		// force index-based compare

	if (word == contextIDCache.word &&
	    Vocab::compare(usedContext, contextIDCache.context) == 0)
	{
	    length = contextIDCache.length;
	    return contextIDCache.id;
	}
    }

    if (word == Vocab_None) {
	fprintf(serverFile, "%s ", REMOTELM_CONTEXTID1);
    } else {
	fprintf(serverFile, "%s ", REMOTELM_CONTEXTID2);
    }

    for (int i = clen - 1; i >= 0; i --) {
    	fprintf(serverFile, "%s ", vocab.getWord(context[i]));
    }

    if (word == Vocab_None) {
	fprintf(serverFile, "\n");
    } else {
	fprintf(serverFile, "%s\n", vocab.getWord(word));
    }
    fflush(serverFile);

    char buffer[REMOTELM_MAXRESULTLEN];

    int msglen = recv(fileno(serverFile), buffer, sizeof(buffer)-1, 0);

    if (msglen < 0) {
	cerr << "server " << serverPort << "@" << serverHost
	     << ": " << strerror(errno) << endl;
	exit(1);
    } else {
	buffer[msglen] = '\0';

	unsigned long cid;

	if (strncmp(buffer, REMOTELM_OK, sizeof(REMOTELM_OK)-1) == 0 &&
	    sscanf(buffer + sizeof(REMOTELM_OK), "%lu %u", &cid, &length) == 2)
    	 {
	    if (clen < cacheOrder) {
	    	// cache results
		contextIDCache.word = word;
		contextIDCache.context[clen] = Vocab_None;
		Vocab::copy(contextIDCache.context, usedContext);
		contextIDCache.id = (void *)cid;
		contextIDCache.length = length;
	    }
	    return (void *)cid;
	} else {
	    cerr << "server " << serverPort << "@" << serverHost
		 << ": unexpected return: " << buffer;
	    exit(1);
	}
    }
#else 
    exit(1);
#endif /* SOCK_STREAM */
}

LogP
LMClient::contextBOW(const VocabIndex *context, unsigned length)
{
#ifdef SOCK_STREAM
    if (serverFile == 0) {
    	exit(1);
    }

    unsigned clen = Vocab::length(context);

    /*
     * Limit context length as requested
     */
    if (order > 0 && clen > order - 1) {
    	clen = order - 1;
    }

    TruncatedContext usedContext(context, clen);

    /*
     * If this context is cacheable, see if we already have it from the
     * immediately prior call
     */

    if (clen < cacheOrder) {
	Vocab::compareVocab = 0;		// force index-based compare

	if (length == contextBOWCache.length &&
	    Vocab::compare(usedContext, contextBOWCache.context) == 0)
	{
	    return contextBOWCache.bow;
	}
    }

    fprintf(serverFile, "%s ", REMOTELM_CONTEXTBOW);
    for (int i = clen - 1; i >= 0; i --) {
    	fprintf(serverFile, "%s ", vocab.getWord(context[i]));
    }
    fprintf(serverFile, "%u\n", length);
    fflush(serverFile);

    char buffer[REMOTELM_MAXRESULTLEN];

    int msglen = recv(fileno(serverFile), buffer, sizeof(buffer)-1, 0);

    if (msglen < 0) {
	cerr << "server " << serverPort << "@" << serverHost
	     << ": " << strerror(errno) << endl;
	exit(1);
    } else {
	buffer[msglen] = '\0';

	LogP bow;

	if (strncmp(buffer, REMOTELM_OK, sizeof(REMOTELM_OK)-1) == 0 &&
	    parseLogP(buffer + sizeof(REMOTELM_OK), bow))
    	{
	    if (clen < cacheOrder) {
	    	// cache results
		contextBOWCache.context[clen] = Vocab_None;
		Vocab::copy(contextBOWCache.context, usedContext);
		contextBOWCache.length = length;
		contextBOWCache.bow = bow;
	    }
	    return bow;
	} else {
	    cerr << "server " << serverPort << "@" << serverHost
		 << ": unexpected return: " << buffer;
	    exit(1);
	}
    }
#else 
    exit(1);
#endif /* SOCK_STREAM */
}

