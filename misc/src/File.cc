/*
 * File.cc --
 *	File I/O for LM 
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2010 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/File.cc,v 1.16 2010/06/02 04:44:21 stolcke Exp $";
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Boolean.h"
#include "File.h"

#define ZIO_HACK
#include "zio.h"

#if defined(sgi) || defined(_MSC_VER) || defined(WIN32) || defined(linux) && defined(__INTEL_COMPILER) && __INTEL_COMPILER<=700
#define fseeko fseek
#define ftello ftell
#endif

const char *wordSeparators = " \t\r\n";

#define START_BUF_LEN 128

File::File(const char *name, const char *mode, int exitOnError)
    : name(name), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(0), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false)
{
    assert(buffer != 0);

    fp = fopen(name, mode);

    if (fp == 0) {
	if (exitOnError) {
	    perror(name);
	    exit(exitOnError);
	}
    }
}

File::File(FILE *fp, int exitOnError)
    : name(0), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(fp), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false)
{
    assert(buffer != 0);
}

File::~File()
{
    /*
     * If we opened the file (name != 0), then we should close it
     * as well.
     */
    if (name != 0) {
	close();
    }

    free(buffer);
}

int
File::close()
{
    int status = fp ? fclose(fp) : 0;

    fp = 0;
    if (status != 0) {
	if (exitOnError != 0) {
	    perror(name ? name : "");
	    exit(exitOnError);
	}
    }
    return status;
}

Boolean
File::reopen(const char *newName, const char *mode)
{
    /*
     * If we opened the file (name != 0), then we should close it
     * as well.
     */
    if (name != 0) {
	close();
    }

    /*
     * Open new file as in File::File()
     */
    name = newName;

    fp = fopen(name, mode);

    if (fp == 0) {
	if (exitOnError) {
	    perror(name);
	    exit(exitOnError);
	}

	return false;
    }

    return true;
}

Boolean
File::reopen(const char *mode)
{
    if (fp == 0) {
    	return false;
    }

    if (fflush(fp) != 0) {
	if (exitOnError != 0) {
	    perror(name ? name : "");
	    exit(exitOnError);
	}
    }

    FILE *fpNew = fdopen(fileno(fp), mode);

    if (fpNew == 0) {
    	return false;
    } else {
    	// XXX: we can't fclose(fp), so the old stream object becomes garbage
	fp = fpNew;
	return true;
    }
}

char *
File::getline()
{
    if (reuseBuffer) {
	reuseBuffer = false;
	return buffer;
    }

    while (1) {

	unsigned bufOffset = 0;
	Boolean lineDone = false;

	do {
	    buffer[bufLen-1] = 'X';	// check for buffer overflow

	    if (fgets(buffer + bufOffset, bufLen - bufOffset, fp) == 0) {
		if (bufOffset == 0) {
		    return 0;
		} else {
		    buffer[bufOffset] = '\0';
		    lineDone = true;
		}
	    } 

	    // assume bufLen >= 2 !!
	    if (buffer[bufLen-1] == '\0' && buffer[bufLen-2] != '\n') {
		/*
 		 * enlarge buffer
		 */
		bufOffset = bufLen - 1;
		bufLen *= 2;
		buffer = (char *)realloc(buffer, bufLen);
		assert(buffer != 0);
	    } else {
		lineDone = true;
	    }
	} while (!lineDone);

	lineno ++;

	/*
	 * skip entirely blank lines
	 */
	register const char *p = buffer;
	while (*p && isspace(*p)) p++;
	if (*p == '\0') {
	    continue;
	}

	/*
	 * skip comment lines (started with double '#')
	 */
	if (skipComments && buffer[0] == '#' && buffer[1] == '#') {
	    continue;
	}

	reuseBuffer = false;
	return buffer;
    }
}

void
File::ungetline()
{
    reuseBuffer = true;
}

ostream &
File::position(ostream &stream)
{
    if (name) {
	stream << name << ": ";
    }
    return stream << "line " << lineno << ": ";
}

ostream &
File::offset(ostream &stream)
{
    if (name) {
	stream << name << ": ";
    }
    return stream << "offset " << ftello(fp) << ": ";
}
