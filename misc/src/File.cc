/*
 * File.cc --
 *	File I/O for LM 
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2010 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/misc/src/File.cc,v 1.20 2011/05/04 01:44:17 stolcke Exp $";
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
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
      reuseBuffer(false), strFileLen(0), strFilePos(0), strFileActive(0)

{
    assert(buffer != 0);

    fp = fopen(name, mode);

    if (fp == 0) {
	if (exitOnError) {
	    perror(name);
	    exit(exitOnError);
	}
    }
    strFile = "";
}

File::File(FILE *fp, int exitOnError)
    : name(0), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(fp), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false), strFileLen(0), strFilePos(0), strFileActive(0)
{
    assert(buffer != 0);
    strFile = "";
}

File::File(const char *fileStr, size_t fileStrLen, int exitOnError, int reserved_length)
    : name(0), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(NULL), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false), strFileLen(0), strFilePos(0), strFileActive(0)
{
    assert(buffer != 0);

    strFile = fileStr;
    strFileLen = strFile.length();
    strFileActive = 1;
    // only reserve space if bigger than current capacity
    if (reserved_length > strFileLen) strFile.reserve(reserved_length);
}

File::File(std::string& fileStr, int exitOnError, int reserved_length)
    : name(0), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(NULL), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false), strFileLen(0), strFilePos(0), strFileActive(0)
{
    assert(buffer != 0);

    strFile = fileStr;
    strFileLen = strFile.length();
    strFileActive = 1;
    // only reserve space if bigger than current capacity
    if (reserved_length > strFileLen) strFile.reserve(reserved_length);
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

    if (buffer) free(buffer);
    buffer = NULL;
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
    strFile = "";
    strFileLen = 0;
    strFilePos = 0;
    strFileActive = 0;
    
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
    strFile = "";
    strFileLen = 0;
    strFilePos = 0;
    strFileActive = 0;
    
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

Boolean
File::reopen(const char *fileStr, size_t fileStrLen, int reserved_length)
{
    if (name != 0) {
	close();
    }

    strFile = fileStr;
    strFileLen = strFile.length();
    strFilePos = 0;
    strFileActive = 1;
    // only reserve space if bigger than current capacity
    if (reserved_length > strFileLen) strFile.reserve(reserved_length);

    return true;
}

Boolean
File::reopen(std::string& fileStr, int reserved_length)
{
    if (name != 0) {
	close();
    }

    strFile = fileStr;
    strFileLen = strFile.length();
    strFilePos = 0;
    strFileActive = 1;
    // only reserve space if bigger than current capacity
    if (reserved_length > strFileLen) strFile.reserve(reserved_length);

    return true;
}

Boolean
File::error()
{
    if (strFileActive) return 0; // i/o using strings not file pointer, so no error
    return (fp == 0) || ferror(fp);
};

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

	    if (fgets(buffer + bufOffset, bufLen - bufOffset) == 0) {
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
    if (fp) {
        return stream << "offset " << ::ftello(fp) << ": ";
    } else {
        return stream << "offset unknown " << ": ";
    }
}



/*------------------------------------------------------------------------*
 * "stdio" functions:
 *------------------------------------------------------------------------*/

int
File::fgetc()
{
    if (fp) {
        return ::fgetc(fp);
    }

    if (!strFileActive || strFileLen <= 0 || strFilePos >= strFileLen) return EOF;

    return strFile.at(strFilePos++);
}

// override fgets in case object using strFile
char *
File::fgets(char *str, int n)
{
    if (fp) {
        return ::fgets(str, n, fp);
    }

    if (!str || n <= 0) return NULL;

    int i = 0;

    for (i = 0; i < n - 1; i++) {
	int c = fgetc();
	if (c == EOF) {
            break;
	} 

        str[i] = c;
        // xxx use \r on MacOS X?
        if (c == '\n') {
            break;
        }
    }

    // always terminate
    str[i] = '\0';
    if (i == 0)
	return NULL;
    else
	return str;
}

int
File::fputc(int c)
{
    if (fp) {
        return ::fputc(c, fp);
    }

    // error condition, no string active
    if (!strFileActive) return EOF;

    strFile += c;

    return 0;
}
     
int
File::fputs(const char *str)
{
    if (fp) {
        return ::fputs(str, fp);
    }

    // error condition, no string active
    if (!strFileActive) return -1;

    strFile += str;

    return 0;
}

int
File::fprintf(const char *format, ...)
{
    if (fp) {
        va_list args;
        va_start(args, format);
        int num_written = vfprintf(fp, format, args);
        va_end(args);
        return num_written;
    }

    // error condition, no string active
    if (!strFileActive) return -1;
    
    char message[4096];         // xxx size!
    va_list args;
    va_start(args, format);
#if defined(sgi)
    // vsnprintf() doesn't exist in Irix 5.3
    vsprintf(message, format, args);
#else
    vsnprintf(message, sizeof(message), format, args);
#endif
    va_end(args);

    strFile += message;

    return 0;
}

long
File::ftell()
{
    if (fp) {
        return ::ftell(fp);
    }

    // error condition, no string active
    if (!strFileActive) return -1;
    
    return (long) strFilePos;    
}

int
File::fseek(long offset, int origin)
{
    if (fp) {
        return ::fseek(fp, offset, origin);
    }

    // error condition, no string active
    if (!strFileActive) return -1;

    // xxx doesn't do (much) error checking
    if (origin == SEEK_CUR) {
        strFilePos += offset;
    } else if (origin == SEEK_END) {
        strFilePos = strFileLen + offset; // use negative offset!
    } else if (origin == SEEK_SET) {
        strFilePos = offset;
    } else {
        // invalid origin
        return -1;
    }

    // xxx we check that position is not negative, but (currently) allow it to be greater than length
    if (strFilePos < 0) strFilePos = 0;

    return 0;
}

const char *
File::c_str()
{
    if (fp) return NULL;

    // error condition, no string active
    if (!strFileActive) return NULL;

    return strFile.c_str();
}
     
const char *
File::data()
{
    if (fp) return NULL;

    // error condition, no string active
    if (!strFileActive) return NULL;

    return strFile.data();
}
     
size_t
File::length()
{
    if (fp) return 0;

    // error condition, no string active
    if (!strFileActive) return 0;

    return strFile.length();
}
