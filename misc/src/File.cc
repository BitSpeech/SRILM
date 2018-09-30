/*
 * File.cc --
 *	File I/O for LM 
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2011 SRI International, 2012 Microsoft Corp.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/CVS/srilm/misc/src/File.cc,v 1.29 2012/08/08 00:28:50 stolcke Exp $";
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include "Boolean.h"
#include "File.h"
#include "Array.cc"

#define ZIO_HACK
#include "zio.h"

#include "srilm_iconv.h"

#if defined(sgi) || defined(_MSC_VER) || defined(WIN32) || defined(linux) && defined(__INTEL_COMPILER) && __INTEL_COMPILER<=700
#define fseeko fseek
#define ftello ftell
#endif

/*
 * Deal with different types of iconv() input buffer argument
 */
#if defined(sun) || defined(sgi)
# define ICONV_CONST	const
#else
# define ICONV_CONST
#endif

const char *wordSeparators = " \t\r\n";

#define START_BUF_LEN 128		// needs to be > 2

#define iconvNone	((void *)-1)

File::File(const char *name, const char *mode, int exitOnError)
    : name(name), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(0), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false), atFirstLine(true), encoding(ASCII), iconvID(iconvNone),
      strFileLen(0), strFilePos(0), strFileActive(0)

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
      reuseBuffer(false), atFirstLine(true), encoding(ASCII), iconvID(iconvNone),
      strFileLen(0), strFilePos(0), strFileActive(0)
{
    assert(buffer != 0);
    strFile = "";
}

File::File(const char *fileStr, size_t fileStrLen, int exitOnError, int reserved_length)
    : name(0), lineno(0), exitOnError(exitOnError), skipComments(true),
      fp(NULL), buffer((char *)malloc(START_BUF_LEN)), bufLen(START_BUF_LEN),
      reuseBuffer(false), atFirstLine(true), encoding(ASCII), iconvID(iconvNone),
      strFileLen(0), strFilePos(0), strFileActive(0)
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
      reuseBuffer(false), atFirstLine(true), encoding(ASCII), iconvID(iconvNone),
      strFileLen(0), strFilePos(0), strFileActive(0)
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

    if (iconvID != iconvNone) iconv_close((iconv_t)iconvID);

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
    
    atFirstLine = true;
    encoding = ASCII;
    if (iconvID != iconvNone) {
	iconv_close((iconv_t)iconvID);
	iconvID = iconvNone;
    }

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

    atFirstLine = true;
    encoding = ASCII;
    if (iconvID != iconvNone) {
	iconv_close((iconv_t)iconvID);
	iconvID = iconvNone;
    }

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
    atFirstLine = true;
    encoding = ASCII;
    if (iconvID != iconvNone) {
	iconv_close((iconv_t)iconvID);
	iconvID = iconvNone;
    }

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
    atFirstLine = true;
    encoding = ASCII;
    if (iconvID != iconvNone) {
	iconv_close((iconv_t)iconvID);
	iconvID = iconvNone;
    }

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

const char UTF8magic[] = "\357\273\277";
const char UTF16LEmagic[] = "\377\376";
const char UTF16BEmagic[] = "\376\377";

char *
File::fgetsUTF8(char *buffer, int buflen)
{
    memset(buffer, 0, buflen);

    /*
     * make sure 2-byte encodings have one extra byte for final \0
     */
    char *result = fgets(buffer, buflen % 2 ? buflen : buflen - 1);

    if (result == 0) {
	return 0;
    }

    /*
     * When at the start of the file, try to determine charcter encoding scheme
     */
    if (atFirstLine) {
        const unsigned UTF8magicLen = sizeof(UTF8magic)-1;
        const unsigned UTF16LEmagicLen = sizeof(UTF16LEmagic)-1;
        const unsigned UTF16BEmagicLen = sizeof(UTF16BEmagic)-1;
	unsigned magicLen = 0;

        atFirstLine = false;
	iconvID = (void *)0;

	if (strncmp(buffer, UTF8magic, UTF8magicLen) == 0) {
	    encoding = UTF8;
	    magicLen = UTF8magicLen;
	} else if (strncmp(buffer, UTF16LEmagic, UTF16LEmagicLen) == 0) {
	    encoding = UTF16LE;
	    magicLen = UTF16LEmagicLen;
	    iconvID = (void *)iconv_open("UTF-8", "UTF-16LE");
	} else if (strncmp(buffer, UTF16BEmagic, UTF16BEmagicLen) == 0) {
	    encoding = UTF16BE;
	    magicLen = UTF16BEmagicLen;
	    iconvID = (void *)iconv_open("UTF-8", "UTF-16BE");
	}

	if (iconvID == iconvNone) {
	    this->position() << "conversion from UTF-16" << (encoding == UTF16LE ? "LE" : "BE") << " not supported\n";
	    return 0;
	} else if (iconvID == (void *)0) {
	    iconvID = iconvNone;
	}

	/*
	 * remove the magic string from the buffer
	 */
	if (magicLen > 0) {
	    memmove(buffer, buffer + magicLen, buflen - magicLen);
	    memset(buffer + buflen - magicLen, 0, magicLen);
	}
    }

    /*
     * change 16-bit encoding to UTF-8 if needed
     */
    if (iconvID != iconvNone) {
	makeArray(char, buffer2, buflen);

	ICONV_CONST char *cp = buffer;
	size_t inSize = buflen % 2 ? buflen-1 : buflen;
	char *dp = buffer2;
	size_t outSize = buflen;

#ifdef DEBUG_ICONV
	::fprintf(stderr, "insize = %d input chars = ", (int)inSize);
	for (unsigned j = 0; j < inSize; j ++) {
	    ::fprintf(stderr, "'%c'(%03o) ", (buffer2[j] == '\r' ?  'R' : buffer[j]), ((unsigned char *)buffer)[j]);
	}
	::fprintf(stderr, "\n");
#endif

	if (iconv((iconv_t)iconvID, &cp, &inSize, &dp, &outSize) == (size_t)-1) {
	    perror("iconv");
	    return 0;
	}

#ifdef DEBUG_ICONV
	::fprintf(stderr, "buflen = %d outsize = %d chars = ", buflen, (int)outSize);
	for (unsigned j = 0; j < outSize; j ++) {
	    ::fprintf(stderr, "'%c'(%03o) ", (buffer2[j] == '\r' ?  'R' : buffer2[j]), ((unsigned char *)(char *)buffer2)[j]);
	}
	::fprintf(stderr, "\n");
#endif

	memcpy(buffer, buffer2, outSize);

	if (outSize < (size_t)buflen) {
	    memset(buffer + outSize, 0, buflen - outSize);
	}

	if (encoding == UTF16LE) {
	    /*
	     * fgets() only reads up the \n --
	     * need to skip the following \0 byte
	     */
	    unsigned len = strlen(buffer);
	    if (len > 0 && buffer[len-1] == '\n') fgetc();
	}
    }

    return buffer;
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
	    if (fgetsUTF8(buffer + bufOffset, bufLen - bufOffset) == 0) {
		if (bufOffset == 0) {
		    return 0;
		} else {
		    buffer[bufOffset] = '\0';
		    break;
		}
	    } 

	    /*
	     * Check if line end has been reached
	     */
	    unsigned numbytes = strlen(buffer+bufOffset);

	    if (numbytes > 0 && buffer[bufOffset+numbytes-1] != '\n') {
		if (bufOffset + numbytes >= bufLen - START_BUF_LEN) {
		    /*
		     * enlarge buffer
		     */
		    //cerr << "!REALLOC!" << endl;
		    bufLen *= 2;
		    buffer = (char *)realloc(buffer, bufLen);
		    assert(buffer != 0);
		}
		bufOffset += numbytes;
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
	    // include \n in result
	    i++;
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

    // This is the default max size to append at any one time. On sgi we
    // get a buffer overrrun if we exceed this but elsewhere we manually
    // allocate a larger buffer if needed.
    const int maxMessage = 4096;
    char message[maxMessage];
    va_list args;
    va_start(args, format);
#if defined(sgi)
    // vsnprintf() doesn't exist in Irix 5.3
    // Return value >= 0 is number of bytes written to buffer not including
    // NULL terminator.
    int nwritten = vsprintf(message, format, args);
    if (nwritten >= maxMessage) {
        // Buffer overflow!
        if (exitOnError) {
            exit(exitOnError);
        }
        // At least indicate overflow in output (if haven't crashed already)
        sprintf(message, "In class File, BUFFER OVERFLOW %d >= %d\n", nwritten, maxMessage);
    }
    strFile += message;
#else
    // Return value not consistent...
    // Non-Windows: >= 0 is number of bytes needed in buffer not including
    // NULL terminator.
    // Windows: Returns -1 if output truncated.
    int checkSize = vsnprintf(message, maxMessage, format, args);
    if ((checkSize >= maxMessage) || (checkSize < 0)) {
        int curSize;
        if (checkSize >= maxMessage) {
            // Should know exact size needed
            curSize = checkSize + 1;
        } else {
            // Start with double initial size
            curSize = maxMessage * 2;
        }
        bool success = false;
        // Loop until successful but also impose 1GB cap on buffer size.
        const int maxAlloc = 1000000000;
        while (!success) {
            va_end(args);
            va_start(args, format);
            char* buf = new char[curSize];
            checkSize = vsnprintf(buf, curSize, format, args);
            if ((checkSize >= 0) && (checkSize < curSize)) {
                strFile += buf;
                success = true;
            } else {
                // Try larger size
                if (curSize <= maxAlloc / 2) {
                    curSize *= 2;
                } else if (curSize < maxAlloc) {
                    // Don't exceed cap
                    curSize = maxAlloc;
                } else {
                    // Fail
                    delete[] buf;
                    if (exitOnError) {
                        exit(exitOnError);
                    }
                    strFile += "In class File, failed writing to buffer\n";
                    break;
                }
            }
            delete[] buf;
        }
    } else {
        strFile += message;
    }
#endif

    va_end(args);

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
