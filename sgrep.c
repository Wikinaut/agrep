/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/* #define DEBUG2 */
/* #define DEBUG */

/*	

[chg]	05.10.96	sgrep():
			- when the last buffer is read:
			  no need to look back to the previous CR, 
			  just process until (end) = (buf_end)
			  
[chg]			bm():
			- when there is a hit at the last line of a file,
			  and the file has not a CR as the last character,
			  now the print buffer is terminated with an artifical
			  CR.
			   
[chg]	04.10.96	bm(): major bugs in the algorithm removed [TG]
[chg]	22.09.96	UL850[].lower_1 -> UL850.lower [TG]
[chg]	21.08.96	header file ISO_CHAR  [TG]
[chg] 	13.08.96	edited by Tom Gries <gries@ibm.net>

*/

#include <stdio.h>
#include <ctype.h>
#include "agrep.h"
#include "codepage.h"

/* TG 29.04.04 */
#include <errno.h>

extern unsigned char LUT[256];

#undef	MAXSYM
#define MAXSYM  256

#define MAXMEMBER 8192
#define	CHARTYPE	unsigned char

#undef	MaxError			/* don't use agrep.h definition */
#define MaxError 20

#define MAXPATT 256

#undef	MAXLINE
#define MAXLINE 1024

#undef	MAXNAME
#define MAXNAME 256

#undef	MaxCan				/* don't use agrep.h definition */
#define MaxCan  2048

#define BLOCKSIZE    16384
#define MAX_SHIFT_2  4096

#undef	ON
#define ON      1

#undef	OFF
#define OFF	0

#define LOG_ASCII	8
#define LOG_DNA 	3
#define MAXMEMBER_1	65536
#define LONG_EXAC	20
#define LONG_APPX	24

#if	ISO_CHAR_SET
#define W_DELIM		256
#else
#define W_DELIM		128
#endif

#ifndef _WIN32
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

extern int tuncompressible();
extern int quick_tcompress();
extern int quick_tuncompress();

extern int DELIMITER, OUTTAIL;
extern int D_length, tc_D_length;
extern unsigned char D_pattern[MaxDelimit *2], tc_D_pattern[MaxDelimit *2];
extern int LIMITOUTPUT, LIMITPERFILE, INVERSE;
extern int CurrentByteOffset;
extern int BYTECOUNT;
extern int PRINTOFFSET;
extern int PRINTRECORD;
extern int CONSTANT, COUNT, FNAME, SILENT, FILENAMEONLY, prev_num_of_matched, num_of_matched;

extern int DNA ;	/* DNA flag is set in checksg when pattern is DNA pattern
			   and p_size > 16  */

extern WORDBOUND, WHOLELINE, NOUPPER;
extern unsigned char CurrentFileName[],  Progname[]; 
extern unsigned Mask[];
extern unsigned endposition;
extern int agrep_inlen;
extern CHARTYPE *agrep_inbuffer;

extern int agrep_initialfd;
extern FILE *agrep_finalfp;
extern int agrep_outpointer;
extern int agrep_outlen;
extern CHARTYPE * agrep_outbuffer;

extern int NEW_FILE, POST_FILTER;

extern int EXITONERROR;
extern int errno;
extern int TCOMPRESSED;
extern int EASYSEARCH;
extern char FREQ_FILE[MAX_LINE_LEN], HASH_FILE[MAX_LINE_LEN], STRING_FILE[MAX_LINE_LEN];

extern void alloc_buf(int fd, unsigned char **sbuf, int size);
extern void free_buf(int fd, char *sbuf);
extern unsigned char * backward_delimiter(unsigned char *end, unsigned char *begin, unsigned char *delim, int len, int outtail);
extern unsigned char * forward_delimiter(unsigned char *begin, unsigned char *end, unsigned char *delim, int len, int outtail);

int  fill_buf();          /* bitap.c */
int  a_monkey();          /* sgrep.c */
int  agrep();             /* sgrep.c */
int  bm();                /* sgrep.c */
int  blog();              /* sgrep.c */
int  monkey();            /* sgrep.c */
int  monkey4();           /* sgrep.c */
int  s_output();          /* sgrep.c */
int  verify();            /* sgrep.c */

#if	MEASURE_TIMES
/* timing variables */
extern int OUTFILTER_ms;
extern int FILTERALGO_ms;
extern int INFILTER_ms;
#endif	/*MEASURE_TIMES*/

unsigned char BSize;                /* log_c m   */
unsigned char char_map[MAXSYM];

/* data area */

int		shift_1;
CHARTYPE	SHIFT[MAXSYM];
CHARTYPE	MEMBER[MAXMEMBER];
CHARTYPE	pat[MAXPATT];
unsigned	Hashmask;
char		MEMBER_1[MAXMEMBER_1];
CHARTYPE	TR[MAXSYM];

static void initmask();
static void am_preprocess();
static void m_preprocess();
static void prep();
static void prep4();
static void prep_bm();

/*
 * General idea behind output processing with delimiters, inverse, compression, etc.

 * CAUTION: In compressed files, we can search ONLY for simple patterns or their ;,.
 * Attempts to search for complex patterns / with errors might lead to spurious matches.

 * 1. Once we find the match, go back and forward to get the delimiters that surround
 *    the matched region.

 * 2. If it is a compressed file, verify that the match is "real" (compressed files
 *    can have pseudo matches hence this filtering step is required).

 * 3. Increment num_of_matched.

 * 4. Process some output options which print stuff before the matched region is
 *    printed.

 * 5. If there is compression, decomress and output the matched region. Otherwise
 *    just output it as is. Remember, from step (1) we know the matched region.

 * 6. If inverse is set, then we must keep track of the end of the last matched region
 *    in the variable lastout. When there is a match, we must print everything from
 *    lastout to the beginning of the current matched region (curtextbegin) and then
 *    update lastout to point to the end of the current matched region (curtextend).
 *    ALSO: if we exit from the main loops, we must output everything from the end
 *    of the last matched region to the end of the input buffer.

 * 7. Delimiter handling in complex patterns is different: there the search is done
 *    for a boolean and of the delimiter pattern and the actual pattern.

 */

/* skips over escaped characters */

unsigned char *
mystrchr(s, c)

unsigned char *s;
int c;
{
	unsigned char	*t = s;

	while (*t) {
		if (*t == '\\') t++;
		else if (c == *t) return t;
		t ++;
	}
	return NULL;
}

void
char_tr(pat, m)

unsigned char *pat;
int *m;
{
	int i;
	unsigned char temp[MAXPATT];

	for(i=0; i<MAXSYM; i++) TR[i] = i;

	/* if(NOUPPER) [TG] */ {

		for(i=0; i<MAXSYM; i++)

#if (defined(__EMX__) && defined(ISO_CHAR_SET))

			TR[i] = TR[LUT[i]];
#else
			if (isupper(i)) TR[i] = TR[tolower(i)];
#endif
	}
	

	/*
	if(WORDBOUND) {
		for(i=0; i<MAXSYM; i++) {
			if(!isalnum(i)) TR[i] = W_DELIM;  removed by Udi.
			
			we don't use the trick of making the boundary W_delim anymore.
			It's too buggy otherwise and it's not necessary.
			
		}
	}
	removed by bg 11/8/94
	*/
		
	if(WHOLELINE) {
		memcpy(temp, pat, *m);
		pat[0] = '\n';
		memcpy(pat+1, temp, *m);
		pat[*m+1] = '\n';
		pat[*m+2] = 0;
		*m = *m + 2;
	}
}

int sgrep(in_pat, in_m, fd, D, samepattern)

CHARTYPE *in_pat;  
int fd, in_m, D;
{
	CHARTYPE patbuf[MAXLINE];
	CHARTYPE *pat = patbuf;
	int m = in_m;
	CHARTYPE *text; /* input text stream */
	int offset = 2*MAXLINE;
	int buf_end, num_read, i, start, end, residue = 0;
	int first_time = 1;
	CHARTYPE *oldpat = pat;
	int k, j, oldm = m;
	static CHARTYPE newpat[MAXLINE];	/* holds compressed version */
	static int newm;

#if	MEASURE_TIMES
	static struct timeval initt, finalt;
#endif

	CHARTYPE *tempbuf;
	int	oldCurrentByteOffset;

	strncpy(pat, in_pat, MAXLINE);
	pat[MAXLINE-1] = '\0';

#define PROCESS_PATTERN \
	if (!CONSTANT) {\
		if( (pat[0] == '^') || (pat[0] == '$') ) pat[0] = '\n';\
		if ((m>1) && (pat[m-2] != '\\') && ((pat[m-1] == '^') || (pat[m-1] == '$'))) pat[m-1] = '\n';\
	}\
	/* whether constant or not, interpret the escape character */\
	for (k=0; k<m; k++) {\
		if (pat[k] == '\\') {\
			for (j=k; j<m; j++)\
				pat[j] = pat[j+1]; /* including '\0' */\
			m--;\
		}\
	}\
	char_tr(pat, &m);   /* will change pat, and m if WHOLELINE is ON */\
	if(m >= MAXPATT) {\
		fprintf(stderr, "%s: pattern too long (has > %d chars)\n", Progname, MAXPATT);\
		if (!EXITONERROR) {\
			errno = AGREP_ERROR;\
			return -1;\
		}\
		else exit(2);\
	}\
	if(D == 0) {\
		if(m > LONG_EXAC) m_preprocess(pat);\
		else prep_bm(pat, m);\
	}\
	else if (DNA) prep4(pat, m);\
	else 	if(m >= LONG_APPX) am_preprocess(pat);\
	else {\
		prep(pat, m, D);\
		initmask(pat, Mask, m, 0, &endposition);\
	}

#if	AGREP_POINTER
	if (fd != -1) {
#endif	/*AGREP_POINTER*/
		alloc_buf(fd, &text, 2*BLOCKSIZE+2*MAXLINE+MAXPATT);
		text[offset-1] = '\n';  /* initial case */
		for(i=0; i < MAXLINE; i++) text[i] = 0;   /* security zone */
		start = offset;   
		if(WHOLELINE) {
			start--;
			CurrentByteOffset --;
		}

		while( (num_read = fill_buf(fd, text+offset, 2*BLOCKSIZE)) > 0) 
		{
			buf_end = end = offset + num_read -1 ;
			
			oldCurrentByteOffset = CurrentByteOffset;

			if (first_time) {
				if ((TCOMPRESSED == ON) && tuncompressible(text+offset, num_read)) {
					EASYSEARCH = text[offset+SIGNATURE_LEN-1];
					start += SIGNATURE_LEN;
					CurrentByteOffset += SIGNATURE_LEN;
					if (!EASYSEARCH) {
						fprintf(stderr, "not compressed for easy-search: can miss some matches in: %s\n", CurrentFileName);
					}
#if	MEASURE_TIMES
					gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
					if (samepattern || ((newm = quick_tcompress(FREQ_FILE, HASH_FILE, pat, m, newpat, MAXLINE-8, EASYSEARCH)) > 0)) {
						oldm = m;
						oldpat = pat;
						m = newm;
						pat = newpat;
					}
#if	MEASURE_TIMES
					gettimeofday(&finalt, NULL);
					INFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
				}
				else TCOMPRESSED = OFF;

				PROCESS_PATTERN	/* must be AFTER we know that it is a compressed pattern... */

				/* to make sure the skip loop in bm() won't go out of bound in later iterations:
				   This was the original code. 
				   It will be inefficient to place a copy of the pattern at the end
				   of the buffer. Better put that stop pattern to the end of the
				   _actual_ read block (end)  [TG] 04.10.96 */
				   
				/* for(i=1; i<=m; i++) text[2*BLOCKSIZE+offset+i] = pat[m-1]; [del] [TG] */

				/* Emergency Stop: put one copy of pattern to the end of the buffer
				   to make sure that the skip loop in bm()
				   won't go out of bound in later iterations */
		   
				/* save portion being overwritten.
				   copied from below (memagrep()), but not need here: */
				/* memcpy(tempbuf, text+end+1, m); */
				
				for(i=1; i<=m; i++) text[end+i] = pat[m-1]; /* [new] [TG] */

				first_time = 0;
			}

                        if (!DELIMITER) {
/* [TG] */			if (num_read == 2*BLOCKSIZE) {
                                	while ((text[end]  != '\n') && (end > offset)) end--;
				}
				/* else end = buf_end; no need to look back [TG] */
				
                                text[start-1] = '\n';
                        }
                        else {
                                unsigned char *newbuf = text + end + 1;
                                newbuf = backward_delimiter(newbuf, text+offset, D_pattern, D_length, OUTTAIL);        /* see agrep.c/'d' */
				if (newbuf < text+offset+D_length) newbuf = text + end + 1;
                                end = newbuf - text - 1;
                                memcpy(text+start-D_length, D_pattern, D_length);
                        }

			residue = buf_end - end + 1 ;

			/* SGREP_PROCESS */
			/* No harm in sending a few extra parameters even if they are unused: they are not accessed in monkey*()s */
			if(D==0)  {
				if(m > LONG_EXAC) {
					if (-1 == monkey(pat, m, text+start, text+end, oldpat, oldm)) {
						free_buf(fd, text);
						return -1;
					}
				}
				else {
					if (-1 == bm(pat, m, text+start, text+end, oldpat, oldm)) {
						free_buf(fd, text);
						return -1;
					}
				}
			}
			else {
				if(DNA) {
					if (-1 == monkey4( pat, m, text+start, text+end, D , oldpat, oldm )) {
						free_buf(fd, text);
						return -1;
					}
				}
				else {
					if(m >= LONG_APPX) {
						if (-1 == a_monkey(pat, m, text+start, text+end, D, oldpat, oldm)) {
							free_buf(fd, text);
							return -1;
						}
					}
					else {
						if (-1 == agrep(pat, m, text+start, text+end, D, oldpat, oldm)) {
							free_buf(fd, text);
							return -1;
						}
					}
				}
			}
			if(FILENAMEONLY && (num_of_matched - prev_num_of_matched) && (NEW_FILE || !POST_FILTER)) {
				if (agrep_finalfp != NULL)
					fprintf(agrep_finalfp, "%s\n", CurrentFileName);
				else {
					int outindex;
					for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
							(CurrentFileName[outindex] != '\0'); outindex++) {
						agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
					}
					if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer+1>=agrep_outlen)) {
						OUTPUT_OVERFLOW;
						free_buf(fd, text);
						return -1;
					}
					else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
					agrep_outpointer += outindex;
				}
				free_buf(fd, text);
				NEW_FILE = OFF;
				return 0; 
			}

			CurrentByteOffset = oldCurrentByteOffset + end - start + 1;	/* for a new iteration: avoid complicated calculations below */
			start = offset - residue ;
			if(start < MAXLINE) {
				start = MAXLINE; 
			}
			strncpy(text+start, text+end, residue);
			start++;
			if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
			    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
				free_buf(fd, text);
				return 0;	/* done */
			}
		} /* end of while(num_read = ...) */
                if (!DELIMITER) {
                        text[start-1] = '\n';
                        text[start+residue] = '\n';
                }
                else {
                        if (start > D_length) memcpy(text+start-D_length, D_pattern, D_length);
                        memcpy(text+start+residue, D_pattern, D_length);
                }
		end = start + residue - 2;
                if(residue > 1) {
			/* SGREP_PROCESS */
			/* No harm in sending a few extra parameters even if they are unused: they are not accessed in monkey*()s */
			if(D==0)  {
				if(m > LONG_EXAC) {
					if (-1 == monkey(pat, m, text+start, text+end, oldpat, oldm)) {
						free_buf(fd, text);
						return -1;
					}
				}
				else {
					if (-1 == bm(pat, m, text+start, text+end, oldpat, oldm)) {
						free_buf(fd, text);
						return -1;
					}
				}
			}
			else {
				if(DNA) {
					if (-1 == monkey4( pat, m, text+start, text+end, D , oldpat, oldm )) {
						free_buf(fd, text);
						return -1;
					}
				}
				else {
					if(m >= LONG_APPX) {
						if (-1 == a_monkey(pat, m, text+start, text+end, D, oldpat, oldm)) {
							free_buf(fd, text);
							return -1;
						}
					}
					else {
						if (-1 == agrep(pat, m, text+start, text+end, D, oldpat, oldm)) {
							free_buf(fd, text);
							return -1;
						}
					}
				}
			}
			if(FILENAMEONLY && (num_of_matched - prev_num_of_matched) && (NEW_FILE || !POST_FILTER)) {
				if (agrep_finalfp != NULL)
					fprintf(agrep_finalfp, "%s\n", CurrentFileName);
				else {
					int outindex;
					for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
							(CurrentFileName[outindex] != '\0'); outindex++) {
						agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
					}
					if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer+1>=agrep_outlen)) {
						OUTPUT_OVERFLOW;
						free_buf(fd, text);
						return -1;
					}
					else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
					agrep_outpointer += outindex;
				}
				free_buf(fd, text);
				NEW_FILE = OFF;
				return 0; 
			}
                }
		free_buf(fd, text);
		return 0;
#if	AGREP_POINTER
	}
	else {	/* as if only one iteration of the while-loop and offset = 0 */
		tempbuf = (CHARTYPE*)malloc(m);
		text = (CHARTYPE *)agrep_inbuffer;
		num_read = agrep_inlen;
		start = 0;
		buf_end = end = num_read - 1;
#if	0
		if (WHOLELINE) {
			start --;
			CurrentByteOffset --;
		}
#endif
		if ((TCOMPRESSED == ON) && tuncompressible(text+1, num_read)) {
			EASYSEARCH = text[offset+SIGNATURE_LEN-1];
			start += SIGNATURE_LEN;
			CurrentByteOffset += SIGNATURE_LEN;
			if (!EASYSEARCH) {
				fprintf(stderr, "not compressed for easy-search: can miss some matches in: %s\n", CurrentFileName);
			}
#if	MEASURE_TIMES
			gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
			if (samepattern || ((newm = quick_tcompress(FREQ_FILE, HASH_FILE, pat, m, newpat, MAXLINE-8, EASYSEARCH)) > 0)) {
				oldm = m;
				oldpat = pat;
				m = newm;
				pat = newpat;
			}
#if	MEASURE_TIMES
			gettimeofday(&finalt, NULL);
			INFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
		}
		else TCOMPRESSED = OFF;

		PROCESS_PATTERN	/* must be after we know whether it is compressed or not */

		/* Emergency Stop: put one copy of pattern to the end of the buffer
		   to make sure that the skip loop in bm()
		   won't go out of bound in later iterations */
		   
		memcpy(tempbuf, text+end+1, m);	/* save portion being overwritten */
		for(i=1; i<=m; i++) text[end+i] = pat[m-1];

                        if (!DELIMITER)
                                while(text[end]  != '\n' && end > 1) end--;
                        else {
                                unsigned char *newbuf = text + end + 1;
                                newbuf = backward_delimiter(newbuf, text, D_pattern, D_length, OUTTAIL);        /* see agrep.c/'d' */
				if (newbuf < text+offset+D_length) newbuf = text + end + 1;
                                end = newbuf - text - 1;
                        }
                        /* text[0] = text[end] = r_newline; : the user must ensure that the delimiter is there at text[0] and occurs somewhere before text[end ] */

			/* An exact copy of the above SGREP_PROCESS */
			/* No harm in sending a few extra parameters even if they are unused: they are not accessed in monkey*()s */
			if(D==0)  {
				if(m > LONG_EXAC) {
					if (-1 == monkey(pat, m, text+start, text+end, oldpat, oldm)) {
						free_buf(fd, text);
						memcpy(text+end+1, tempbuf, m); /* restore */
						free(tempbuf);
						return -1;
					}
				}
				else {
					if (-1 == bm(pat, m, text+start, text+end, oldpat, oldm)) {
						free_buf(fd, text);
						memcpy(text+end+1, tempbuf, m); /* restore */
						free(tempbuf);
						return -1;
					}
				}
			}
			else {
				if(DNA) {
					if (-1 == monkey4( pat, m, text+start, text+end, D , oldpat, oldm )) {
						free_buf(fd, text);
						memcpy(text+end+1, tempbuf, m); /* restore */
						free(tempbuf);
						return -1;
					}
				}
				else {
					if(m >= LONG_APPX) {
						if (-1 == a_monkey(pat, m, text+start, text+end, D, oldpat, oldm)) {
							free_buf(fd, text);
							memcpy(text+end+1, tempbuf, m); /* restore */
							free(tempbuf);
							return -1;
						}
					}
					else {
						if (-1 == agrep(pat, m, text+start, text+end, D, oldpat, oldm)) {
							free_buf(fd, text);
							memcpy(text+end+1, tempbuf, m); /* restore */
							free(tempbuf);
							return -1;
						}
					}
				}
			}
			if(FILENAMEONLY && (num_of_matched - prev_num_of_matched) && (NEW_FILE || !POST_FILTER)) {	/* externally set */
				if (agrep_finalfp != NULL)
					fprintf(agrep_finalfp, "%s\n", CurrentFileName);
				else {
					int outindex;
					for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
							(CurrentFileName[outindex] != '\0'); outindex++) {
						agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
					}
					if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer+1>=agrep_outlen)) {
						OUTPUT_OVERFLOW;
						free_buf(fd, text);
						memcpy(text+end+1, tempbuf, m); /* restore */
						free(tempbuf);
						return -1;
					}
					else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
					agrep_outpointer += outindex;
				}
				free_buf(fd, text);
				NEW_FILE = OFF;
			}

		memcpy(text+end+1, tempbuf, m); /* restore */
		free(tempbuf);
		return 0;
	}
#endif	/*AGREP_POINTER*/
} /* end sgrep */

/* SUN:

   BOYER-MOORE.
   
   Our implementation of bm assumes
   that the content of text[n]...text[n+m-1] is pat[m-1] (emergency stop)
   such that the skip loop is guaranteed to terminated.
      
*/

int bm(pat, m, text, textend, oldpat, oldm)

CHARTYPE *text, *textend, *pat, *oldpat;
int m, oldm;
{
	int PRINTED = 0;
	register int shift;
	register int  m1, j, d1; 
	CHARTYPE *textbegin = text;
	int newlen;
	CHARTYPE *textstart;
	CHARTYPE *curtextbegin;
	CHARTYPE *curtextend;
#if	MEASURE_TIMES
	struct timeval initt, finalt;
#endif
	CHARTYPE *lastout = text;

	d1 = shift_1;	/* at least 1 */
	m1 = m - 1;
	shift = 0;	/* to start with the skip loop: assume,
			   the first character is a match. */

#ifdef DEBUG2
	printf("***BM-START*** textend=%d=%c=\n",textend,*textend);
#endif

	/* The original loop was: while (text <= textend) [TG] 04.10.96 */

	while (text < textend) {
	
		textstart = text;
		
#ifdef DEBUG2
		printf("shift=%d text=%d=%c\n",shift,text,*text);
#endif	
		/* the skip-loop: skip until a match is found (shift=0) */
		while(shift) {
			shift = SHIFT[*(text += shift)];
#ifdef DEBUG2
			printf("shift=%d text=%d=%c\n",shift,text,*text);
#endif
		}
		
		CurrentByteOffset += text - textstart;

		j = 0;
		while(TR[pat[m1 - j]] == TR[*(text - j)]) {
			if(++j == m)  break;       /* if statement can be saved,
						      but for safety ... */
		}
		
		if (j == m ) {
		
			if(text > textend) return 0;
			
			if(WORDBOUND) {
				if(isalnum(*(text+1))) { shift=1 /*TG*/ ; goto CONT; }	/* as if there was no match */
				if(isalnum(*(text-m))) { shift=1 /*TG*/ ; goto CONT; }	/* as if there was no match */
				
				/* changed by Udi 11/7/94 to avoid having to set TR[] to W_delim */
			}

			if (TCOMPRESSED == ON) {
				/* Don't update CurrentByteOffset here: only before outputting properly */
				if (!DELIMITER) {
					curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
					if (*curtextbegin == '\n') curtextbegin ++;
					curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
					if (*curtextend == '\n') curtextend ++;
					
/*** [TG] *THE CODE BELOW MUST BE COPIED HERE ***/

				}
				else {
					curtextbegin = backward_delimiter(text, textbegin, tc_D_pattern, tc_D_length, OUTTAIL);
					curtextend = forward_delimiter(text+1, textend, tc_D_pattern, tc_D_length, OUTTAIL);
				}
			}
			else {
				/* Don't update CurrentByteOffset here: only before outputting properly */
				if (!DELIMITER) {
					curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
					if (*curtextbegin == '\n') curtextbegin ++;
					
					curtextend = text+1;
					while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
					if (*curtextend == '\n') curtextend ++;
					
					/* adjust for files without CR as last character;
					   this part is only needed for hits at the very last line. [TG] */

					if (curtextend >= textend) {
						curtextend=textend+1;
						if (*(curtextend-1) != '\n') *curtextend++='\n';
					}
					
				}
				else {
					curtextbegin = backward_delimiter(text, textbegin, D_pattern, D_length, OUTTAIL);
					curtextend = forward_delimiter(text+1, textend, D_pattern, D_length, OUTTAIL);
				}
			}

			if (TCOMPRESSED == ON) {
#if     MEASURE_TIMES
                                gettimeofday(&initt, NULL);
#endif  /*MEASURE_TIMES*/
				if (-1 == exists_tcompressed_word(pat, m, curtextbegin, text - curtextbegin + m, EASYSEARCH)) {
					shift = 1;	/* TG */
					goto CONT;	/* as if there was no match */
				}
#if     MEASURE_TIMES
                                gettimeofday(&finalt, NULL);
                                FILTERALGO_ms +=  (finalt.tv_sec *1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif  /*MEASURE_TIMES*/
			}

			textbegin = curtextend; /* (curtextend - 1 > textbegin ? curtextend - 1 : curtextend); */
			num_of_matched++;
			if(FILENAMEONLY) return 0;
			if(!COUNT) {
				if (!INVERSE) {
					if(FNAME && (NEW_FILE || !POST_FILTER)) {
						char	nextchar = (POST_FILTER == ON)?'\n':' ';
						char	*prevstring = (POST_FILTER == ON)?"\n":"";
						if (agrep_finalfp != NULL)
							fprintf(agrep_finalfp, "%s%s:%c", prevstring, CurrentFileName, nextchar);
						else {
							int outindex;
							if (prevstring[0] != '\0') {
								if(agrep_outpointer + 1 >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer ++] = prevstring[0];
							}
							for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
									(CurrentFileName[outindex] != '\0'); outindex++) {
								agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
							}
							if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer+2>=agrep_outlen)) {
								OUTPUT_OVERFLOW;
								return -1;
							}
							else {
								agrep_outbuffer[agrep_outpointer+outindex++] = ':';
								agrep_outbuffer[agrep_outpointer+outindex++] = nextchar;
							}
							agrep_outpointer += outindex;
						}
						NEW_FILE = OFF;
						PRINTED = 1;
					}

					if(BYTECOUNT) {
						if (agrep_finalfp != NULL)
							fprintf(agrep_finalfp, "%d= ", CurrentByteOffset);
						else {
							char s[32];
							int  outindex;
							sprintf(s, "%d=", CurrentByteOffset);
							for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
									(s[outindex] != '\0'); outindex++) {
								agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
							}
							if (s[outindex] != '\0') {
								OUTPUT_OVERFLOW;
								return -1;
							}
							agrep_outpointer += outindex;
						}
						PRINTED = 1;
					}

					if (PRINTOFFSET) {
						if (agrep_finalfp != NULL)
							fprintf(agrep_finalfp, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
						else {
							char s[32];
							int outindex;
							sprintf(s, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
							for (outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
									 (s[outindex] != '\0'); outindex ++) {
								agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
							}
							if (s[outindex] != '\0') {
								OUTPUT_OVERFLOW;
								return -1;
							}
							agrep_outpointer += outindex;
						}
						PRINTED = 1;
					}

					CurrentByteOffset += textbegin - text;

					text = textbegin;

					if (PRINTRECORD) {
					if (TCOMPRESSED == ON) {
#if	MEASURE_TIMES
						gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
						if (agrep_finalfp != NULL)
							newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);
						else {
							if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
								if (agrep_outpointer + newlen + 1 >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += newlen;
							}
						}
#if	MEASURE_TIMES
						gettimeofday(&finalt, NULL);
						OUTFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
					}
					else {
						if (agrep_finalfp != NULL) {
							fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);
						}
						else {
							if (agrep_outpointer + curtextend - curtextbegin >= agrep_outlen) {
								OUTPUT_OVERFLOW;
								return -1;
							}
							memcpy(agrep_outbuffer+agrep_outpointer, curtextbegin, curtextend-curtextbegin);
							agrep_outpointer += curtextend - curtextbegin;
						}
					}
					}
					else if (PRINTED) {
						if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
						else agrep_outbuffer[agrep_outpointer ++] = '\n';
						PRINTED = 0;
					}
				}
				else {	/* INVERSE */
					if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
						if (agrep_finalfp != NULL)
							newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_finalfp, -1, EASYSEARCH);
						else {
							if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
								if (newlen + agrep_outpointer >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += newlen;
							}
						}
						lastout=textbegin;
						CurrentByteOffset += textbegin - text;
						text = textbegin;
					}
					else { /* NOT TCOMPRESSED */
						if (agrep_finalfp != NULL)
							fwrite(lastout, 1, curtextbegin-lastout, agrep_finalfp);
						else {
							if (curtextbegin - lastout + agrep_outpointer >= agrep_outlen) {
								OUTPUT_OVERFLOW;
								return -1;
							}
							memcpy(agrep_outbuffer+agrep_outpointer, lastout, curtextbegin-lastout);
							agrep_outpointer += (curtextbegin - lastout);
						}
						lastout=textbegin;
						CurrentByteOffset += textbegin - text;
						text = textbegin;
					} /* TCOMPRESSED */
				} /* INVERSE */
			}
			else {	/* COUNT */
				CurrentByteOffset += textbegin - text;
				text = textbegin;
			}
			if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
			    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */

/* CONT: was here TG */
			/* shift = 1;			[del] [TG] */
			shift = SHIFT[*text];	/* 	[new] [TG] */
CONT:						/*	now here to restart the skip-loop */
#ifdef _WIN32
;
#endif 
;
		}
		else shift = d1;
	}

	if (INVERSE && !COUNT && (lastout <= textend)) {
		if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
			if (agrep_finalfp != NULL)
				newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_finalfp, -1, EASYSEARCH);
			else {
				if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
					if (newlen + agrep_outpointer >= agrep_outlen) {
						OUTPUT_OVERFLOW;
						return -1;
					}
					agrep_outpointer += newlen;
				}
			}
		}
		else { /* NOT TCOMPRESSED */
			if (agrep_finalfp != NULL)
				fwrite(lastout, 1, textend-lastout + 1, agrep_finalfp);
			else {
				if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				memcpy(agrep_outbuffer+agrep_outpointer, lastout, textend-lastout + 1);
				agrep_outpointer += (textend - lastout + 1);
			}
		} /* TCOMPRESSED */
	}

	return 0;
}


/* initmask() initializes the mask table for the pattern                    */ 
/* endposition is a mask for the endposition of the pattern                 */
/* endposition will contain k mask bits if the pattern contains k fragments */

static void initmask(pattern, Mask, m, D, endposition)

CHARTYPE *pattern; 
unsigned *Mask; 
register int m, D; 
unsigned *endposition;
{
	register unsigned Bit1, c;
	register int i, j, frag_num;

	/* Bit1 = 1 << 31;*/    /* the first bit of Bit1 is 1, others 0.  */
	Bit1 = (unsigned)0x80000000;
	frag_num = D+1; 
	*endposition = 0;
	for (i = 0; i < frag_num; i++) *endposition = *endposition | (Bit1 >> i);
	*endposition = *endposition >> (m - frag_num);
	for(i = 0; i < m; i++) 
		if (pattern[i] == '^' || pattern[i] == '$') {
			pattern[i] = '\n'; 
		}
	for(i = 0; i < MAXSYM; i++) Mask[i] = ~0;
	for(i = 0; i < m; i++)     /* initialize the mask table */
	{  
		c = pattern[i];
		for ( j = 0; j < m; j++)
			if( c == pattern[j] )
				Mask[c] = Mask[c] & ~( Bit1 >> j ) ;
	}
}

static void
prep(Pattern, M, D)             /* preprocessing for partitioning_bm */
CHARTYPE *Pattern;  /* can be fine-tuned to choose a better partition */
register int M, D;
{
	register int i, j, k, p, shift;
	register unsigned m;
	unsigned hash, b_size = 3;
	m = M/(D+1);
	p = M - m*(D+1);
	for (i = 0; i < MAXSYM; i++) SHIFT[i] = m;
	for (i = M-1; i>=p ; i--) {
		shift = (M-1-i)%m;
		hash = Pattern[i];
		if((int)(SHIFT[hash]) > (int)(shift)) SHIFT[hash] = shift;
	}
#ifdef DEBUG
	for(i=0; i<M; i++) printf(" %d,", SHIFT[Pattern[i]]);
	printf("\n");
#endif
	shift_1 = m;
	for(i=0; i<D+1; i++) {
		j = M-1 - m*i;
		for(k=1; k<m; k++) {
			for(p=0; p<D+1; p++) 
				if(Pattern[j-k] == Pattern[M-1-m*p]) 
					if(k < shift_1) shift_1 = k;
		}
	}
#ifdef DEBUG
	printf("\nshift_1 = %d", shift_1);
#endif
	if(shift_1 == 0) shift_1 = 1;
	for(i=0; i<MAXMEMBER; i++) MEMBER[i] = 0;
	if (m < 3) b_size = m;
	for(i=0; i<D+1; i++) {
		j = M-1 - m*i;
		hash = 0;
		for(k=0; k<b_size; k++) {
			hash = (hash << 2) + Pattern[j-k];
		}
#ifdef DEBUG
		printf(" hash = %d,", hash);
#endif
		MEMBER[hash] = 1;
	}
}

int
agrep( pat, M, text, textend, D, oldpat, oldM) 
int M, D, oldM; 
register CHARTYPE *text, *textend, *pat, *oldpat;
{
	register int i;
	register int m = M/(D+1);
	register CHARTYPE *textbegin;
	CHARTYPE *textstart;
	register int shift, HASH;
	int  j=0, k, d1;
	int  n, cdx;
	int  Candidate[MaxCan][2], round, lastend=0;
	unsigned R1[MaxError+1], R2[MaxError+1]; 
	register unsigned int r1, endpos, c; 
	unsigned currentpos;
	unsigned Bit1;
	unsigned r_newline;
	int oldbyteoffset;
	CHARTYPE *lastout = text;
	int newlen;

	Candidate[0][0] = Candidate[0][1] = 0; 
	d1 = shift_1;
	cdx = 0;
	if(m < 3) r1 = m;
	else r1 = 3;
	textbegin = text;
	shift = m-1;
	while (text < textend) {
		textstart = text;
		shift = SHIFT[*(text += shift)];
		while(shift) {
			shift = SHIFT[*(text += shift)];
			shift = SHIFT[*(text += shift)];
		}
		CurrentByteOffset += text - textstart;
		j = 1; 
		HASH = *text;
		while(j < r1) { 
			HASH = (HASH << 2) + *(text-j);
			j++; 
		}
		if (MEMBER[HASH]) { 
			i = text - textbegin;
			if((i - M - D - 10) > Candidate[cdx][1]) { 	
				Candidate[++cdx][0] = i-M-D-2;
				Candidate[cdx][1] = i+M+D; 
			}
			else Candidate[cdx][1] = i+M+D;
			shift = d1;
		}
		else shift = d1;
	}

	CurrentByteOffset += (textbegin - text);
	text = textbegin;
	n = textend - textbegin;
	r_newline = '\n';
	/* for those candidate areas, find the D-error matches                     */
	if(Candidate[1][0] < 0) Candidate[1][0] = 0;
	endpos = endposition;                /* the mask table and the endposition */
	/* Bit1 = (1 << 31); */
	Bit1 = (unsigned)0x80000000;
	oldbyteoffset = CurrentByteOffset;
	for(round = 0; round <= cdx; round++)
	{  
		i = Candidate[round][0] ; 
		if(Candidate[round][1] > n) Candidate[round][1] = n;
		if(i < 0) i = 0;
		CurrentByteOffset = oldbyteoffset+i;
		R1[0] = R2[0] = ~0;
		R1[1] = R2[1] = ~Bit1;
		for(k = 1; k <= D; k++) R1[k] = R2[k] = (R1[k-1] >> 1) & R1[k-1];
		while (i < Candidate[round][1])                     
		{  
			c = text[i++];
			CurrentByteOffset ++;
			if(c == r_newline) {
				for(k = 0 ; k <= D; k++) R1[k] = R2[k] = (~0 );
			}
			r1 = Mask[c];
			R1[0] = (R2[0] >> 1) | r1;
			for(k=1; k<=D; k++)
				R1[k] = ((R2[k] >> 1) | r1) & R2[k-1] & ((R1[k-1] & R2[k-1]) >> 1);
			if((R1[D] & endpos) == 0) { 
				num_of_matched++;
				if(FILENAMEONLY) return 0; 
				currentpos = i;
				if(i <= lastend) {
					CurrentByteOffset += lastend - i;
					i = lastend;
				}
				else {
					int oldcurrentpos = currentpos;
					if (-1 == s_output(text, &currentpos, textbegin, textend, &lastout, pat, M, oldpat, oldM)) return -1;
					CurrentByteOffset += currentpos - oldcurrentpos;
					i = currentpos; 
				}
				lastend = i;
				for(k=0; k<=D; k++) R1[k] = R2[k] = ~0;
				if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
				    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */
			}

			/* copying the code to save a few instructions.
			you need to understand the shift-or algorithm
			to figure this one... */

			c = text[i++];
			CurrentByteOffset ++;
			if(c == r_newline) {
				for(k = 0 ; k <= D; k++) R1[k] = R2[k] = (~0 );
			}
			r1 = Mask[c];
			R2[0] = (R1[0] >> 1) | r1;
			for(k = 1; k <= D; k++)
				R2[k] = ((R1[k] >> 1) | r1) & R1[k-1] & ((R1[k-1] & R2[k-1]) >> 1);
			if((R2[D] & endpos) == 0) { 
				currentpos = i;
				num_of_matched++;
				if(FILENAMEONLY) return 0; 
				if(i <= lastend) {
					CurrentByteOffset += lastend - i;
					i = lastend;
				}
				else {
					int oldcurrentpos = currentpos;
					if (-1 == s_output(text, &currentpos, textbegin, textend, &lastout, pat, M, oldpat, oldM)) return -1;
					CurrentByteOffset += currentpos - oldcurrentpos;
					i = currentpos; 
				}
				lastend = i;
				for(k=0; k<=D; k++) R1[k] = R2[k] = ~0;
				if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
				    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */
			}
		}
	}


	if (INVERSE && !COUNT && (lastout <= textend)) {
		if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
			if (agrep_finalfp != NULL)
				newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_finalfp, -1, EASYSEARCH);
			else {
				if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
					if (newlen + agrep_outpointer >= agrep_outlen) {
						OUTPUT_OVERFLOW;
						return -1;
					}
					agrep_outpointer += newlen;
				}
			}
		}
		else { /* NOT TCOMPRESSED */
			if (agrep_finalfp != NULL)
				fwrite(lastout, 1, textend-lastout + 1, agrep_finalfp);
			else {
				if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				memcpy(agrep_outbuffer+agrep_outpointer, lastout, textend-lastout + 1);
				agrep_outpointer += (textend - lastout + 1);
			}
		} /* TCOMPRESSED */
	}

	return 0;
}

/* Don't update CurrentByteOffset here: done by caller */
int
s_output(text, i, textbegin, textend, lastout, pat, m, oldpat, oldm) 
int *i;	/* in, out */
int m, oldm; 
CHARTYPE *text, *textbegin, *textend, *pat, *oldpat;
CHARTYPE **lastout;	/* in, out */
{
	int PRINTED = 0;
	int newlen; 
	int oldi;
	CHARTYPE *curtextbegin;
	CHARTYPE *curtextend;
#if	MEASURE_TIMES
	struct timeval initt, finalt;
#endif

	if(SILENT) return 0;
	if (TCOMPRESSED == ON) {
		if (!DELIMITER) {
			curtextbegin = text + *i; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
			if (*curtextbegin == '\n') curtextbegin ++;
			curtextend = text + *i /* + 1 agrep() has i++ */; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
			if (*curtextend == '\n') curtextend ++;
		}
		else {
			curtextbegin = backward_delimiter(text + *i, text, tc_D_pattern, tc_D_length, OUTTAIL);
			curtextend = forward_delimiter(text + *i /* + 1 agrep() has i++ */, textend, tc_D_pattern, tc_D_length, OUTTAIL);
		}
	}
	else {
		if (!DELIMITER) {
			curtextbegin = text + *i; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
			if (*curtextbegin == '\n') curtextbegin ++;
			curtextend = text + *i /* + 1 agrep() has i++ */; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
			if (*curtextend == '\n') curtextend ++;
		}
		else {
			curtextbegin = backward_delimiter(text + *i, text, D_pattern, D_length, OUTTAIL);
			curtextend = forward_delimiter(text + *i /* + 1 agrep() has i++ */, textend, D_pattern, D_length, OUTTAIL);
		}
	}

	if (TCOMPRESSED == ON) {
#if     MEASURE_TIMES
		gettimeofday(&initt, NULL);
#endif  /*MEASURE_TIMES*/
		if (-1 == exists_tcompressed_word(pat, m, curtextbegin, text  + *i - curtextbegin + m, EASYSEARCH)) {
			num_of_matched --;
			return 0;
		}
#if     MEASURE_TIMES
		gettimeofday(&finalt, NULL);
		FILTERALGO_ms +=  (finalt.tv_sec *1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif  /*MEASURE_TIMES*/
	}

	textbegin = curtextend; /*(curtextend - 1 > textbegin ? curtextend - 1 : curtextend); */
	oldi = *i;
	*i += textbegin - (text + *i);
	if(COUNT) return 0;


	if (INVERSE) {
		if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
			if (agrep_finalfp != NULL)
				newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, *lastout, curtextbegin - *lastout, agrep_finalfp, -1, EASYSEARCH);
			else {
				if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, *lastout, curtextbegin - *lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
					if (newlen + agrep_outpointer >= agrep_outlen) {
						OUTPUT_OVERFLOW;
						return -1;
					}
					agrep_outpointer += newlen;
				}
			}
			*lastout=textbegin;
			CurrentByteOffset += textbegin - text;
			text = textbegin;
		}
		else { /* NOT TCOMPRESSED */
			if (agrep_finalfp != NULL)
				fwrite(*lastout, 1, curtextbegin-*lastout, agrep_finalfp);
			else {
				if (curtextbegin - *lastout + agrep_outpointer >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				memcpy(agrep_outbuffer+agrep_outpointer, *lastout, curtextbegin-*lastout);
				agrep_outpointer += (curtextbegin - *lastout);
			}
			*lastout=textbegin;
			CurrentByteOffset += textbegin - text;
			text = textbegin;
		} /* TCOMPRESSED */
		return 0;
	}

	if(FNAME && (NEW_FILE || !POST_FILTER)) {
		char	nextchar = (POST_FILTER == ON)?'\n':' ';
		char	*prevstring = (POST_FILTER == ON)?"\n":"";
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%s%s:%c", prevstring, CurrentFileName, nextchar);
		else {
			int outindex;
			if (prevstring[0] != '\0') {
				if(agrep_outpointer + 1 >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				else agrep_outbuffer[agrep_outpointer ++] = prevstring[0];
			}
			for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
					(CurrentFileName[outindex] != '\0'); outindex++) {
				agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
			}
			if ((CurrentFileName[outindex] != '\0') || (outindex + agrep_outpointer + 2 >= agrep_outlen)) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			agrep_outbuffer[agrep_outpointer + outindex++] = ':';
			agrep_outbuffer[agrep_outpointer + outindex++] = nextchar;
			agrep_outpointer += outindex;
		}
		NEW_FILE = OFF;
		PRINTED = 1;
	}

	if(BYTECOUNT) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%d= ", CurrentByteOffset);
		else {
			char s[32];
			int  outindex;
			sprintf(s, "%d= ", CurrentByteOffset);
			for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
					(s[outindex] != '\0'); outindex++) {
				agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
			}
			if (s[outindex] != '\0') {
				OUTPUT_OVERFLOW;
				return -1;
			}
			agrep_outpointer += outindex;
		}
		PRINTED = 1;
	}

	if (PRINTOFFSET) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "@%d{%d} ", CurrentByteOffset - (text + oldi-curtextbegin), curtextend-curtextbegin);
		else {
			char s[32];
			int outindex;
			sprintf(s, "@%d{%d} ", CurrentByteOffset - (text + oldi-curtextbegin), curtextend-curtextbegin);
			for (outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
					 (s[outindex] != '\0'); outindex ++) {
				agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
			}
			if (s[outindex] != '\0') {
				OUTPUT_OVERFLOW;
				return -1;
			}
			agrep_outpointer += outindex;
		}
		PRINTED = 1;
	}
	if (PRINTRECORD) {

	if (TCOMPRESSED == ON) {
#if	MEASURE_TIMES
		gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
		if (agrep_finalfp != NULL) {
			newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);
		}
		else {
			if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
				if (agrep_outpointer + newlen + 1 >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				agrep_outpointer += newlen;
			}
		}
#if	MEASURE_TIMES
		gettimeofday(&finalt, NULL);
		OUTFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
	}
	else {
		if (agrep_finalfp != NULL) {
			fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);
		}
		else {
			if (agrep_outpointer + curtextend - curtextbegin >= agrep_outlen) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			memcpy(agrep_outbuffer + agrep_outpointer, curtextbegin, curtextend - curtextbegin);
			agrep_outpointer += curtextend - curtextbegin;
		}
	}
	}
	else if (PRINTED) {
		if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
		else agrep_outbuffer[agrep_outpointer ++] = '\n';
		PRINTED = 0;
	}
	return 0;
}

static void
prep_bm(Pattern, m)      
unsigned char *Pattern;
register m;
{
	int i;
	unsigned hash;
	unsigned char lastc;
	for (i = 0; i < MAXSYM; i++) SHIFT[i] = m;
	for (i = m-1; i>=0; i--) {
		hash = TR[Pattern[i]];
		if((int)(SHIFT[hash]) >= (int)(m - 1)) SHIFT[hash] = m-1-i;
	}
	shift_1 = m-1;

	/* shift_1 records the previous occurrence of the last character of
	the pattern. When we match this last character but do not have a match,
	we can shift until we reach the next occurrence from the right. */

	lastc = TR[Pattern[m-1]];

	for (i= m-2; i>=0; i--) {
	
		if(TR[Pattern[i]] == lastc )
		{ 
			shift_1 = m-1 - i;  
			i = -1; 
		}
	}
	
	if(shift_1 == 0) shift_1 = 1; /* can never happen - Udi 11/7/94 */

	/* if(NOUPPER) */ for(i=0; i<MAXSYM; i++) {

#if (defined(__EMX__) && defined(ISO_CHAR_SET))

		SHIFT[i] = SHIFT[LUT[i]];
#else
		if (isupper(i)) SHIFT[i] = SHIFT[tolower(i)];
#endif
	}
	
#ifdef DEBUG
	for(i=0; i<MAXSYM; i++) {
		printf("%c:%d ", i, SHIFT[i]); 
		if ((i % 8) == 0) printf("\n");
	}
	printf("\n");
#endif
}

/* monkey uses two characters for delta_1 shifting */

CHARTYPE SHIFT_2[MAX_SHIFT_2];

int
monkey( pat, m, text, textend  ) 
register int m  ; 
register CHARTYPE *text, *textend, *pat;
{
	int PRINTED = 0;
	register unsigned hash;
	register CHARTYPE shift;
	register int  m1, j; 
	CHARTYPE *textbegin = text;
	CHARTYPE *textstart;
	int newlen;
	CHARTYPE *curtextbegin;
	CHARTYPE *curtextend;
#if	MEASURE_TIMES
	struct timeval initt, finalt;
#endif
	CHARTYPE *lastout = text;

	m1 = m - 1;
	text = text+m1;
	CurrentByteOffset += m1;
	
	while (text < textend) {
		textstart = text;
		hash = TR[*text];
		hash = (hash << 3) + TR[*(text-1)];
		shift = SHIFT_2[hash];
		while(shift) {
			text = text + shift;
			hash = (TR[*text] << 3) + TR[*(text-1)];
			shift = SHIFT_2[hash];
		}
		CurrentByteOffset += text - textstart;
		j = 0;
		while(TR[pat[m1 - j]] == TR[*(text - j)]) { 
			if(++j == m) break; 
		}
		
		if (j == m ) {
		
			if(text > textend) return 0; /* Udi: used to be >= for some reason */
			
		  	/* added by Udi 11/7/94 */
			
			if(WORDBOUND) {
				if(isalnum(*(text+1))) {shift=1/*TG*/; goto CONT; }	/* as if there was no match */
				if(isalnum(*(text-m))) {shift=1/*TG*/; goto CONT; }	/* as if there was no match */
				
				/* changed by Udi 11/7/94 to avoid having to set TR[] to W_delim */
			}

			if (TCOMPRESSED == ON) {
				/* Don't update CurrentByteOffset here: only before outputting properly */
				if (!DELIMITER) {
					curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
					if (*curtextbegin == '\n') curtextbegin ++;
					curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
					if (*curtextend == '\n') curtextend ++;
				}
				else {
					curtextbegin = backward_delimiter(text, textbegin, tc_D_pattern, tc_D_length, OUTTAIL);
					curtextend = forward_delimiter(text + 1, textend, tc_D_pattern, tc_D_length, OUTTAIL);
				}
			}
			else {

				/* Don't update CurrentByteOffset here: only before outputting properly */

				if (!DELIMITER) {
					curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
					if (*curtextbegin == '\n') curtextbegin ++;
					curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
					if (*curtextend == '\n') curtextend ++;
				}
				else {
					curtextbegin = backward_delimiter(text, textbegin, D_pattern, D_length, OUTTAIL);
					curtextend = forward_delimiter(text + 1, textend, D_pattern, D_length, OUTTAIL);
				}
			}

			if (TCOMPRESSED == ON) {

#if     MEASURE_TIMES
                                gettimeofday(&initt, NULL);
#endif  /*MEASURE_TIMES*/
				if (-1 == exists_tcompressed_word(pat, m, curtextbegin, text - curtextbegin + m, EASYSEARCH)) {
					shift = 1;
					goto CONT;	/* as if there was no match */
				}
#if     MEASURE_TIMES
                                gettimeofday(&finalt, NULL);
                                FILTERALGO_ms +=  (finalt.tv_sec *1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif  /*MEASURE_TIMES*/
			}

			textbegin = curtextend; /*(curtextend - 1 > textbegin ? curtextend - 1 : curtextend); */
			num_of_matched++;
			if(FILENAMEONLY)  return 0;
			if (!COUNT) {
				if (!INVERSE) {
					if(FNAME && (NEW_FILE || !POST_FILTER)) {
						char	nextchar = (POST_FILTER == ON)?'\n':' ';
						char	*prevstring = (POST_FILTER == ON)?"\n":"";
						if (agrep_finalfp != NULL)
							fprintf(agrep_finalfp, "%s%s:%c", prevstring, CurrentFileName, nextchar);
						else {
							int outindex;
							if (prevstring[0] != '\0') {
								if(agrep_outpointer + 1 >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer ++] = prevstring[0];
							}
							for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
									(CurrentFileName[outindex] != '\0'); outindex++) {
								agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
							}
							if ((CurrentFileName[outindex] != '\0') || (outindex + agrep_outpointer + 2 >= agrep_outlen)) {
								OUTPUT_OVERFLOW;
								return -1;
							}
							agrep_outbuffer[agrep_outpointer + outindex++] = ':';
							agrep_outbuffer[agrep_outpointer + outindex++] = nextchar;
							agrep_outpointer += outindex;
						}
						NEW_FILE = OFF;
						PRINTED = 1;
					}

					if(BYTECOUNT) {
						if (agrep_finalfp != NULL)
							fprintf(agrep_finalfp, "%d= ", CurrentByteOffset);
						else {
							char s[32];
							int  outindex;
							sprintf(s, "%d= ", CurrentByteOffset);
							for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
									(s[outindex] != '\0'); outindex++) {
								agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
							}
							if (s[outindex] != '\0') {
								OUTPUT_OVERFLOW;
								return -1;
							}
							agrep_outpointer += outindex;
						}
						PRINTED = 1;
					}

					if (PRINTOFFSET) {
						if (agrep_finalfp != NULL)
							fprintf(agrep_finalfp, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
						else {
							char s[32];
							int outindex;
							sprintf(s, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
							for (outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
									 (s[outindex] != '\0'); outindex ++) {
								agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
							}
							if (s[outindex] != '\0') {
								OUTPUT_OVERFLOW;
								return -1;
							}
							agrep_outpointer += outindex;
						}
						PRINTED = 1;
					}

					CurrentByteOffset += textbegin - text;
					text = textbegin;

					if (PRINTRECORD) {
					if (TCOMPRESSED == ON) {
#if	MEASURE_TIMES
						gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
						if (agrep_finalfp != NULL)
							newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);
						else {
							if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
								if (agrep_outpointer + newlen + 1 >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += newlen;
							}
						}
#if	MEASURE_TIMES
						gettimeofday(&finalt, NULL);
						OUTFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
					}
					else {
						if (agrep_finalfp != NULL) {
							fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);
						}
						else {
							if (agrep_outpointer + curtextend - curtextbegin >= agrep_outlen) {
								OUTPUT_OVERFLOW;
								return -1;
							}
							memcpy(agrep_outbuffer+agrep_outpointer, curtextbegin, curtextend-curtextbegin);
							agrep_outpointer += curtextend - curtextbegin;
						}
					}
					}
					else if (PRINTED) {
						if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
						else agrep_outbuffer[agrep_outpointer ++] = '\n';
						PRINTED = 0;
					}
				}
				else {	/* INVERSE */
					if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
						if (agrep_finalfp != NULL)
							newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_finalfp, -1, EASYSEARCH);
						else {
							if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
								if (newlen + agrep_outpointer >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += newlen;
							}
						}
						lastout=textbegin;
						CurrentByteOffset += textbegin - text;
						text = textbegin;
					}
					else { /* NOT TCOMPRESSED */
						if (agrep_finalfp != NULL)
							fwrite(lastout, 1, curtextbegin-lastout, agrep_finalfp);
						else {
							if (curtextbegin - lastout + agrep_outpointer >= agrep_outlen) {
								OUTPUT_OVERFLOW;
								return -1;
							}
							memcpy(agrep_outbuffer+agrep_outpointer, lastout, curtextbegin-lastout);
							agrep_outpointer += (curtextbegin - lastout);
						}
						lastout=textbegin;
						CurrentByteOffset += textbegin - text;
						text = textbegin;
					} /* TCOMPRESSED */
				} /* INVERSE */
			}
			else {	/* COUNT */
				CurrentByteOffset += textbegin - text;
				text = textbegin;
			}

			/* Counteract the ++ below */
			text --;
			CurrentByteOffset --;
			if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
			    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */
		}
	CONT:
		text++;
		CurrentByteOffset ++;
	}

	if (INVERSE && !COUNT && (lastout <= textend)) {
		if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
			if (agrep_finalfp != NULL)
				newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_finalfp, -1, EASYSEARCH);
			else {
				if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
					if (newlen + agrep_outpointer >= agrep_outlen) {
						OUTPUT_OVERFLOW;
						return -1;
					}
					agrep_outpointer += newlen;
				}
			}
		}
		else { /* NOT TCOMPRESSED */
			if (agrep_finalfp != NULL)
				fwrite(lastout, 1, textend-lastout + 1, agrep_finalfp);
			else {
				if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				memcpy(agrep_outbuffer+agrep_outpointer, lastout, textend-lastout + 1);
				agrep_outpointer += (textend - lastout + 1);
			}
		} /* TCOMPRESSED */
	}

	return 0;
}

/* a_monkey() the approximate monkey move */

int
a_monkey( pat, m, text, textend, D )

register int m, D ; 
register CHARTYPE *text, *textend, *pat;
{
	int PRINTED = 0;
	register CHARTYPE *oldtext;
	CHARTYPE *curtextbegin;
	CHARTYPE *curtextend;
	register unsigned hash, hashmask, suffix_error; 
	register int  m1 = m-1-D, pos; 
	CHARTYPE *textbegin = text;
	CHARTYPE *textstart;
	CHARTYPE *lastout = text;
	int newlen;

	hashmask = Hashmask;
	oldtext  = text;

	while (text < textend) {
		textstart = text;
		text = text+m1;
		suffix_error = 0;
		while(suffix_error <= D) {
			hash = *text--;
			while(MEMBER_1[hash]) {
				hash = ((hash << LOG_ASCII) + *(text--)) & hashmask;
			}
			suffix_error++;
		}
		CurrentByteOffset += text - textstart;
		if(text <= oldtext) {
			if((pos = verify(m, 2*m+D, D, pat, oldtext)) > 0)  {
				CurrentByteOffset += (oldtext+pos - text);
				text = oldtext+pos;
				if(text > textend) return 0;

				/* Don't update CurrentByteOffset here: only before outputting properly */
				if (TCOMPRESSED == ON) {
					if (!DELIMITER) {
						curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
						if (*curtextbegin == '\n') curtextbegin ++;
						curtextend = text + 1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
						if (*curtextend == '\n') curtextend ++;
					}
					else {
						curtextbegin = backward_delimiter(text, textbegin, tc_D_pattern, tc_D_length, OUTTAIL);
						curtextend = forward_delimiter(text + 1, textend, tc_D_pattern, tc_D_length, OUTTAIL);
					}
				}
				else {
					if (!DELIMITER) {
						curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
						if (*curtextbegin == '\n') curtextbegin ++;
						curtextend = text + 1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
						if (*curtextend == '\n') curtextend ++;
					}
					else {
						curtextbegin = backward_delimiter(text, textbegin, D_pattern, D_length, OUTTAIL);
						curtextend = forward_delimiter(text + 1, textend, D_pattern, D_length, OUTTAIL);
					}
				}
				textbegin = curtextend; /* (curtextend - 1 > textbegin ? curtextend - 1 : curtextend); */

				num_of_matched++;
				if(FILENAMEONLY) return 0;
				if(!COUNT) {
					if (!INVERSE) {
						if(FNAME && (NEW_FILE || !POST_FILTER)) {
							char	nextchar = (POST_FILTER == ON)?'\n':' ';
							char	*prevstring = (POST_FILTER == ON)?"\n":"";
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s%s:%c", prevstring, CurrentFileName, nextchar);
							else {
								int outindex;
								if (prevstring[0] != '\0') {
									if(agrep_outpointer + 1 >= agrep_outlen) {
										OUTPUT_OVERFLOW;
										return -1;
									}
									else agrep_outbuffer[agrep_outpointer ++] = prevstring[0];
								}
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex + agrep_outpointer + 2 >= agrep_outlen)) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outbuffer[agrep_outpointer + outindex++] = ':';
								agrep_outbuffer[agrep_outpointer + outindex++] = nextchar;
								agrep_outpointer += outindex;
							}
							NEW_FILE = OFF;
							PRINTED = 1;
						}

						if(BYTECOUNT) {
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%d= ", CurrentByteOffset);
							else {
								char s[32];
								int  outindex;
								sprintf(s, "%d= ", CurrentByteOffset);
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(s[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += outindex;
							}
							PRINTED = 1;
						}

						if (PRINTOFFSET) {
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
							else {
								char s[32];
								int outindex;
								sprintf(s, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
								for (outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										 (s[outindex] != '\0'); outindex ++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += outindex;
							}
							PRINTED = 1;
						}

						CurrentByteOffset += textbegin - text;
						text = textbegin;

						if (PRINTRECORD) {
						if (TCOMPRESSED == ON) {
#if	MEASURE_TIMES
							gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
							if (agrep_finalfp != NULL)
								newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);
							else {
								if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
									if (agrep_outpointer + newlen + 1 >= agrep_outlen) {
										OUTPUT_OVERFLOW;
										return -1;
									}
									agrep_outpointer += newlen;
								}
							}
#if	MEASURE_TIMES
							gettimeofday(&finalt, NULL);
							OUTFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
						}
						else {
							if (agrep_finalfp != NULL) {
								fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);
							}
							else {
								if (agrep_outpointer + curtextend - curtextbegin >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								memcpy(agrep_outbuffer+agrep_outpointer, curtextbegin, curtextend-curtextbegin);
								agrep_outpointer += curtextend - curtextbegin;
							}
						}
						}
						else if (PRINTED) {
							if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
							else agrep_outbuffer[agrep_outpointer ++] = '\n';
							PRINTED = 0;
						}
					}
					else {	/* INVERSE */
						if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
							if (agrep_finalfp != NULL)
								newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_finalfp, -1, EASYSEARCH);
							else {
								if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
									if (newlen + agrep_outpointer >= agrep_outlen) {
										OUTPUT_OVERFLOW;
										return -1;
									}
									agrep_outpointer += newlen;
								}
							}
							lastout=textbegin;
							CurrentByteOffset += textbegin - text;
							text = textbegin;
						}
						else { /* NOT TCOMPRESSED */
							if (agrep_finalfp != NULL)
								fwrite(lastout, 1, curtextbegin-lastout, agrep_finalfp);
							else {
								if (curtextbegin - lastout + agrep_outpointer >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								memcpy(agrep_outbuffer+agrep_outpointer, lastout, curtextbegin-lastout);
								agrep_outpointer += (curtextbegin - lastout);
							}
							lastout=textbegin;
							CurrentByteOffset += textbegin - text;
							text = textbegin;
						} /* TCOMPRESSED */
					}
				}
				else {	/* COUNT */
					CurrentByteOffset += textbegin - text;
					text = textbegin;
				}
				if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
				    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */
			}
			else {
				CurrentByteOffset += (oldtext + m - text);
				text = oldtext + m;
			}
		}
		oldtext = text;
	}

	if (INVERSE && !COUNT && (lastout <= textend)) {
		if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
			if (agrep_finalfp != NULL)
				newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_finalfp, -1, EASYSEARCH);
			else {
				if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
					if (newlen + agrep_outpointer >= agrep_outlen) {
						OUTPUT_OVERFLOW;
						return -1;
					}
					agrep_outpointer += newlen;
				}
			}
		}
		else { /* NOT TCOMPRESSED */
			if (agrep_finalfp != NULL)
				fwrite(lastout, 1, textend-lastout + 1, agrep_finalfp);
			else {
				if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				memcpy(agrep_outbuffer+agrep_outpointer, lastout, textend-lastout + 1);
				agrep_outpointer += (textend - lastout + 1);
			}
		} /* TCOMPRESSED */
	}

	return 0;
}

static void
am_preprocess(Pattern)

CHARTYPE *Pattern;
{
	int i, m;
	m = strlen(Pattern);
	for (i = 1, Hashmask = 1 ; i<16 ; i++) Hashmask = (Hashmask << 1) + 1 ;
	for (i = 0; i < MAXMEMBER_1; i++) MEMBER_1[i] = 0;
	for (i = m-1; i>=0; i--) {
		MEMBER_1[Pattern[i]] = 1;
	}
	for (i = m-1; i > 0; i--) {
		MEMBER_1[(Pattern[i] << LOG_ASCII) + Pattern[i-1]] = 1;
	}
}

int
verify(m, n, D, pat, text)

register int m, n, D;
CHARTYPE *pat, *text;
{   
	int A[MAXPATT], B[MAXPATT];
	register int last = D;      
	register int cost = 0;  
	register int k, i, c;
	register int m1 = m+1;
	CHARTYPE *textend = text+n;
	CHARTYPE *textbegin = text;

	for (i = 0; i <= m1; i++)  A[i] = B[i] = i;
	while (text < textend)
	{
		for (k = 1; k <= last; k++)
		{
			cost = B[k-1]+1; 
			if (pat[k-1] != *text)
			{   
				if (B[k]+1 < cost) cost = B[k]+1; 
				if (A[k-1]+1 < cost) cost = A[k-1]+1; 
			}
			else cost = cost -1; 
			A[k] = cost; 
		}
		if(pat[last] == *text++) { 
			A[last+1] = B[last]; 
			last++; 
		}
		if(A[last] < D) A[last+1] = A[last++]+1;
		while (A[last] > D) last = last - 1;
		if(last >= m) return(text - textbegin - 1);
		if(*text == '\n') {
			last = D;
			for(c = 0; c<=m1; c++) A[c] = B[c] = c;
		}
		for (k = 1; k <= last; k++)
		{
			cost = A[k-1]+1; 
			if (pat[k-1] != *text)
			{   
				if (A[k]+1 < cost) cost = A[k]+1; 
				if (B[k-1]+1 < cost) cost = B[k-1]+1; 
			}
			else cost = cost -1; 
			B[k] = cost;
		}
		if(pat[last] == *text++) { 
			B[last+1] = A[last]; 
			last++; 
		}
		if(B[last] < D) B[last+1] = B[last++]+1;
		while (B[last] > D) last = last -1;
		if(last >= m)   return(text - textbegin - 1);
		if(*text == '\n') {
			last = D;
			for(c = 0; c<=m1; c++) A[c] = B[c] = c;
		}
	}    
	return(0);
}


/* preprocessing for monkey()   */

static void
m_preprocess(Pattern)

CHARTYPE *Pattern;
{
	int i, j, m;
	unsigned hash;
	m = strlen(Pattern);
	for (i = 0; i < MAX_SHIFT_2; i++) SHIFT_2[i] = m;
	for (i = m-1; i>=1; i--) {
		hash = TR[Pattern[i]];
		hash = hash << 3;
		for (j = 0; j< MAXSYM; j++) {
			if(SHIFT_2[hash+j] == m) SHIFT_2[hash+j] = m-1;
		}
		hash = hash + TR[Pattern[i-1]];
		if((int)(SHIFT_2[hash]) >= (int)(m - 1)) SHIFT_2[hash] = m-1-i;
	}
	shift_1 = m-1;
	for (i= m-2; i>=0; i--) {
		if(TR[Pattern[i]] == TR[Pattern[m-1]] )
		{ 
			shift_1 = m-1 - i;  
			i = -1; 
		}
	}
	if(shift_1 == 0) shift_1 = 1;
	SHIFT_2[0] = 0;
}

/* monkey4() the approximate monkey move */

char *MEMBER_D = NULL;

int
monkey4( pat, m, text, textend, D  ) 

register int m, D ; 
register unsigned char *text, *pat, *textend;
{
	int PRINTED = 0;
	register unsigned char *oldtext;
	register unsigned hash, hashmask, suffix_error; 
	register int m1=m-1-D, pos; 
	CHARTYPE *textbegin = text;
	CHARTYPE *textstart;
	CHARTYPE *curtextbegin;
	CHARTYPE *curtextend;
	CHARTYPE *lastout = text;
	int newlen;

	hashmask = Hashmask;
	oldtext = text ;
	while (text < textend) {
		textstart = text;
		text = text + m1;
		suffix_error = 0;
		while(suffix_error <= D) {
			hash = char_map[*text--];
			hash = ((hash << LOG_DNA) + char_map[*(text--)]) & hashmask;
			while(MEMBER_D[hash]) {
				hash = ((hash << LOG_DNA) + char_map[*(text--)]) & hashmask;
			}
			suffix_error++;
		}
		CurrentByteOffset += text - textstart;
		if(text <= oldtext) {
			if((pos = verify(m, 2*m+D, D, pat, oldtext)) > 0)  {
				CurrentByteOffset += (oldtext+pos - text);
				text = oldtext+pos;
				if(text > textend) return 0;

				if (TCOMPRESSED == ON) {
					/* Don't update CurrentByteOffset here: only before outputting properly */
					if (!DELIMITER) {
						curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
						if (*curtextbegin == '\n') curtextbegin ++;
						curtextend = text + 1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
						if (*curtextend == '\n') curtextend ++;
					}
					else {
						curtextbegin = backward_delimiter(text, textbegin, tc_D_pattern, tc_D_length, OUTTAIL);
						curtextend = forward_delimiter(text + 1, textend, tc_D_pattern, tc_D_length, OUTTAIL);
					}
				}
				else {
					/* Don't update CurrentByteOffset here: only before outputting properly */
					if (!DELIMITER) {
						curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
						if (*curtextbegin == '\n') curtextbegin ++;
						curtextend = text + 1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
						if (*curtextend == '\n') curtextend ++;
					}
					else {
						curtextbegin = backward_delimiter(text, textbegin, D_pattern, D_length, OUTTAIL);
						curtextend = forward_delimiter(text + 1, textend, D_pattern, D_length, OUTTAIL);
					}
				}
				textbegin = curtextend; /*(curtextend - 1 > textbegin ? curtextend - 1 : curtextend); */

				num_of_matched++;
				if(FILENAMEONLY) return 0;
				if(!COUNT) {
					if (!INVERSE) {
						if(FNAME && (NEW_FILE || !POST_FILTER)) {
							char	nextchar = (POST_FILTER == ON)?'\n':' ';
							char	*prevstring = (POST_FILTER == ON)?"\n":"";
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s%s:%c", prevstring, CurrentFileName, nextchar);
							else {
								int outindex;
								if (prevstring[0] != '\0') {
									if(agrep_outpointer + 1 >= agrep_outlen) {
										OUTPUT_OVERFLOW;
										return -1;
									}
									else agrep_outbuffer[agrep_outpointer ++] = prevstring[0];
								}
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex + agrep_outpointer + 2 >= agrep_outlen)) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outbuffer[agrep_outpointer + outindex++] = ':';
								agrep_outbuffer[agrep_outpointer + outindex++] = nextchar;
								agrep_outpointer += outindex;
							}
							NEW_FILE = OFF;
							PRINTED = 1;
						}

						if(BYTECOUNT) {
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%d= ", CurrentByteOffset);
							else {
								char s[32];
								int  outindex;
								sprintf(s, "%d= ", CurrentByteOffset);
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(s[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += outindex;
							}
							PRINTED = 1;
						}

						if (PRINTOFFSET) {
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
							else {
								char s[32];
								int outindex;
								sprintf(s, "@%d{%d} ", CurrentByteOffset - (text -curtextbegin), curtextend-curtextbegin);
								for (outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										 (s[outindex] != '\0'); outindex ++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									return -1;
								}
								agrep_outpointer += outindex;
							}
							PRINTED = 1;
						}

						CurrentByteOffset += textbegin + 1 - text;
						text = textbegin + 1;

						if (PRINTRECORD) {
						if (TCOMPRESSED == ON) {
#if	MEASURE_TIMES
							gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
							if (agrep_finalfp != NULL)
								newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);
							else {
								if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
									if (agrep_outpointer + newlen + 1 >= agrep_outlen) {
										OUTPUT_OVERFLOW;
										return -1;
									}
									agrep_outpointer += newlen;
								}
							}
#if	MEASURE_TIMES
							gettimeofday(&finalt, NULL);
							OUTFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
						}
						else {
							if (agrep_finalfp != NULL) {
								fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);
							}
							else {
								if (agrep_outpointer + curtextend - curtextbegin >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								memcpy(agrep_outbuffer+agrep_outpointer, curtextbegin, curtextend-curtextbegin);
								agrep_outpointer += curtextend - curtextbegin;
							}
						}
						}
						else if (PRINTED) {
							if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
							else agrep_outbuffer[agrep_outpointer ++] = '\n';
							PRINTED = 0;
						}
					}
					else {	/* INVERSE */
						if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
							if (agrep_finalfp != NULL)
								newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_finalfp, -1, EASYSEARCH);
							else {
								if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
									if (newlen + agrep_outpointer >= agrep_outlen) {
										OUTPUT_OVERFLOW;
										return -1;
									}
									agrep_outpointer += newlen;
								}
							}
							lastout=textbegin;
							CurrentByteOffset += textbegin + 1 - text;
							text = textbegin + 1;
						}
						else { /* NOT TCOMPRESSED */
							if (agrep_finalfp != NULL)
								fwrite(lastout, 1, curtextbegin-lastout, agrep_finalfp);
							else {
								if (curtextbegin - lastout + agrep_outpointer >= agrep_outlen) {
									OUTPUT_OVERFLOW;
									return -1;
								}
								memcpy(agrep_outbuffer+agrep_outpointer, lastout, curtextbegin-lastout);
								agrep_outpointer += (curtextbegin - lastout);
							}
							lastout=textbegin;
							CurrentByteOffset += textbegin + 1 - text;
							text = textbegin + 1;
						} /* TCOMPRESSED */
					}
				}
				else {	/* COUNT */
					CurrentByteOffset += textbegin + 1 - text;
					text = textbegin + 1 ;
				}
				if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
				    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */
			}
			else { CurrentByteOffset += (oldtext + m - text); text = oldtext + m; }
		}
		oldtext = text; 
	}

	if (INVERSE && !COUNT && (lastout <= textend)) {
		if (TCOMPRESSED == ON) { /* INVERSE: Don't care about filtering time */
			if (agrep_finalfp != NULL)
				newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_finalfp, -1, EASYSEARCH);
			else {
				if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, textend - lastout + 1, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {
					if (newlen + agrep_outpointer >= agrep_outlen) {
						OUTPUT_OVERFLOW;
						return -1;
					}
					agrep_outpointer += newlen;
				}
			}
		}
		else { /* NOT TCOMPRESSED */
			if (agrep_finalfp != NULL)
				fwrite(lastout, 1, textend-lastout + 1, agrep_finalfp);
			else {
				if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				memcpy(agrep_outbuffer+agrep_outpointer, lastout, textend-lastout + 1);
				agrep_outpointer += (textend - lastout + 1);
			}
		} /* TCOMPRESSED */
	}

	return 0;
}

static void
prep4(Pattern, m)

char *Pattern; 
int m;
{
	int i, j, k;
	unsigned hash;

	for(i=0; i< MAXSYM; i++) char_map[i] = 0;
	char_map['a'] = char_map['A'] = 4;
	char_map['g'] = char_map['g'] = 1;
	char_map['t'] = char_map['t'] = 2;
	char_map['c'] = char_map['c'] = 3;
	char_map['n'] = char_map['n'] = 5;

	BSize = blog(4, m);
	for (i = 1, Hashmask = 1 ; i<(int)(BSize*LOG_DNA); i++) Hashmask = (Hashmask << 1) + 1 ;
	if (MEMBER_D != NULL) free(MEMBER_D);
	MEMBER_D = (char *) malloc((Hashmask+1)  * sizeof(char));
#ifdef DEBUG
	printf("BSize = %d", BSize);
#endif 
	for (i=0; i <= Hashmask; i++) MEMBER_D[i] = 0;
	for (j=0; j < (int)BSize; j++) {
		for(i=m-1; i >= j; i--) {
			hash = 0;
			for(k=0; k <= j; k++) 
				hash = (hash << LOG_DNA) +char_map[Pattern[i-k]]; 
#ifdef DEBUG
			printf("< %d >, ", hash);
#endif
			MEMBER_D[hash] = 1;
		}
	}
}

int
blog(base, m )

int base, m;
{
	int i, exp;
	exp = base;
	m = m + m/2;
	for (i = 1; exp < m; i++) exp = exp * base;
	return(i);
}
