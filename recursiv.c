/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/* Burra Gopal:

   The function of the program is to traverse the
   
   This program is derived from the C-programming language book
   
   Originally, the program opens a directory file as a regular file.
   But it won't work.
   
   We have to open a directory file using opendir() system call,
   and use readdir() to read each entry of the directory.
   
   
   [chg] T.Gries 11.08.96
   
*/

/* #define REC_DIAG */

#include "autoconf.h"	/* ../libtemplate/include */
#include <stdio.h>
#include <sys/types.h>

#if	ISO_CHAR_SET
#include <locale.h>
#endif

#if HAVE_DIRENT_H

#ifndef _WIN32
# include <dirent.h>
#else
# include "ntdirent.h"
#endif
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen

# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif

# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif

# if HAVE_NDIR_H
#  include <ndir.h>
# endif

#endif

#ifdef __APPLE__
    #include <sys/stat.h>
#endif

#ifdef _WIN32
#include "config.h"
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
int  exec();    /* agrep.c */
#endif

/*
 * #include <sys/stat.h>
 */
#include <fcntl.h>
#define BUFSIZE 256
#define DIRSIZE 14
#define max_list 10

#ifndef S_ISREG
#define S_ISREG(mode) (0100000&(mode))
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (0040000&(mode))
#endif

/* TG 28.04.04 */
/* TG 2013032 */
#ifndef S_IFDIR
#define S_IFDIR __S_IFDIR
#endif

#ifndef S_IFMT
#define S_IFMT __S_IFMT
#endif

char *file_list[max_list*2];
int  fdx=0; /* index of file_List */
extern int Numfiles;
char name_buf[BUFSIZE];

void directory();
static void treewalk();

/* returns -1 if error, num of matches >= 0 otherwise */

int
recursive(argc, argv)

int argc;
char **argv;
{
	int i,j;
	int num = 0, ret;

	for(i=0; i< argc; i++) {

/*		printf("RECURSIVE: I= %d = %s\n",i,argv[i]);	*/

		strcpy(name_buf, argv[i]);
		treewalk(name_buf);

		if(fdx > 0) {
			Numfiles = fdx;
			if ((ret = exec(3, file_list)) == -1) return -1;
			num += ret;
			for(j=0; j<fdx; j++) {
				free(file_list[j]);
			}
		}
		fdx = 0;
	}

	return num;
}


/*
main(argc, argv)
int argc; char **argv;
{
	char buf[BUFSIZE];

#if	ISO_CHAR_SET
	setlocale(LC_ALL, "");
#endif
	if (argc == 1) {
		strcpy(buf, ".");
		treewalk(buf);
	}
	else 
		while(--argc > 0) {
			strcpy(buf, *++argv);
			treewalk(buf);
		}
}
*/

static void
treewalk(name)

char *name;
{
	struct stat stbuf;
	int i;
	extern void *malloc();

#ifdef REC_DIAG
	printf(" In treewalk. name= %s\n",name);
#endif

#ifndef _WIN32
	if(lstat(name, &stbuf) == -1) {
#else
        if(stat(name, &stbuf) != 0) {
#endif
		fprintf(stderr, "permission denied or non-existent: %s\n", name);
		return;
	}
#ifndef _WIN32
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK)  {

#ifdef REC_DIAG	
	        fprintf(stderr, "S_IFLNK %s",name);
#endif

		return;
	}
#endif
	if (( stbuf.st_mode & S_IFMT) == S_IFDIR) {

#ifdef REC_DIAG	
	        fprintf(stderr, "S_IFDIR %s",name);
#endif
		
     		directory(name);
		}
	else {
		file_list[fdx] = (char *)malloc(BUFSIZE);
		strcpy(file_list[fdx++], name);
		
#ifdef REC_DIAG
		printf("	%s\n",  name);
#endif		
		if(fdx >= max_list) {
			Numfiles = fdx;
			exec(3, file_list);
			for(i=0; i<max_list; i++) free(file_list[i]);
			fdx=0;
		}

	}
}

void
directory(name)
char *name;
{
	struct dirent *dp;
	char *nbp;
	DIR *dirp;

#ifdef REC_DIAG
		printf("in directory, name= %s\n",name);
#endif

	nbp = name + strlen(name);
	if( nbp+DIRSIZE+2 >= name+BUFSIZE ) /* name too long */
	{
		fprintf(stderr, "name too long: %.32s...\n", name);
		return;
	}
	
	if((dirp = opendir(name)) == NULL) {
		fprintf(stderr, "permission denied: %s\n", name);
		return;
	}
	
	*nbp++ = '/';
	*nbp = '\0';
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	
		if (dp->d_name[0] == '\0' || strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..")==0) 
			goto CONT;

#ifdef REC_DIAG
			printf("dp->d_name = %s\n", dp->d_name);
#endif

		strcpy(nbp, dp->d_name);
		treewalk(name);
CONT:
		;
	}
	closedir (dirp);
	*--nbp = '\0'; /* restore name */
}
