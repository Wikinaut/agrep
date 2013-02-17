/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
#include "agrep.h"
#include "checkfile.h"
#include <errno.h>

extern int errno;
extern CHAR Progname[MAXNAME]; 
extern int SGREP, PAT_FILE, PAT_BUFFER, EXITONERROR, SIMPLEPATTERN,
	CONSTANT, D, NOUPPER, JUMP, I, LINENUM, INVERSE, WORDBOUND, WHOLELINE,
	SILENT, DNA, BESTMATCH, DELIMITER;

/* Make it an interface routine that tells you whether mgrep can be used for the pattern or not: must sneak and access global variable D though... */
int
checksg(Pattern, D, set)
CHAR *Pattern; 
int D;
int set;	/* should I set flags SGREP and DNA? not if called from glimpse via library */
{
	char c;
	int i, m;
	int NOTSGREP = 0;

	if (set) SGREP = OFF;
	m = strlen(Pattern);
#if	DEBUG
	fprintf(stderr, "checksg: len=%d, pat=%s, pat[len]=%d\n", m, Pattern, Pattern[m]);
#endif
	if(!(PAT_FILE || PAT_BUFFER) && (m <= D)) {
		fprintf(stderr, "%s: size of pattern '%s' must be > #of errors %d\n", Progname, Pattern, D);
		if (!EXITONERROR) {
			errno = AGREP_ERROR;
			return -1;
		}
		else exit(2);
	}
	SIMPLEPATTERN = ON;
	for (i=0; i < m; i++) 
	{
		switch(Pattern[i])
		{
		case ';' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case ',' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '.' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '*' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '-' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '[' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case ']' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '(' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case ')' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '<' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '>' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '^' : 
			/* NOTSGREP = 1; sgrep does it; bg 4/27/97 */
			if(D > 0) {
				SIMPLEPATTERN = OFF; 
				goto outoffor;
			}
			break;
		case '$' : 
			/* NOTSGREP = 1; sgrep does it; bg 4/27/97 */
			if(D > 0) {
				SIMPLEPATTERN = OFF; 
				goto outoffor;
			}
			break;
		case '|' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '#' : 
			SIMPLEPATTERN = OFF; 
			goto outoffor;
		case '{':
			SIMPLEPATTERN = OFF;
			goto outoffor;
		case '}':
			SIMPLEPATTERN = OFF;
			goto outoffor;
		case '~':
			SIMPLEPATTERN = OFF;
			goto outoffor;
		case '\\' : 
		{	/* Should I DO the left shift Pattern including Pattern[m] which is '\0', or just ignore the next character after '\\'????? */
			if (set) {	/* preprocess and maskgen figure out what to do */
				i++;	/* in addition to for loop ++ */
			}
			else {	/* maskgen won't be called if we can help it, so shift it to make it verbatim */
				/*
				int j;
				for (j=i; j<m; j++) Pattern[j] = Pattern[j+1];
				m --;
				*/
				i++;
			}
			break;
		}
		default  : 
			break;
		}
	}

outoffor:
	if (CONSTANT) SIMPLEPATTERN = ON;
	if (SIMPLEPATTERN == OFF) return 0;
	if (BESTMATCH) return 0;	/* can have errors, not simple */
	if (!set && (D>0)) return 0;	/* errors, not simple */
	if (NOUPPER && (D>0)) return 0;	/* errors, not simple */     
	if (JUMP == ON) return 0;	/* I, S, D costs, not simple */
	if (DELIMITER) return 0;        /* delimiters avoid mgrep */
	if (I == 0) return 0;		/* I has 0 cost not 1, not simple */
	if (LINENUM) return 0;		/* can't use mgrep, so not simple */
	if (WORDBOUND && (D > 0)) return 0; /* errors, not simple */  
	if (WHOLELINE && (D > 0)) return 0; /* errors, not simple */  
	if (SILENT) return 1;		/* dont care output, so dont care pat */

	if (set) {
		if (!NOTSGREP || CONSTANT) SGREP = ON;
		if (m >= 16) DNA = ON;
		for(i=0; i<m; i++) {
			c = Pattern[i];
			if(c == 'a' || c == 'c' || c == 't' || c == 'g' ) ;
			else DNA = OFF;
		}
	}

#if	0
	/* Ditch this: sgrep does it internally anyway */
	if (SGREP) {	/* => set MUST be on */
	    for (i=0; i < m; i++) 
	    {
		switch(Pattern[i])
		{
		case '\\' : 
			for (j=i; j<m; j++) Pattern[j] = Pattern[j+1];
			m --;
			break;
		default  : 
			break;
		}
	    }
	}
#endif	/*0*/
	return 1;			/* remains simple */
}
