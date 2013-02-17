/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
/* substitute metachar with special symbol                               */
/* if regularr expression, then set flag REGEX                           */
/* if REGEX and MULTIPAT then report error message,                      */
/* -w only for single word pattern. If WORDBOUND & MULTIWORD error       */
/* process start of line, endof line symbol,                             */
/* process -w WORDBOUND option, append special symbol at begin&end of    */
/* process -d option before this routine                                 */
/* the delimiter pattern is in D_pattern (need to end with '; ')         */
/* if '-t' (suggestion: how about -B) the pattern is passed to sgrep     */
/* and doesn't go here                                                   */
/* in that case, -d is ignored? or not necessary                         */
/* upon return, Pattern contains the pattern to be processed by maskgen  */
/* D_pattern contains transformed D_pattern                              */

#include "agrep.h"
#include <errno.h>

extern int PAT_FILE, PAT_BUFFER;
extern ParseTree *AParse;
extern int WHOLELINE, REGEX, FASTREGEX, RE_ERR, DELIMITER, TAIL, WORDBOUND;
extern int HEAD;
extern CHAR Progname[];
extern int D_length, tc_D_length;
extern CHAR tc_D_pattern[MaxDelimit * 2];
extern int table[WORD][WORD];
extern int agrep_initialfd;
extern int EXITONERROR;
extern int errno;

extern int  multifd;
extern char *multibuf;
extern int  multilen;
extern int anum_terminals;
extern ParseTree aterminals[MAXNUM_PAT];
extern char FREQ_FILE[MAX_LINE_LEN], HASH_FILE[MAX_LINE_LEN], STRING_FILE[MAX_LINE_LEN];	/* interfacing with tcompress */
extern int AComplexBoolean;

int
preprocess(D_pattern, Pattern)   /* need two parameters  */
CHAR D_pattern[], Pattern[];
{
	CHAR temp[Maxline], *r_pat, *old_pat;  /* r_pat for r.e. */
	CHAR old_D_pat[MaxDelimit*2];
	int i, j=0, rp=0, m, t=0, num_pos, ANDON = 0;
	int d_end ;  
	int IN_RANGE=0;
	int ret1, ret2;

#if	DEBUG
	fprintf(stderr, "preprocess: m=%d, pat=%s, PAT_FILE=%d, PAT_BUFFER=%d\n", strlen(Pattern), Pattern, PAT_FILE, PAT_BUFFER);
#endif
	if ((m = strlen(Pattern)) <= 0) return 0;
	if (PAT_FILE || PAT_BUFFER) return 0;
	REGEX = OFF;
	FASTREGEX = OFF;
	old_pat = Pattern; /* to remember the starting position */

	/* Check if pattern is a concatenation of ands OR ors of simple patterns */
	multibuf = (char *)malloc(m * 2 + 2);	/* worst case: a,a,a,a,a,a */
	if (multibuf == NULL) goto normal_processing;
	/* if (WORDBOUND) goto normal_processing; */

	multilen = 0;
	AParse = 0;
	ret1 = ret2 = 0;
	if (((ret1 = asplit_pattern(Pattern, m, aterminals, &anum_terminals, &AParse)) <= 0) ||	/* can change the pattern if simple boolean with {} */
	    ((ret2 = asplit_terminal(0, anum_terminals, multibuf, &multilen)) <= 0) ||
	    ((ret2 == 1) && !(aterminals[0].op & NOTPAT))) {	/* must do normal processing */
		if (AComplexBoolean && (AParse != NULL)) destroy_tree(AParse);	/* so that direct exec invocations don't use AParse by mistake! */
#if	DEBUG
		fprintf(stderr, "preprocess: split_pat = %d, split_term = %d, #terms = %d\n", ret1, ret2, anum_terminals);
#endif	/*DEBUG*/
		/*
		if (ret2 == 1) {
			strcpy(Pattern, aterminals[0].data.leaf.value);
			m = strlen(Pattern);
		}
		*/
		m = strlen(Pattern);
		AParse = 0;
		free(multibuf);
		multibuf = NULL;
		multilen = 0;
		goto normal_processing;
	}

	/* This is quick processing */
	if (AParse != 0) {	/* successfully converted to ANDPAT/ORPAT */
		PAT_BUFFER = 1;
		/* printf("preprocess(): converted= %d, patterns= %s", AParse, multibuf); */
		/* Now I have to process the delimiter if any */
		if (DELIMITER) {
			/* D_pattern is "<PAT>; ", D_length is 1 + length of string PAT: see agrep.c/'d' */
			preprocess_delimiter(D_pattern+1, D_length - 1, D_pattern, &D_length);
			/* D_pattern is the exact stuff we want to match, D_length is its strlen */
			if ((tc_D_length = quick_tcompress(FREQ_FILE, HASH_FILE, D_pattern, D_length, tc_D_pattern, MaxDelimit*2, TC_EASYSEARCH)) <= 0) {
				strcpy(tc_D_pattern, D_pattern);
				tc_D_length = D_length;
			}
			/* printf("mgrep's delim=%s,%d tc_delim=%s,%d\n", D_pattern, D_length, tc_D_pattern, tc_D_length); */
		}
		return 0;
	}
	/* else either unknown character, one simple pattern or none at all */

normal_processing:
	for(i=0; i< m; i++) {
		if(Pattern[i] == '\\') i++;
		else if(Pattern[i] == '|' || Pattern[i] == '*') REGEX = ON;
	}

	r_pat = (CHAR *) malloc(strlen(Pattern)+2*strlen(D_pattern) + 8);	/* bug-report, From: Chris Dalton <crd@hplb.hpl.hp.com> */
	strcpy(temp, D_pattern);
	d_end = t = strlen(temp);  /* size of D_pattern, including '; ' */
	if (WHOLELINE) { 
		temp[t++] = LANGLE; 
		temp[t++] = NNLINE; 
		temp[t++] = RANGLE;
		temp[t] = '\0';
		strcat(temp, Pattern);
		m = strlen(temp);
		temp[m++] = LANGLE; 
		temp[m++] = '\n'; 
		temp[m++] = RANGLE; 
		temp[m] = '\0';  
	}
	else {
		if (WORDBOUND) { 
			temp[t++] = LANGLE; 
			temp[t++] = WORDB; 
			temp[t++] = RANGLE;
			temp[t] = '\0'; 
		}
		strcat(temp, Pattern);
		m = strlen(temp);
		if (WORDBOUND) { 
			temp[m++] = LANGLE; 
			temp[m++] = WORDB; 
			temp[m++] = RANGLE; 
		}
		temp[m] = '\0';
	}
	/* now temp contains augmented pattern , m it's size */
	D_length = 0;
	for (i=0, j=0; i< d_end-2; i++) {
		switch(temp[i]) 
		{
		case '\\' : 
			i++; 
			Pattern[j++] = temp[i];
			old_D_pat[D_length++] = temp[i];
			break;
		case '<'  : 
			Pattern[j++] = LANGLE;
			break;
		case '>'  : 
			Pattern[j++] = RANGLE;
			break;
		case '^'  : 
			Pattern[j++] = '\n';
			old_D_pat[D_length++] = temp[i];
			break;
		case '$'  : 
			Pattern[j++] = '\n';
			old_D_pat[D_length++] = temp[i];
			break;
		default  :  
			Pattern[j++] = temp[i];
			old_D_pat[D_length++] = temp[i];
			break;
		}
	}
	if(D_length > MAXDELIM) {
		fprintf(stderr, "%s: delimiter pattern too long (has > %d chars)\n", Progname, MAXDELIM);
		free(r_pat);
		if (!EXITONERROR) {
			errno = AGREP_ERROR;
			return -1;
		}
		else exit(2);
	}

	Pattern[j++] = ANDPAT;
	old_D_pat[D_length] = '\0';
	strcpy(D_pattern, old_D_pat);
	D_length++;
	/*
	  Pattern[j++] = ' ';
	*/
	Pattern[j] = '\0';
	rp = 0; 
	if(REGEX) {
		r_pat[rp++] = '.';    /* if REGEX: always append '.' in front */
		r_pat[rp++] = '(';
		Pattern[j++] = NOCARE;
		HEAD = ON;
	}
	for (i=d_end; i < m ; i++)
	{
		switch(temp[i]) 
		{
		case '\\': 
			i++;  
			Pattern[j++] = temp[i]; 
			r_pat[rp++] = 'o';   /* the symbol doesn't matter */
			break;
		case '#':  
			FASTREGEX = ON;
			if(REGEX) {
				Pattern[j++] = NOCARE;
				r_pat[rp++] = '.';
				r_pat[rp++] = '*';
				break; 
			}
			Pattern[j++] = WILDCD;
			break; 
		case '(':  
			Pattern[j++] = LPARENT; 
			r_pat[rp++] = '(';     
			break;
		case ')':  
			Pattern[j++] = RPARENT; 
			r_pat[rp++] = ')'; 
			break;
		case '[':  
			Pattern[j++] = LRANGE;  
			r_pat[rp++] = '[';
			IN_RANGE = ON;
			break;
		case ']':  
			Pattern[j++] = RRANGE;  
			r_pat[rp++] = ']'; 
			IN_RANGE = OFF;
			break;
		case '<':  
			Pattern[j++] = LANGLE;  
			break;
		case '>':  
			Pattern[j++] = RANGLE;  
			break;
		case '^':  
			if (temp[i-1] == '[') Pattern[j++] = NOTSYM;
			else Pattern[j++] = '\n';
			r_pat[rp++] = '^';
			break;
		case '$':  
			Pattern[j++] = '\n'; 
			r_pat[rp++] = '$';
			break;
		case '.':  
			Pattern[j++] = NOCARE;
			r_pat[rp++] = '.';
			break;
		case '*':  
			Pattern[j++] = STAR; 
			r_pat[rp++] = '*';
			break;
		case '|':  
			Pattern[j++] = ORSYM; 
			r_pat[rp++] = '|';
			break;
		case ',':  
			Pattern[j++] = ORPAT;  
			RE_ERR = ON; 
			break;
		case ';':  
			if(ANDON) RE_ERR = ON; 
			Pattern[j++] = ANDPAT;
			ANDON = ON;
			break;
		case '-':  
			if(IN_RANGE) {
				Pattern[j++] = HYPHEN; 
				r_pat[rp++] = '-';
			}
			else { 
				Pattern[j++] = temp[i];
				r_pat[rp++] = temp[i];
			}  
			break;
		case NNLINE :
			Pattern[j++] = temp[i];
			r_pat[rp++] = 'N';
			break;
		default:   
			Pattern[j++] = temp[i]; 
			r_pat[rp++] = temp[i];
			break;
		}
	}
	if(REGEX) {           /* append ').' at end of regular expression */
		r_pat[rp++] = ')';
		r_pat[rp++] = '.';
		Pattern[j++] = NOCARE;
		TAIL = ON;
	}
	Pattern[j] = '\0'; 
	m = j;
	r_pat[rp] = '\0'; 
	if(REGEX)
	{  
		if(DELIMITER || WORDBOUND)  {
			fprintf(stderr, "%s: -d or -w option is not supported for this pattern\n", Progname);
			free(r_pat);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		if(RE_ERR) {
			fprintf(stderr, "%s: illegal regular expression\n", Progname);
			free(r_pat);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		while(*Pattern != NOCARE && m-- > 0) Pattern++;  /* poit to . */
		num_pos = init(r_pat, table);
		if(num_pos <= 0) {
			fprintf(stderr, "%s: illegal regular expression\n", Progname);
			free(r_pat);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		if(num_pos > MAXREG) {
			fprintf(stderr, "%s: regular expression too long, max is %d\n", Progname,MAXREG);
			free(r_pat);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		strcpy(old_pat, Pattern); /* do real change to the Pattern to be returned */
		free(r_pat);
		return 0;
	} /* if regex */

	free(r_pat);
	return 0;
}

