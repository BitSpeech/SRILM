/*
    File:   zio.c
    Author: Andreas Stolcke
    Date:   Wed Feb 15 15:19:44 PST 1995
   
    Description:
                 Compressed file stdio extension
*/

#ifndef lint
static char Copyright[] = "Copyright (c) 1995,1997 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/misc/src/RCS/zio.c,v 1.14 1999/10/13 09:07:13 stolcke Exp $";
#endif

/*
 * $Log: zio.c,v $
 * Revision 1.14  1999/10/13 09:07:13  stolcke
 * make filename checking functions public
 *
 * Revision 1.13  1997/06/07 15:58:47  stolcke
 * fixed some gcc warnings
 *
 * Revision 1.13  1997/06/07 15:56:24  stolcke
 * fixed some gcc warnings
 *
 * Revision 1.12  1997/01/23 20:38:35  stolcke
 * *** empty log message ***
 *
 * Revision 1.11  1997/01/23 20:02:59  stolcke
 * handle SIGPIPE termination
 *
 * Revision 1.10  1997/01/22 07:52:08  stolcke
 * warn about multiple uses of -
 *
 * Revision 1.9  1996/11/30 21:08:59  stolcke
 * use exec in compress commands
 *
 * Revision 1.8  1995/07/19 16:51:31  stolcke
 * remove PATH assignment to account for local setup
 *
 * Revision 1.7  1995/06/22 20:47:16  stolcke
 * dup stdio descriptors so fclose won't disturb them
 *
 * Revision 1.6  1995/06/22 20:44:39  stolcke
 * return more error info
 *
 * Revision 1.5  1995/06/22 19:58:11  stolcke
 * ansi-fied
 *
 * Revision 1.4  1995/06/12 22:57:12  tmk
 * Added ifdef around the redefinitions of fopen() and fclose().
 *
 */

/*******************************************************************
   Copyright 1994,1997 SRI International.  All rights reserved.
   This is an unpublished work of SRI International and is not to be
   used or disclosed except as provided in a license agreement or
   nondisclosure agreement with SRI International.
 ********************************************************************/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <errno.h>

extern int errno;

#include "zio.h"

#ifdef ZIO_HACK
#undef fopen
#undef fclose
#endif

#define STDIO_NAME	  "-"

#define STD_PATH    ":"   /* "PATH=/usr/bin:/usr/ucb:/usr/bsd:/usr/local/bin" */

#define COMPRESS_CMD	  "exec compress -c"
#define UNCOMPRESS_CMD	  "exec uncompress -c"

#define GZIP_CMD	  "exec gzip -c"
#define GUNZIP_CMD	  "exec gunzip -c"

/*
 * Does the filename refer to stdin/stdout ?
 */
int
stdio_filename_p (const char *name)
{
    return (strcmp(name, STDIO_NAME) == 0);
}

/*
 * Does the filename refer to a compressed file ?
 */
int
compressed_filename_p (const char *name)
{
    unsigned len = strlen(name);

    return
	(len > sizeof(COMPRESS_SUFFIX)-1) &&
		(strcmp(name + len - (sizeof(COMPRESS_SUFFIX)-1),
			COMPRESS_SUFFIX) == 0);
}

/*
 * Does the filename refer to a gzipped file ?
 */
int
gzipped_filename_p (const char *name)
{
    unsigned len = strlen(name);

    return 
	(len > sizeof(GZIP_SUFFIX)-1) &&
		(strcmp(name + len - (sizeof(GZIP_SUFFIX)-1),
			GZIP_SUFFIX) == 0) ||
	(len > sizeof(OLD_GZIP_SUFFIX)-1) &&
		(strcmp(name + len - (sizeof(OLD_GZIP_SUFFIX)-1),
			OLD_GZIP_SUFFIX) == 0);
}

/*
 * Check file readability
 */
static int
readable_p (const char *name)
{
    int fd = open(name, O_RDONLY);

    if (fd < 0)
        return 0;
    else {
        close(fd);
	return 1;
    }
}

/*
 * Check file writability
 */
static int
writable_p (const char *name)
{
    int fd = open(name, O_WRONLY|O_CREAT, 0666);

    if (fd < 0)
        return 0;
    else {
        close(fd);
	return 1;
    }
}

/*
 * Open a stdio stream, handling special filenames
 */
FILE *zopen(const char *name, const char *mode)
{
    char command[MAXPATHLEN + 100];

    if (stdio_filename_p(name)) {
	/*
	 * Return stream to stdin or stdout
	 */
	if (*mode == 'r') {
		static int stdin_used = 0;
		int fd;

		if (stdin_used) {
			fprintf(stderr,
				"warning: '-' used multiple times for input\n");
		} else {
			stdin_used = 1;
		}

		fd = dup(0);
		return fd < 0 ? NULL : fdopen(fd, mode);
	} else if (*mode == 'w' || *mode == 'a') {
		static int stdout_used = 0;
		int fd;

		if (stdout_used) {
			fprintf(stderr,
				"warning: '-' used multiple times for output\n");
		} else {
			stdout_used = 1;
		}

		fd = dup(1);
		return fd < 0 ? NULL : fdopen(fd, mode);
	} else {
		return NULL;
	}
    } else if (compressed_filename_p(name)) {
	/*
	 * Return stream to compress pipe
	 */
	if (*mode == 'r') {
		if (!readable_p(name))
		    return NULL;
		sprintf(command, "%s;%s %s", STD_PATH, UNCOMPRESS_CMD, name);
		return popen(command, mode);
	} else if (*mode == 'w') {
		if (!writable_p(name))
		    return NULL;
		sprintf(command, "%s;%s >%s", STD_PATH, COMPRESS_CMD, name);
		return popen(command, mode);
	} else {
		return NULL;
	}
    } else if (gzipped_filename_p(name)) {
	/*
	 * Return stream to gzip pipe
	 */
	if (*mode == 'r') {
		if (!readable_p(name))
		    return NULL;
		sprintf(command, "%s;%s %s", STD_PATH, GUNZIP_CMD, name);
		return popen(command, mode);
	} else if (*mode == 'w') {
		if (!writable_p(name))
		    return NULL;
		sprintf(command, "%s;%s >%s", STD_PATH, GZIP_CMD, name);
		return popen(command, mode);
	} else {
		return NULL;
	}
    } else {
	return fopen(name, mode);
    }
}

/*
 * Close a stream created by zopen()
 */
int
zclose(FILE *stream)
{
    int status;
    struct stat statb;

    /*
     * pclose(), according to the man page, should diagnose streams not 
     * created by popen() and return -1.  however, on SGIs, it core dumps
     * in that case.  So we better be careful and try to figure out
     * what type of stream it is.
     */
    if (fstat(fileno(stream), &statb) < 0)
	return -1;

    /*
     * First try pclose().  It will tell us if stream is not a pipe
     */
    if ((statb.st_mode & S_IFMT) != S_IFIFO ||
        fileno(stream) == 0 || fileno(stream) == 1)
    {
        return fclose(stream);
    } else {
	status = pclose(stream);
	if (status == -1) {
	    /*
	     * stream was not created by popen(), but popen() does fclose
	     * for us in thise case.
	     */
	    return ferror(stream);
	} else if (status == SIGPIPE) {
	    /*
	     * It's normal for the uncompressor to terminate by SIGPIPE,
	     * i.e., if the user program closed the file before reaching
	     * EOF. 
	     */
	     return 0;
	} else {
	    /*
	     * The compressor program terminated with an error, and supposedly
	     * has printed a message to stderr.
	     * Set errno to a generic error code if it hasn't been set already.
	     */
	    if (errno == 0) {
		errno = EIO;
	    }
	    return status;
	}
    }
}

#ifdef STAND
int
main (argc, argv)
    int argc;
    char **argv;
{
    int dowrite = 0;
    char buffer[BUFSIZ];
    int nread;
    FILE *stream;

    if (argc < 3) {
	printf("usage: %s file {r|w}\n", argv[0]);
 	exit(2);
    }

    if (*argv[2] == 'r') {
	stream = zopen(argv[1], argv[2]);

	if (!stream) {
		perror(argv[1]);
		exit(1);
	}

	while (!ferror(stream) && !feof(stream) &&!ferror(stdout)) {
		nread = fread(buffer, 1, sizeof(buffer), stream);
		(void)fwrite(buffer, 1, nread, stdout);
	}
    } else {
	stream = zopen(argv[1], argv[2]);

	if (!stream) {
		perror(argv[1]);
		exit(1);
	}

	while (!ferror(stdin) && !feof(stdin) && !ferror(stream)) {
		nread = fread(buffer, 1, sizeof(buffer), stdin);
		(void)fwrite(buffer, 1, nread, stream);
	}
   }
   if (ferror(stdin)) {
	perror("stdin");
   } else if (ferror(stdout)) {
	perror("stdout");
   } else if (ferror(stream)) {
	perror(argv[1]);
   }
   zclose(stream);
   
}
#endif /* STAND */
