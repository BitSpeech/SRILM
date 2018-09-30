/*
    File:   zio.h
    Author: Andreas Stolcke
    Date:   Wed Feb 15 15:19:44 PST 1995
   
    Description:

    Copyright (c) 1994, SRI International.  All Rights Reserved.

    RCS ID: $Id: zio.h,v 1.7 2003/02/21 20:18:53 stolcke Exp $
*/

/*
 *  $Log: zio.h,v $
 *  Revision 1.7  2003/02/21 20:18:53  stolcke
 *  avoid conflict if zopen is already defined in library
 *
 *  Revision 1.6  1999/10/13 09:07:13  stolcke
 *  make filename checking functions public
 *
 *  Revision 1.5  1995/06/22 19:58:26  stolcke
 *  ansi-fied
 *
 *  Revision 1.4  1995/06/12 22:56:37  tmk
 *  Added ifdef around the redefinitions of fopen() and fclose().
 *
 */

/*******************************************************************
   Copyright 1994 SRI International.  All rights reserved.
   This is an unpublished work of SRI International and is not to be
   used or disclosed except as provided in a license agreement or
   nondisclosure agreement with SRI International.
 ********************************************************************/


#ifndef _ZIO_H
#define _ZIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include declarations files. */

#include <stdio.h>

/* Avoid conflict with library function */
#ifdef HAVE_ZOPEN
#define zopen my_zopen
#endif

/* Constants */

#define COMPRESS_SUFFIX   ".Z"
#define GZIP_SUFFIX	  ".gz"
#define OLD_GZIP_SUFFIX	  ".z"

/* Define function prototypes. */

int	stdio_filename_p (const char *name);
int	compressed_filename_p (const char *name);
int 	gzipped_filename_p (const char *name);

FILE *	zopen (const char *name, const char *mode);
int	zclose (FILE *stream);

/* Users of this header implicitly always use zopen/zclose in stdio */

#ifdef ZIO_HACK
#define fopen(name,mode)	zopen(name,mode)
#define fclose(stream)		zclose(stream)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ZIO_H */

