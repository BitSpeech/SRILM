/*
 * File.h
 *	File I/O utilities for LM
 *
 * Copyright (c) 1995,2006 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/misc/src/RCS/File.h,v 1.15 2007/11/27 19:00:22 stolcke Exp $
 *
 */

#ifndef _File_h_
#define _File_h_

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif

#include <stdio.h>

#include "Boolean.h"

const unsigned int maxWordsPerLine = 50000;

extern const char *wordSeparators;

typedef FILE * FILEptr;

/*
 * A File object is a wrapper around a stdio FILE pointer.  If presently
 * provides two kinds of convenience.
 *
 * - constructors and destructors manage opening and closing of the stream.
 *   The stream is checked for errors on closing, and the default behavior
 *   is to exit() with an error message if a problem was found.
 * - the getline() method strips comments and keeps track of input line
 *   numbers for error reporting.
 * 
 * File object can be cast to (FILE *) to perform most of the standard
 * stdio operations in a seamless way.
 */
class File
{
public:
    File(const char *name, const char *mode, int exitOnError = 1);
    File(FILE *fp = 0, int exitOnError = 1);
    ~File();

    char *getline();
    void ungetline();
    int close();
    Boolean reopen(const char *name, const char *mode);
    Boolean reopen(const char *mode);		// switch to binary I/O
    Boolean error() { return (fp == 0) || ferror(fp); };

    operator FILEptr() { return fp; };
    ostream &position(ostream &stream = cerr);
    ostream &offset(ostream &stream = cerr);

    const char *name;
    unsigned int lineno;
    Boolean exitOnError;
    Boolean skipComments;

private:
    FILE *fp;
    char *buffer;
    unsigned bufLen;
    Boolean reuseBuffer;
};

#endif /* _File_h_ */

