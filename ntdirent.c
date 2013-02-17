/* 
   dir.c for MS-DOS by Samuel Lam <skl@van-bc.UUCP>, June/87 
*/ 
 
/* #ifdef WIN32 */
/* 
 * @(#)dir.c 1.4 87/11/06 Public Domain. 
 * 
 *  A public domain implementation of BSD directory routines for 
 *  MS-DOS.  Written by Michael Rendell ({uunet,utai}michael@garfield), 
 *  August 1897 
 *  Ported to OS/2 by Kai Uwe Rommel 
 *  December 1989, February 1990 
 *  Ported to Windows NT 22 May 91 
 *    other mods Summer '92 brianmo@microsoft.com 
 *  opendirx() was horribly written, very inefficient, and did not take care
 *    of all cases.  It is still not too clean, but it is far more efficient.
 *    Changes made by Gordon Chaffee (chaffee@bugs-bunny.cs.berkeley.edu)
 */ 
 
 
/*Includes: 
 *	crt 
 */ 
#include <windows.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys\types.h> 
#include <sys\stat.h> 
#include "ntdirent.h" 

#define stat _stat

/* 
 *	NT specific 
 */ 
#include <stdio.h> 
 
/* 
 *	random typedefs 
 */ 
#define HDIR        HANDLE 
#define HFILE       HANDLE 
#define PHFILE      PHANDLE 
 
/* 
 *	local functions 
 */ 
static char *getdirent(char *); 
static void free_dircontents(struct _dircontents *); 
 
static HDIR				FindHandle; 
static WIN32_FIND_DATA	FileFindData; 
 
static struct dirent dp; 
 
DIR *opendirx(char *name, char *pattern) 
{ 
    struct stat statb; 
    DIR *dirp; 
    char c; 
    char *s; 
    struct _dircontents *dp; 
    int len;
    int unc;
    char path[ OFS_MAXPATHNAME ]; 
    register char *ip, *op;

    for (ip = name, op = path; ; op++, ip++) {
	*op = *ip;
	if (*ip == '\0') {
	    break;
	}
    }
    len = ip - name;
    if (len > 0) {
	unc = ((path[0] == '\\' || path[0] == '/') &&
	       (path[1] == '\\' || path[1] == '/'));
	c = path[len - 1];
	if (unc) {
	    if (c != '\\' && c != '/') {
		path[len] = '/';
		len++;
		path[len] ='\0';
	    }
	} else {
	    if ((c == '\\' || c == '/') && (len > 1)) {
		len--;
		path[len] = '\0';
 
		if (path[len - 1] == ':' ) {
		    path[len] = '/'; len++;
		    path[len] = '.'; len++;
		    path[len] = '\0';
		}
	    } else if (c == ':' ) {
		path[len] = '.';
		len++;
		path[len] ='\0';
	    }
	}
    } else {
	unc = 0;
	path[0] = '.';
	path[1] = '\0';
	len = 1;
    }
 
    if (stat(path, &statb) < 0 || (statb.st_mode & S_IFMT) != S_IFDIR) {
	return NULL; 
    }

    dirp = malloc(sizeof(DIR));
    if (dirp == NULL) {
	return dirp;
    }
 
    c = path[len - 1];
    if (c == '.' ) {
	if (len == 1) {
	    len--;
	} else {
	    c = path[len - 2];
	    if (c == '\\' || c == ':') {
		len--;
	    } else {
		path[len] = '/';
		len++;
	    }
	}
    } else if (!unc && ((len != 1) || (c != '\\' && c != '/'))) {
	path[len] = '/';
	len++;
    }
    strcpy(path + len, pattern);
 
    dirp -> dd_loc = 0; 
    dirp -> dd_contents = dirp -> dd_cp = NULL; 
 
    if ((s = getdirent(path)) == NULL) {
	return dirp;
    }
 
    do 
    { 
	if (((dp = malloc(sizeof(struct _dircontents))) == NULL) || 
	    ((dp -> _d_entry = malloc(strlen(s) + 1)) == NULL)      ) 
	{ 
	    if (dp) 
		free(dp); 
	    free_dircontents(dirp -> dd_contents); 
 
	    return NULL; 
	} 
 
	if (dirp -> dd_contents) 
	    dirp -> dd_cp = dirp -> dd_cp -> _d_next = dp; 
	else 
	    dirp -> dd_contents = dirp -> dd_cp = dp; 
 
	strcpy(dp -> _d_entry, s); 
	dp -> _d_next = NULL; 
 
    } 
    while ((s = getdirent(NULL)) != NULL); 
 
    dirp -> dd_cp = dirp -> dd_contents; 
    return dirp; 
} 
 
DIR *opendir(char *name)
{
  return opendirx(name, "*");
} 

void closedir(DIR * dirp) 
{ 
  free_dircontents(dirp -> dd_contents); 
  free(dirp); 
} 
 
struct dirent *readdir(DIR * dirp) 
{ 
  /* static struct dirent dp; */ 
  if (dirp -> dd_cp == NULL) 
    return NULL; 
 
  /*strcpy(dp.d_name,dirp->dd_cp->_d_entry); */ 
 
  dp.d_name = dirp->dd_cp->_d_entry; 
 
  dp.d_namlen = dp.d_reclen = 
    strlen(dp.d_name); 
 
  dp.d_ino = dirp->dd_loc+1; /* fake the inode */ 
 
  dirp -> dd_cp = dirp -> dd_cp -> _d_next; 
  dirp -> dd_loc++; 
 
 
  return &dp; 
} 
 
void seekdir(DIR * dirp, long off) 
{ 
  long i = off; 
  struct _dircontents *dp; 
 
  if (off >= 0) 
  { 
    for (dp = dirp -> dd_contents; --i >= 0 && dp; dp = dp -> _d_next); 
 
    dirp -> dd_loc = off - (i + 1); 
    dirp -> dd_cp = dp; 
  } 
} 
 
 
long telldir(DIR * dirp) 
{ 
  return dirp -> dd_loc; 
} 
 
static void free_dircontents(struct _dircontents * dp) 
{ 
  struct _dircontents *odp; 
 
  while (dp) 
  { 
    if (dp -> _d_entry) 
      free(dp -> _d_entry); 
 
    dp = (odp = dp) -> _d_next; 
    free(odp); 
  } 
} 
/* end of "free_dircontents" */ 
 
static char *getdirent(char *dir) 
{ 
    int got_dirent; 

    if (dir != NULL) 
    {				       /* get first entry */ 
	if ((FindHandle = FindFirstFile( dir, &FileFindData )) 
	    == (HDIR)0xffffffff) 
	{ 
	    return NULL; 
	} 
	got_dirent = 1;
    } 
    else				       /* get next entry */ 
	got_dirent = FindNextFile( FindHandle, &FileFindData ); 
 
    if (got_dirent) 
	return FileFindData.cFileName; 
    else 
    { 
	FindClose(FindHandle); 
	return NULL; 
    } 
} 
/* end of getdirent() */ 

struct passwd * _cdecl
getpwnam(char *name)
{
    return NULL;
}

struct passwd * _cdecl
getpwuid(int uid)
{
    return NULL;
}

int
getuid()
{
    return 0;
}

void _cdecl
endpwent(void)
{
}

/* #endif */
