/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
/* test the conflicts between options */
#include <stdio.h>
#include "agrep.h"

/* TG 29.04.04 */
#include <errno.h>

extern int D;
extern int FILENAMEONLY, APPROX, PAT_FILE, PAT_BUFFER, MULTI_OUTPUT, COUNT, INVERSE, BESTMATCH;
extern FILEOUT;
extern REGEX;
extern DELIMITER;
extern WHOLELINE;
extern LINENUM;
extern I, S, DD;
extern JUMP;
extern char Progname[32];
extern int agrep_initialfd;
extern int EXITONERROR;
extern int errno;

int
compat()
{
	if(BESTMATCH)  if(COUNT || FILENAMEONLY || APPROX || PAT_FILE) {
		BESTMATCH = 0;
		fprintf(stderr, "%s: -B option ignored when -c, -l, -f, or -# is on\n", Progname);
	}
	if (COUNT && LINENUM) {
		LINENUM = 0;
		fprintf(stderr, "%s: -n option ignored with -c\n", Progname);
	}
	if(PAT_FILE || PAT_BUFFER)   {
		if(APPROX && (D > 0))  {
			fprintf(stderr, "%s: approximate matching is not supported with -f option\n", Progname);
		}
		/*
				if(INVERSE) {
					fprintf(stderr, "%s: -f and -v are not compatible\n", Progname);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
		*/
		if(LINENUM) {
			fprintf(stderr, "%s: -f and -n are not compatible\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		/*
		if(DELIMITER) {
			fprintf(stderr, "%s: -f and -d are not compatible\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		*/
	}
	if (MULTI_OUTPUT && LINENUM) {
		fprintf(stderr, "%s: -M and -n are not compatible\n", Progname);
		if (!EXITONERROR) {
			errno = AGREP_ERROR;
			return -1;
		}
		else exit(2);
	}
	if(JUMP) {
		if(REGEX) {
			fprintf(stderr, "%s: -D#, -I#, or -S# option is ignored for regular expression pattern\n", Progname);
			JUMP = 0;
		}
		if(I == 0 || S == 0 || DD == 0) {
			fprintf(stderr, "%s: the error cost cannot be 0\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
	}
	if(DELIMITER) {
		if(WHOLELINE) {
			fprintf(stderr, "%s: -d and -x are not compatible\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
	}
	if (INVERSE && (PAT_FILE || PAT_BUFFER) && MULTI_OUTPUT) {
		fprintf(stderr, "%s: -v and -M are not compatible\n", Progname);
		if (!EXITONERROR) {
			errno = AGREP_ERROR;
			return -1;
		}
		else exit(2);
	}

	return 0;
}
