/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
/* The function of the program is to traverse the
   direcctory tree and collect paath names.
   This program is derived from the C-programming language book
   Originally, the program open a directory file as a regular file. But
   it won't work. We have to open a directory file using
   opendir system call, and use readdir() to read each entry of the 
   directory.
*/

#include "autoconf.h"	/* ../libtemplate/include */
#include <stdio.h>
#include <sys/types.h>
#if	ISO_CHAR_SET
#include <locale.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
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

#include <sys/stat.h>
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

	/* printf(" In treewalk\n"); */
	if(my_lstat(name, &stbuf) == -1) {
		fprintf(stderr, "permission denied or non-existent: %s\n", name);
		return;
	}
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK)  {
		return;
	}
	if (( stbuf.st_mode & S_IFMT) == S_IFDIR) 
		directory(name);
	else {
		file_list[fdx] = (char *)malloc(BUFSIZE);
		strcpy(file_list[fdx++], name);
		/* printf("	%s\n",  name); */
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
	/*
		printf("in directory, name= %s\n",name);
	*/
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
		/*
			printf("dp->d_name = %s\n", dp->d_name);
		*/
		strcpy(nbp, dp->d_name);
		treewalk(name);
CONT:
		;
	}
	closedir (dirp);
	*--nbp = '\0'; /* restore name */
}
