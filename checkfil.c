/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/*
 *  checkfile.c
 *    takes a file name and checks to see if a file is a regular ascii file
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
/*
#include <sys/stat.h>
*/
#include <errno.h>
#include "checkfil.h"

#ifdef _WIN32
#include <sys/stat.h>
#include "config.h"
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (0100000&(mode))
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (0040000&(mode))
#endif

#define MAXLINE 512

extern char Progname[];
extern int errno;

unsigned char ibuf[MAXLINE];

/**************************************************************************
*
*    check_file
*       input:  filename or path (null-terminated character string)
*       returns: int (0 if file is a regular file, non-0 if not)
*
*    uses stat(2) to see if a file is a regular file.
*
***************************************************************************/

int check_file(fname)
char *fname;

{
	struct stat buf;

	if (stat(fname, &buf) != 0) {
		if (errno == ENOENT)
			return NOSUCHFILE;
		else
			return STATFAILED;  
	} 
	else {
		/*
			  int ftype;
			  if (S_ISREG(buf.st_mode)) {
				if ((ftype = samplefile(fname)) == ISASCIIFILE) {
				  return ISASCIIFILE;
				} else if (ftype == ISBINARYFILE) {
				  return ISBINARYFILE;
				} else if (ftype == OPENFAILED) {
				  return OPENFAILED;
				}
			  }
			  if (S_ISDIR(buf.st_mode)) {
				return ISDIRECTORY;
			  }
			  if (S_ISBLK(buf.st_mode)) {
				return ISBLOCKFILE;
			  }
			  if (S_ISSOCK(buf.st_mode)) {
				return ISSOCKET;
			  }
		*/
		return 0;
	}
}

/***************************************************************************
*
*  samplefile
*    reads in the first part of a file, and checks to see that it is
*    all ascii.
*
***************************************************************************/
/*
int samplefile(fname)
char *fname;
{
char *p;
int numread;
int fd;

  if ((fd = open(fname, O_RDONLY)) == -1) {
	fprintf(stderr, "open failed on filename %s\n", fname);
	return OPENFAILED;
  }

  -comment- No need to use alloc_buf and free_buf here since always read from non-ve fd -tnemmoc-
  
  if (numread = fill_buf(fd, ibuf, MAXLINE)) {
   close(fd);
   p = ibuf;
	while (ISASCII(*p++) && --numread);
	if (!numread) {
	  return(ISASCIIFILE);
	} else {
	  return(ISBINARYFILE);
	}
  } else {
	close(fd);
	return(ISASCIIFILE);
  }
}
*/

