/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/*

[fix] TG 22.10.97 3.34	in mgrep(): check bounds before memcpy-ing
[chg] TG 21.08.96	now uses ISO_CHAR.H

*/

/* multipattern matcher */

#include <stdio.h>
#include <ctype.h>

#ifdef __EMX__
#include <sys/types.h>
#endif

#include <sys/stat.h>

#include "agrep.h"
#include "codepage.h"

#include <sys/time.h>

#ifndef _WIN32
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#include "config.h"

/* TG 29.04.04 */
#include <errno.h>

extern unsigned char LUT[256];

#define ddebug
#define uchar unsigned char
#undef	MAXPAT
#define MAXPAT  256
#undef	MAXLINE
#define MAXLINE 1024
#undef	MAXSYM
#define MAXSYM  256
#define MAXMEMBER1 32768
/* #define MAXMEMBER1 262144 */ /*2^18 */ 
#define MAXPATFILE 600000
#define BLOCKSIZE  16384
#define MAXHASH    32768 
/* #define MAXHASH    262144 */
#define mask5 	   32767
#define max_num    40000
#if	ISO_CHAR_SET
#define W_DELIM	   256
#else
#define W_DELIM	   128
#endif
#define L_DELIM    10 
#define Hbits 5 /* how much to shift to perform the hash */

extern int LIMITOUTPUT, LIMITPERFILE;
extern int BYTECOUNT, PRINTOFFSET, PRINTRECORD, CurrentByteOffset;
extern int MULTI_OUTPUT;	/* used by glimpse only if OR, never for AND */
extern int DELIMITER;
extern CHAR D_pattern[MaxDelimit*2];
extern int D_length;
extern CHAR tc_D_pattern[MaxDelimit*2];
extern int tc_D_length;
extern COUNT, FNAME, SILENT, FILENAMEONLY, prev_num_of_matched, num_of_matched;
extern INVERSE, OUTTAIL;
extern WORDBOUND, WHOLELINE, NOUPPER;
extern ParseTree *AParse;
extern int AComplexPattern;
extern unsigned char  CurrentFileName[], Progname[]; 
extern total_line;
extern agrep_initialfd;
extern int EXITONERROR;
extern int PRINTPATTERN;
extern int agrep_inlen;
extern CHAR *agrep_inbuffer;
extern FILE *agrep_finalfp;
extern int agrep_outpointer;
extern int agrep_outlen;
extern CHAR * agrep_outbuffer;
extern int errno;
extern int NEW_FILE, POST_FILTER;

extern int tuncompressible();
extern int quick_tcompress();
extern int quick_tuncompress();
extern int TCOMPRESSED;
extern int EASYSEARCH;
extern char FREQ_FILE[MAX_LINE_LEN], HASH_FILE[MAX_LINE_LEN], STRING_FILE[MAX_LINE_LEN];
extern char PAT_FILE_NAME[MAX_LINE_LEN];

uchar SHIFT1[MAXMEMBER1];

int   LONG  = 0;
int   SHORT = 0;
int   p_size= 0;

uchar tr[MAXSYM];
uchar tr1[MAXSYM];
int   HASH[MAXHASH];
int   Hash2[max_num];
uchar *PatPtr[max_num];
uchar *pat_spool = NULL; /* [MAXPATFILE+2*max_num+MAXPAT]; */
uchar *patt[max_num];
int   pat_len[max_num];
int   pat_indices[max_num]; /* pat_indices[p] gives the actual index in matched_teriminals: used only with AParse != 0 */
int num_pat;

extern char  amatched_terminals[MAXNUM_PAT]; /* which patterns have been matched in the current line? Used only with AParse != 0, so max_num is not needed */
extern int anum_terminals;
extern int AComplexBoolean;
static void countline(unsigned char *text, int len);

#ifdef _WIN32
int  eval_tree();         /* asplit.c */
int  fill_buf();          /* bitap.c */
int  monkey1();           /* newmgrep.c */
int  m_short();           /* newmgrep.c */
#endif

#if	DOTCOMPRESSED
/* Equivalent variables for compression search */
uchar tc_SHIFT1[MAXMEMBER1];

int   tc_LONG  = 0;
int   tc_SHORT = 0;
int   tc_p_size= 0;

uchar tc_tr[MAXSYM];
uchar tc_tr1[MAXSYM];
int   tc_HASH[MAXHASH];
int   tc_Hash2[max_num];
uchar *tc_PatPtr[max_num];
uchar *tc_pat_spool = NULL; /* [MAXPATFILE+2*max_num+MAXPAT]; */
uchar *tc_patt[max_num];
int   tc_pat_len[max_num];
int   tc_pat_indices[max_num]; /* pat_indices[p] gives the actual index in matched_teriminals: used only with AParse != 0 */
int tc_num_pat;	/* must be the same as num_pat */
#endif	/*DOTCOMPRESSED*/

static void f_prep();
static void f_prep1();
static void accumulate();
#if	DOTCOMPRESSED
static void tc_f_prep();
static void tc_f_prep1();
static void tc_accumulate();
#endif

#ifdef perf_check
	int cshift=0, cshift0=0, chash=0;
#endif

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
 * 8. For convenience and speed, the multipattern matching routines to handle
 *    compressed files have been separated from their (normal) counterparts.
 * 9. One special note on handling complicated boolean patterns: the parse
 *    tree will be the same for both compressed and uncomrpessed patterns and the
 *    same amatched_terminals array will be used in both. BUT, the pat_spool and
 *    pat_index, etc., will be different as they refer to the individual terminals.
 */

int
prepf(mfp, mbuf, mlen)
int mfp, mlen;
unsigned char *mbuf;
{
	int length=0, i, p=1;
	uchar *pat_ptr;
	unsigned Mask = 31;
	int num_read;
	unsigned char *buf;
	struct stat stbuf;
	int j, k;	/* to implement \\ */

        if ((mfp == -1) && ((mbuf == NULL) || (mlen <= 0))) return -1;

	if (mfp != -1) {
		if (fstat(mfp, &stbuf) == -1) {
			fprintf(stderr, "%s: cannot stat file: %s\n", Progname, PAT_FILE_NAME);
			return -1;
		}
		if (!S_ISREG(stbuf.st_mode)) {
			fprintf(stderr, "%s: pattern file not regular file: %s\n", Progname, PAT_FILE_NAME);
			return -1;
		}
		if (stbuf.st_size*2 > MAXPATFILE + 2*max_num) {
			fprintf(stderr, "%s: pattern file too large (> %d B): %s\n", Progname, (MAXPATFILE+2*max_num)/2, PAT_FILE_NAME);
			return -1;
		}
		if (pat_spool != NULL) free(pat_spool);
		pat_ptr = pat_spool = (unsigned char *)malloc(stbuf.st_size*2 + MAXPAT);
		alloc_buf(mfp, &buf, MAXPATFILE+2*BLOCKSIZE);
		while((num_read = fill_buf(mfp, buf+length, 2*BLOCKSIZE)) > 0) {
			length = length + num_read;
			if(length > MAXPATFILE) {
				fprintf(stderr, "%s: maximum pattern file size is %d\n", Progname, MAXPATFILE);
                                if (!EXITONERROR) {
                                        errno = AGREP_ERROR;
                                        free_buf(mfp, buf);
                                        return -1;
                                }
                                else exit(2);
			}
		}
	}
	else {
		buf = mbuf;
		length = mlen;
		if (mlen*2 > MAXPATFILE + 2*max_num) {
			fprintf(stderr, "%s: pattern buffer too large (> %d B)\n", Progname, (MAXPATFILE+2*max_num)/2);
			return -1;
		}
		if (pat_spool != NULL) free(pat_spool);
		pat_ptr = pat_spool = (unsigned char *)malloc(mlen*2 + MAXPAT);
	}

	/* Now all the patterns are in buf */
	buf[length] = '\n';
	i=0; p=1;

/* removed by Udi 11/8/94 - we now do WORDBOUND "by hand" 
 *
 *	if(WORDBOUND) {
 *		while(i<length) {
 *			patt[p] = pat_ptr;
 *			*pat_ptr++ = W_DELIM;
 *			while((i<length) && ((*pat_ptr = buf[i++]) != '\n')) pat_ptr++;
 *			*pat_ptr++ = W_DELIM;
 *			*pat_ptr++ = 0;
 *			p++;
 *		}
 *	}
 *	else
 */
	
	if(WHOLELINE) {
		while(i<length) {
			patt[p] = pat_ptr;
			*pat_ptr++ = L_DELIM;
			while((i<length) && ((*pat_ptr = buf[i++]) != '\n')) pat_ptr++;
			*pat_ptr++ = L_DELIM;
			*pat_ptr++ = 0;
			p++;
		}
	}
	else {
		while(i < length) {
			patt[p] = pat_ptr;
			while((i<length) && ((*pat_ptr = buf[i++]) != '\n')) pat_ptr++;
			*pat_ptr++ = 0;
			p++;  
		}
	}

	/* Now, the patterns have been copied into patt[] */
	if(p>max_num) {
		fprintf(stderr, "%s: maximum number of patterns is %d\n", Progname, max_num); 
                if (!EXITONERROR) {
                        errno = AGREP_ERROR;
                        free_buf(mfp, buf);
                        return -1;
                }
                else exit(2);

	}

	for(i=1; i<20; i++) *pat_ptr = i;  /* boundary safety zone */

	/* I might have to keep changing tr s.t. mgrep won't get confused with W_DELIM */
	
	for(i=0; i< MAXSYM; i++) tr[i] = i;
	
	if(NOUPPER) {
                for (i=0; i<MAXSYM; i++)

#if (defined(__EMX__) && defined(ISO_CHAR_SET))
			tr[i] = tr[LUT[i]];
#else
                        if (isupper(i)) tr[i] = tr[tolower(i)];
#endif
	}

/*	if(WORDBOUND) {
		for(i=1; i<MAXSYM; i++) if(!isalnum(i)) tr[i] = W_DELIM;
	}

	removed by Udi 11/8/94 - the trick of using W-delim was too buggy.
	we now do it "by hand" after we find a match
*/

	for(i=0; i< MAXSYM; i++) tr1[i] = tr[i]&Mask;
	num_pat =  p-1;
	p_size  =  MAXPAT;
	for(i=1; i<=num_pat; i++) {
		p = strlen(patt[i]);
		if ((patt[i][0] == '^') || (patt[i][0] == '$')) patt[i][0] = '\n';
		if ((p > 1) && ((patt[i][p-1] == '^') || (patt[i][p-1] == '$')) && (patt[i][p-2] != '\\')) patt[i][p-1] = '\n';

		/* Added by bg, Dec 2nd 1994 */
		for (k=0; k<p; k++) {
			if (patt[i][k] == '\\') {
				for (j=k; j<p; j++)
					patt[i][j] = patt[i][j+1]; /* including '\0' */
				p--;
			}
		}

		pat_len[i] = p;
		/*
		pat_len[i] = (WORDBOUND?(p-2>0?p-2:1):p);  changed by Udi 11/8/94
		*/
#ifdef	debug
		printf("prepf(): patt[%d]=%s, pat_len[%d]=%d\n", i, patt[i], i, pat_len[i]);
#endif
		if(p!=0 && p < p_size) p_size = p;	/* MIN */
	}
	if(p_size == 0) {
		fprintf(stderr, "%s: the pattern file is empty\n", Progname);
                if (!EXITONERROR) {
                        errno = AGREP_ERROR;
                        free_buf(mfp, buf);
                        return -1;
                }
                else exit(2);
	}
	if(length > 400 && p_size > 2) LONG = 1;
	if(p_size == 1) SHORT = 1;
	for(i=0; i<MAXMEMBER1; i++) SHIFT1[i] = p_size - 1 - LONG;
	for(i=0; i<MAXHASH; i++) {
		HASH[i] = 0;
	}
	for(i=1; i<=num_pat; i++) f_prep(i, patt[i]);
	accumulate();
	memset(pat_indices, '\0', sizeof(int) * (num_pat + 1));
	for(i=1; i<=num_pat; i++) f_prep1(i, patt[i]);

#if	DOTCOMPRESSED
	/* prepf for compression */
	if (-1 == tc_prepf(buf, length)) {
		free_buf(mfp, buf);
		return -1;
	}
#endif	/*DOTCOMPRESSED*/
        free_buf(mfp, buf);
        return 0;
}

#if	DOTCOMPRESSED
/*
 * Compression equivalent of prepf: called right after prepf.
 * 1. Read patt and SHIFT1
 * 2. Call tcompress on the patterns in patt and put in tc_patt.
 * 3. Use these patterns to compute tc_SHIFT (ignore WDELIM, LDELIM, case sensitivity, etc.)
 * 4. Process other variables/functions (pat_spool, tr, tr1, pat_len, accumulate, SHIFT1, f_prep, f_prep1, pat_indices) appropriately.
 */
int
tc_prepf(buf, length)
unsigned char *buf;
int	length;
{
	int i, p=1;
	uchar *pat_ptr;
	unsigned Mask = 31;
	int tc_length;
	unsigned char tc_buf[MAXPAT * 2];	/* maximum length of the compressed pattern */
	static struct timeval initt, finalt;

	if (length*2 > MAXPATFILE + 2*max_num) {
		fprintf(stderr, "%s: pattern buffer too large (> %d B)\n", Progname, (MAXPATFILE+2*max_num)/2);
		return -1;
	}
	if (tc_pat_spool != NULL) free(tc_pat_spool);
	pat_ptr = tc_pat_spool = (unsigned char *)malloc(length*2 + MAXPAT);

#if	MEASURE_TIMES
	gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/

	i=0; p=1;
	while(i < length) {
		tc_patt[p] = pat_ptr;
		while((*pat_ptr = buf[i++]) != '\n') pat_ptr++;
		*pat_ptr++ = 0;
		if ((tc_length = quick_tcompress(FREQ_FILE, HASH_FILE, tc_patt[p], strlen(tc_patt[p]), tc_buf, MAXPAT * 2 - 8, TC_EASYSEARCH)) > 0) {
			memcpy(tc_patt[p], tc_buf, tc_length);
			tc_patt[p][tc_length] = '\0';
			pat_ptr = tc_patt[p] + tc_length + 1;	/* character after '\0' */
		}
		p++;  
	}

	for(i=1; i<20; i++) *pat_ptr = i;  /* boundary safety zone */

	/* Ignore all other options: it is automatically W_DELIM */
	for(i=0; i< MAXSYM; i++) tc_tr[i] = i;
	for(i=0; i< MAXSYM; i++) tc_tr1[i] = tc_tr[i]&Mask;
	tc_num_pat =  p-1;
	tc_p_size  =  MAXPAT;
	for(i=1; i<=num_pat; i++) {
		p = strlen(tc_patt[i]);
		tc_pat_len[i] = p;
#ifdef	debug
		printf("prepf(): tc_patt[%d]=%s, tc_pat_len[%d]=%d\n", i, tc_patt[i], i, tc_pat_len[i]);
#endif
		if(p!=0 && p < tc_p_size) tc_p_size = p;	/* MIN */
	}
	if(tc_p_size == 0) {	/* cannot happen NOW */
		fprintf(stderr, "%s: the pattern file is empty\n", Progname);
                if (!EXITONERROR) {
                        errno = AGREP_ERROR;
                        return -1;
                }
                else exit(2);
	}
	if(length > 400 && tc_p_size > 2) tc_LONG = 1;
	if(tc_p_size == 1) tc_SHORT = 1;
	for(i=0; i<MAXMEMBER1; i++) tc_SHIFT1[i] = tc_p_size - 1 - LONG;
	for(i=0; i<MAXHASH; i++) {
		tc_HASH[i] = 0;
	}
	for(i=1; i<=tc_num_pat; i++) tc_f_prep(i, tc_patt[i]);
	tc_accumulate();
	memset(tc_pat_indices, '\0', sizeof(int) * (tc_num_pat + 1));
	for(i=1; i<=tc_num_pat; i++) tc_f_prep1(i, tc_patt[i]);

#if	MEASURE_TIMES
	gettimeofday(&finalt, NULL);
	INFILTER_ms +=  (finalt.tv_sec*1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif	/*MEASURE_TIMES*/
        return 0;
}
#endif	/*DOTCOMPRESSED*/

int
mgrep(fd)
int fd;
{ 
	register char r_newline = '\n';
	unsigned char *text;
	register int buf_end, num_read, start, end, residue = 0;
	int	oldCurrentByteOffset;
	int	first_time = 1;

#if     AGREP_POINTER
        if (fd != -1) {
#endif  /*AGREP_POINTER*/
                alloc_buf(fd, &text, 2*BLOCKSIZE+MAXLINE);
		text[MAXLINE-1] = '\n';  /* initial case */
		start = MAXLINE;

		while( (num_read = fill_buf(fd, text+MAXLINE, 2*BLOCKSIZE)) > 0) 
		{
			buf_end = end = MAXLINE + num_read -1 ;
			oldCurrentByteOffset = CurrentByteOffset;

			if (first_time) {
				if ((TCOMPRESSED == ON) && tuncompressible(text+MAXLINE, num_read)) {
					EASYSEARCH = text[MAXLINE+SIGNATURE_LEN-1];
					start += SIGNATURE_LEN;
					CurrentByteOffset += SIGNATURE_LEN;
					if (!EASYSEARCH) {
						fprintf(stderr, "not compressed for easy-search: can miss some matches in: %s\n", CurrentFileName);
					}
				}
				else TCOMPRESSED = OFF;
				first_time = 0;
			}

			if (!DELIMITER) {
				while(text[end]  != r_newline && end > MAXLINE) end--;
				text[start-1] = r_newline;
			}
			else {
				unsigned char *newbuf = text + end + 1;
				newbuf = backward_delimiter(newbuf, text+MAXLINE, D_pattern, D_length, OUTTAIL);	/* see agrep.c/'d' */
				if (newbuf < text+MAXLINE+D_length) newbuf = text + end + 1;
				end = newbuf - text - 1;

/* TG 22.10.97 Check bounds before memcpy-ing */
/* printf("text %x start %i D_length %i D_pattern %i residue %i\n",text,start,D_length,D_pattern,residue); */

			if (start > D_length) memcpy(text+start-D_length, D_pattern, D_length);
			memcpy(text+start+residue, D_pattern, D_length);

/* original code was:	memcpy(text+start-D_length, D_pattern, D_length);	*/

			}
			residue = buf_end - end  + 1 ;
			if(INVERSE && COUNT) countline(text+MAXLINE, num_read);

			/* MGREP_PROCESS */
			if (TCOMPRESSED) {	/* separate functions since separate globals => too many if-statements within a single function makes it slow */
#if	DOTCOMPRESSED
				if(tc_SHORT) { if (-1 == tc_m_short(text, start, end)) {free_buf(fd, text); return -1;}}
				else      { if (-1 == tc_monkey1(text, start, end)) {free_buf(fd, text); return -1;}}
#endif	/*DOTCOMPRESSED*/
			}
			else {
				if(SHORT) { if (-1 == m_short(text, start, end)) {free_buf(fd, text); return -1;}}
				else      { if (-1 == monkey1(text, start, end)) {free_buf(fd, text); return -1;}}
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
                                        else {
                                                agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
                                        }
                                        agrep_outpointer += outindex;
                                }
                                free_buf(fd, text);
                                NEW_FILE = OFF;
                                return 0;
                        }

			CurrentByteOffset = oldCurrentByteOffset + end - start + 1;
			start = MAXLINE - residue;
			if(start < 0) {
				start = 1; 
			}
			strncpy(text+start, text+end, residue);

			if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
			    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
				free_buf(fd, text);
				return 0;	/* done */
			}
		} /* end of while(num_read = ... */
		if (!DELIMITER) {
			text[start-1] = '\n';
			text[start+residue] = '\n';
		}
		else {
			if (start > D_length) memcpy(text+start-D_length, D_pattern, D_length);
			memcpy(text+start+residue, D_pattern, D_length);
		}
		end = start + residue;
		if(residue > 1) {
			if (TCOMPRESSED) {
#if	DOTCOMPRESSED
				if(tc_SHORT) tc_m_short(text, start, end);
				else      tc_monkey1(text, start, end);
#endif	/*DOTCOMPRESSED*/
			}
			else {
				if(SHORT) m_short(text, start, end);
				else      monkey1(text, start, end);
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
                                        else {
                                                agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
                                        }
                                        agrep_outpointer += outindex;
                                }
                                free_buf(fd, text);
                                NEW_FILE = OFF;
                                return 0;
                        }
		}
		free_buf(fd, text);
		return (0);
#if	AGREP_POINTER
	}
	else {
                text = (unsigned char *)agrep_inbuffer;
                num_read = agrep_inlen;
                start = 0;
                buf_end = end = num_read - 1;

			oldCurrentByteOffset = CurrentByteOffset;

			if (first_time) {
				if ((TCOMPRESSED == ON) && tuncompressible(text+MAXLINE, num_read)) {
					EASYSEARCH = text[MAXLINE+SIGNATURE_LEN-1];
					start += SIGNATURE_LEN;
					CurrentByteOffset += SIGNATURE_LEN;
					if (!EASYSEARCH) {
						fprintf(stderr, "not compressed for easy-search: can miss some matches in: %s\n", CurrentFileName);
					}
				}
				else TCOMPRESSED = OFF;
				first_time = 0;
			}

			if (!DELIMITER)
				while(text[end]  != r_newline && end > 1) end--;
			else {
                                unsigned char *newbuf = text + end + 1;
                                newbuf = backward_delimiter(newbuf, text, D_pattern, D_length, OUTTAIL);        /* see agrep.c/'d' */
				if (newbuf < text+D_length) newbuf = text + end + 1;
                                end = newbuf - text - 1;
			}
			/* text[0] = text[end] = r_newline; : the user must ensure that the delimiter is there at text[0] and occurs somewhere before text[end] */

			if (INVERSE && COUNT) countline(text, num_read);

                        /* An exact copy of the above MGREP_PROCESS */
			if (TCOMPRESSED) {	/* separate functions since separate globals => too many if-statements within a single function makes it slow */
#if	DOTCOMPRESSED
				if(tc_SHORT) { if (-1 == tc_m_short(text, start, end)) {free_buf(fd, text); return -1;}}
				else      { if (-1 == tc_monkey1(text, start, end)) {free_buf(fd, text); return -1;}}
#endif	/*DOTCOMPRESSED*/
			}
			else {
				if(SHORT) { if (-1 == m_short(text, start, end)) {free_buf(fd, text); return -1;}}
				else      { if (-1 == monkey1(text, start, end)) {free_buf(fd, text); return -1;}}
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
                                        else {
                                                agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
                                        }
                                        agrep_outpointer += outindex;
                                }
                                free_buf(fd, text);
                                NEW_FILE = OFF;
                                return 0;
                        }

                return 0;
	}
#endif	/*AGREP_POINTER*/
#ifdef perf_check
	fprintf(stderr,"Shifted %d times; shift=0 %d times; hash was = %d times\n",cshift, cshift0, chash);
	return 0;
#endif
} /* end mgrep */

static void
countline(text, len)
unsigned char *text; int len;
{
int i;
	for (i=0; i<len; i++) if(text[i] == '\n') total_line++;
}

/* Stuff that always needs to be printed whenever there is a match in all functions in this file */
int
print_options(pat_index, text, curtextbegin, curtextend)
	int	pat_index;
	unsigned char	*text, *curtextbegin, *curtextend;
{
	int	PRINTED = 0;
	if(FNAME && (NEW_FILE || !POST_FILTER)) {
		char    nextchar = (POST_FILTER == ON)?'\n':' ';
		char    *prevstring = (POST_FILTER == ON)?"\n":"";
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

	if (PRINTPATTERN) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%d- ", pat_index);
		else {
			char s[32];
			int outindex;
			sprintf(s, "%d- ", pat_index);
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

	if (BYTECOUNT) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%d= ", CurrentByteOffset);
		else {
			char s[32];
			int outindex;
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
	return PRINTED;
}

int
monkey1( text, start, end  ) 
int start, end; register unsigned char *text;
{
	int PRINTED = 0;
        unsigned char *oldtext;
	int pat_index;
	register uchar *textend;
	unsigned char *textbegin;
	unsigned char *curtextend;
	unsigned char *curtextbegin;
	register unsigned hash;
	register uchar shift;
	register int  m1, Long=LONG;
	int MATCHED=0;
	register uchar *qx;
	register uchar *px;
	register int p, p_end;
	uchar *lastout;
	/* int OUT=0; */
	int hash2;
	int j;
	int DOWITHMASK;

	DOWITHMASK = 0;
	if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	textbegin = text + start;
	textend = text + end;
	m1 = p_size-1;
	lastout = text+start;
	text = text + start + m1 -1 ;
	/* -1 to allow match to the first \n in case the pattern has ^ in front of it */
/*
	if (WORDBOUND || WHOLELINE) text = text-1;
	if (WHOLELINE) text = text-1;
*/
		/* to accomodate the extra 2 W_delim */
	while (text <= textend) {
		hash=tr1[*text];
		hash=(hash<<Hbits)+(tr1[*(text-1)]);
		if(Long) hash=(hash<<Hbits)+(tr1[*(text-2)]);
		shift = SHIFT1[hash];
#ifdef perf_check
		cshift++;
#endif
		if(shift == 0) {
			hash=hash&mask5;
			hash2 = (tr[*(text-m1)]<<8) + tr[*(text-m1+1)];
			p = HASH[hash];
#ifdef perf_check
			cshift0++;
#endif
			p_end = HASH[hash+1];
#ifdef debug
			printf("hash=%d, p=%d, p_end=%d\n", hash, p, p_end);
#endif
			while(p++ < p_end) {
				if(hash2 != Hash2[p]) continue;
#ifdef perf_check
				chash++;
#endif
				if (((pat_index = pat_indices[p]) <= 0) || (pat_len[pat_index] <= 0)) continue;
				px = PatPtr[p];
				qx = text-m1;
				while((*px!=0)&&(tr[*px] == tr[*qx])) {
					px++;
					qx++;
				}
				if (*px == 0) {
					if(text > textend) return 0;
					if (WORDBOUND) {
						if (isalnum(*qx)) goto skip_output;
						if (isalnum(*(text-m1-1))) goto skip_output;
					}
					if (!DOWITHMASK) {
                                                /* Don't update CurrentByteOffset here: only before outputting properly */
                                                if (!DELIMITER) {
							curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
							if (*curtextbegin == '\n') curtextbegin ++;
							curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
							if (*curtextend == '\n') curtextend ++;
                                                }
                                                else {
                                                        curtextbegin = backward_delimiter(text, textbegin, D_pattern, D_length, OUTTAIL);
                                                        curtextend = forward_delimiter(text+1, textend, D_pattern, D_length, OUTTAIL);
                                                }
						if (!OUTTAIL || INVERSE) textbegin = curtextend;
						else if (DELIMITER) textbegin = curtextend - D_length;
						else textbegin = curtextend - 1;
					}

					DOWITHMASK = 1;
					amatched_terminals[pat_index - 1] = 1;
					if (AComplexBoolean) {
						/* Can output only after all the matches in the current record have been identified: just like filter_output */
						oldtext = text;
						CurrentByteOffset += (oldtext + pat_len[pat_index] - 1 - text);
						text = oldtext + pat_len[pat_index] - 1;
						MATCHED = 0;
						goto skip_output;
					}
					else if ((long)AParse & AND_EXP) {
						for (j=0; j<anum_terminals; j++) if (!amatched_terminals[j]) break;
						if (j<anum_terminals) goto skip_output;
					}
					MATCHED=1;
                                        oldtext = text; /* only for MULTI_OUTPUT */

#undef	DO_OUTPUT
#define DO_OUTPUT(change_text)\
					num_of_matched++;\
					if(FILENAMEONLY || SILENT)  return 0;\
					if (!COUNT) {\
						PRINTED = print_options(pat_index, text, curtextbegin, curtextend);\
						if(!INVERSE) {\
							if (PRINTRECORD) {\
							if (agrep_finalfp != NULL) {\
								fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);\
							}\
							else {\
								if (agrep_outpointer + curtextend - curtextbegin>= agrep_outlen) {\
									OUTPUT_OVERFLOW;\
									return -1;\
								}\
								else {\
									memcpy(agrep_outbuffer + agrep_outpointer, curtextbegin, curtextend-curtextbegin);\
									agrep_outpointer += curtextend - curtextbegin;\
								}\
							}\
							}\
							else if (PRINTED) {\
								if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);\
								else agrep_outbuffer[agrep_outpointer ++] = '\n';\
								PRINTED = 0;\
							}\
                                                        if ((change_text) && MULTI_OUTPUT) {     /* next match starting from end of current */\
								CurrentByteOffset += (oldtext + pat_len[pat_index] - 1 - text);\
                                                                text = oldtext + pat_len[pat_index] - 1;\
                                                                MATCHED = 0;\
                                                        }\
							else if (change_text) {\
								CurrentByteOffset += textbegin - text;\
								text = textbegin;\
							}\
						}\
						else {	/* INVERSE */\
							/* if(lastout < curtextbegin) OUT=1; */\
							if (agrep_finalfp != NULL)\
								fwrite(lastout, 1, curtextbegin-lastout, agrep_finalfp);\
							else {\
								if (curtextbegin - lastout + agrep_outpointer >= agrep_outlen) {\
									OUTPUT_OVERFLOW;\
									return -1;\
								}\
								memcpy(agrep_outbuffer+agrep_outpointer, lastout, curtextbegin-lastout);\
								agrep_outpointer += (curtextbegin-lastout);\
							}\
							lastout=textbegin;\
							if (change_text) {\
								CurrentByteOffset += textbegin - text;\
								text = textbegin;\
							}\
						}\
					}\
					else if (change_text) {	/* COUNT */\
						CurrentByteOffset += textbegin - text;\
						text = textbegin;\
					}\
					if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||\
					    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */\

					DO_OUTPUT(1)
				}

			skip_output:
                                if (MATCHED && !MULTI_OUTPUT && !AComplexBoolean) break;    /* else look for more possible matches since we never know how many will match */
				if (DOWITHMASK && (text >= curtextend - 1)) {
					DOWITHMASK = 0;
					if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
						DO_OUTPUT(0)
					}
					if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
				}
			}
			/* If I found some match and I am about to cross over a delimiter, then set DOWITHMASK to 0 and zero out the amatched_terminals */
			if (DOWITHMASK && (text >= curtextend - 1)) {
				DOWITHMASK = 0;
				if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
					DO_OUTPUT(0)
				}
				if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
			}
			if(!MATCHED) shift = 1;	/* || MULTI_OUTPUT is implicit */
			else {
				MATCHED = 0;
				shift = m1 - 1 > 0 ? m1 - 1 : 1;
			}
		}

		/* If I found some match and I am about to cross over a delimiter, then set DOWITHMASK to 0 and zero out the amatched_terminals */
		if (DOWITHMASK && (text >= curtextend - 1)) {
			DOWITHMASK = 0;
			if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
				DO_OUTPUT(0)
			}
			if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
		}

		text += shift;
		CurrentByteOffset += shift;
	}

	/* Do residual stuff: check if there was a match at the end of the line | check if rest of the buffer needs to be output due to inverse */

	if (DOWITHMASK && (text >= curtextend - 1)) {
		DOWITHMASK = 0;
		if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
			DO_OUTPUT(0)
		}
		if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	}

	if(INVERSE && !COUNT && (lastout <= textend)) {
                if (agrep_finalfp != NULL) {
                        while(lastout <= textend) fputc(*lastout++, agrep_finalfp);
                }
                else {
                        if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
                                OUTPUT_OVERFLOW;
                                return -1;
                        }
                        memcpy(agrep_outbuffer+agrep_outpointer, lastout, textend-lastout+1);
                        agrep_outpointer += (textend-lastout+1);
                        lastout = textend;
                }
	}

	return 0;
}

#if	DOTCOMPRESSED
int
tc_monkey1( text, start, end  ) 
int start, end;
register unsigned char *text;
{
	int PRINTED = 0;
        unsigned char *oldtext;
	int pat_index;
	register uchar *textend;
	unsigned char *textbegin;
	unsigned char *curtextend;
        unsigned char *curtextbegin;
	register unsigned hash;
	register uchar shift;
	register int  m1, Long=LONG;
	int MATCHED=0;
	register uchar *qx;
	register uchar *px;
	register int p, p_end;
	uchar *lastout;
	/* int OUT=0; */
	int hash2;
	int j;
	int DOWITHMASK;
	struct timeval initt, finalt;
	int newlen;

	DOWITHMASK = 0;
	if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	textbegin = text + start;
	textend = text + end;
	m1 = tc_p_size-1;
	lastout = text+start;
	text = text + start + m1 -1;
	/* -1 to allow match to the first \n in case the pattern has ^ in front of it */
	/* WORDBOUND adjustment not required */
	while (text <= textend) {
		hash=tc_tr1[*text];
		hash=(hash<<Hbits)+(tc_tr1[*(text-1)]);
		if(Long) hash=(hash<<Hbits)+(tc_tr1[*(text-2)]);
		shift = tc_SHIFT1[hash];
#ifdef perf_check
		cshift++;
#endif
		if(shift == 0) {
			hash=hash&mask5;
			hash2 = (tc_tr[*(text-m1)]<<8) + tc_tr[*(text-m1+1)];
			p = tc_HASH[hash];
#ifdef perf_check
			cshift0++;
#endif
			p_end = tc_HASH[hash+1];
#ifdef debug
			printf("hash=%d, p=%d, p_end=%d\n", hash, p, p_end);
#endif
			while(p++ < p_end) {
				if(hash2 != tc_Hash2[p]) continue;
#ifdef perf_check
				chash++;
#endif
				if (((pat_index = tc_pat_indices[p]) <= 0) || (tc_pat_len[pat_index] <= 0)) continue;
				px = tc_PatPtr[p];
				qx = text-m1;

 				while((*px!=0)&&(tc_tr[*px] == tc_tr[*qx])) {
 					px++;
 					qx++;
 				}
 				if (*px == 0) {
					if(text > textend) return 0;
					if (!DOWITHMASK) {
						/* Don't update CurrentByteOffset here: only before outputting properly */
						if (!DELIMITER) {
							curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
							if (*curtextbegin == '\n') curtextbegin ++;
							curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
							if (*curtextend == '\n') curtextend ++;
						}
						else {
							curtextbegin = backward_delimiter(text, textbegin, tc_D_pattern, tc_D_length, OUTTAIL);
							curtextend = forward_delimiter(text+1, textend, tc_D_pattern, tc_D_length, OUTTAIL);
						}
					}
					/* else prev curtextbegin is OK: if full AND isn't found, DOWITHMASK is 0-ed so that we search at most 1 line below */
#if	MEASURE_TIMES
					gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
					/* Was it really a match in the compressed line from prev line in text to text + strlen(tc_pat_len[pat_index]? */
					if (-1==exists_tcompressed_word(tc_PatPtr[p], tc_pat_len[pat_index], curtextbegin, text - curtextbegin + tc_pat_len[pat_index], EASYSEARCH))
						goto skip_output;
#if     MEASURE_TIMES
					gettimeofday(&finalt, NULL);
					FILTERALGO_ms +=  (finalt.tv_sec *1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif  /*MEASURE_TIMES*/
					if (!DOWITHMASK) {
						if (!OUTTAIL || INVERSE) textbegin = curtextend;
						else if (DELIMITER) textbegin = curtextend - D_length;
						else textbegin = curtextend - 1;
					}
					DOWITHMASK = 1;
					amatched_terminals[pat_index-1] = 1;
					if (AComplexBoolean) {
						/* Can output only after all the matches in the current record have been identified: just like filter_output */
						oldtext = text;
						CurrentByteOffset += (oldtext + pat_len[pat_index] - 1 - text);
						text = oldtext + pat_len[pat_index] - 1;
						MATCHED = 0;
						goto skip_output;
					}
					else if ((long)AParse & AND_EXP) {
						for (j=0; j<anum_terminals; j++) if (!amatched_terminals[j]) break;
						if (j<anum_terminals) goto skip_output;
					}

					MATCHED=1;
                                        oldtext = text; /* only for MULTI_OUTPUT */

#undef	DO_OUTPUT
#define DO_OUTPUT(change_text)\
					num_of_matched++;\
					if(FILENAMEONLY || SILENT)  return 0;\
					if (!COUNT) {\
						PRINTED = print_options(pat_index, text, curtextbegin, curtextend);\
						if(!INVERSE) {\
							if (PRINTRECORD) {\
/* #if     MEASURE_TIMES\
							gettimeofday(&initt, NULL);\
#endif  MEASURE_TIMES */\
							if (agrep_finalfp != NULL)\
								newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);\
							else {\
								if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {\
									if (newlen + agrep_outpointer >= agrep_outlen) {\
										OUTPUT_OVERFLOW;\
										return -1;\
									}\
									agrep_outpointer += newlen;\
								}\
							}\
/* #if     MEASURE_TIMES\
							gettimeofday(&finalt, NULL);\
							OUTFILTER_ms += (finalt.tv_sec* 1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);\
#endif  MEASURE_TIMES */\
							}\
							else if (PRINTED) {\
								if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);\
								else agrep_outbuffer[agrep_outpointer ++] = '\n';\
								PRINTED = 0;\
							}\
                                                        if ((change_text) && MULTI_OUTPUT) {     /* next match starting from end of current */\
								CurrentByteOffset += (oldtext + tc_pat_len[pat_index] - 1 - text);\
                                                                text = oldtext + tc_pat_len[pat_index] - 1;\
                                                                MATCHED = 0;\
                                                        }\
							else if (change_text) {\
								CurrentByteOffset += textbegin - text;\
								text = textbegin;\
							}\
						}\
						else {	/* INVERSE: Don't care about filtering time */\
							/* if(lastout < curtextbegin) OUT=1; */\
							if (agrep_finalfp != NULL)\
								newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_finalfp, -1, EASYSEARCH);\
							else {\
								if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {\
									if (newlen + agrep_outpointer >= agrep_outlen) {\
										OUTPUT_OVERFLOW;\
										return -1;\
									}\
									agrep_outpointer += newlen;\
								}\
							}\
							lastout=textbegin;\
							if (change_text) {\
								CurrentByteOffset += textbegin - text;\
								text = textbegin;\
							}\
						}\
					}\
					else if (change_text) {\
						CurrentByteOffset += textbegin - text;\
						text = textbegin;\
					}\
					if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||\
					    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */\

					DO_OUTPUT(1)
				}

			skip_output:
                                if (MATCHED && !MULTI_OUTPUT && !AComplexBoolean) break;    /* else look for more possible matches since we never know how many will match */
				if (DOWITHMASK && (text >= curtextend - 1)) {
					DOWITHMASK = 0;
					if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
						DO_OUTPUT(0)
					}
					if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
				}
			}
			/* If I found some match and I am about to cross over a delimiter, then set DOWITHMASK to 0 and zero out the amatched_terminals */
			if (DOWITHMASK && (text >= curtextend - 1)) {
				DOWITHMASK = 0;
				if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
					DO_OUTPUT(0)
				}
				if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
			}
			if(!MATCHED) shift = 1;	/* || MULTI_OUTPUT is implicit */
			else {
				MATCHED = 0;
				shift = m1 - 1 > 0 ? m1 - 1 : 1;
			}
		}

		/* If I found some match and I am about to cross over a delimiter, then set DOWITHMASK to 0 and zero out the amatched_terminals */
		if (DOWITHMASK && (text >= curtextend - 1)) {
			DOWITHMASK = 0;
			if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
				DO_OUTPUT(0)
			}
			if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
		}

		text += shift;
		CurrentByteOffset += shift;
	}

	/* Do residual stuff: check if there was a match at the end of the line | check if rest of the buffer needs to be output due to inverse */

	if (DOWITHMASK && (text >= curtextend - 1)) {
		DOWITHMASK = 0;
		if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
			DO_OUTPUT(0)
		}
		if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	}

	if (INVERSE && !COUNT && (lastout <= textend)) {
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

	return 0;
}
#endif	/*DOTCOMPRESSED*/

/* shift is always 1: slight change in MATCHED semantics: it is set to 1 even if COUNT is set: previously, it wasn't set. Will it effect m_short? */
int
m_short(text, start, end)
int start, end; register uchar *text;
{
	int PRINTED = 0;
	int pat_index;
        unsigned char *oldtext;
	register uchar *textend;
	unsigned char *textbegin;
	unsigned char *curtextend;
	unsigned char *curtextbegin;
	register int p, p_end;
	int MATCHED=0;
	/* int OUT=0; */
	uchar *lastout;
	uchar *qx;
	uchar *px;
	int j;
	int DOWITHMASK;

	DOWITHMASK = 0;
	if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	textend = text + end;
	lastout = text + start;
	textbegin = text + start;
	text = text + start - 1 ;
/*
	if (WORDBOUND || WHOLELINE) text = text-1;
*/
	if (WHOLELINE) text = text-1;
		/* to accomodate the extra 2 W_delim */
	while (++text <= textend) {
		CurrentByteOffset ++;
		p = HASH[tr[*text]];
		p_end = HASH[tr[*text]+1];
		while(p++ < p_end) {
			if (((pat_index = pat_indices[p]) <= 0) || (pat_len[pat_index] <= 0)) continue;
#ifdef	debug
			printf("m_short(): p=%d pat_index=%d off=%d\n", p, pat_index, textend - text);
#endif
			px = PatPtr[p];
			qx = text;
			while((*px!=0)&&(tr[*px] == tr[*qx])) {
				px++;
				qx++;
			}
			if (*px == 0) {
				if(text >= textend) return 0;
				if (WORDBOUND) {
					if (isalnum(*qx)) goto skip_output;
					if (isalnum(*(text-1))) goto skip_output;
				}
                                if (!DOWITHMASK) {
                                        /* Don't update CurrentByteOffset here: only before outputting properly */
                                        if (!DELIMITER) {
						curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
						if (*curtextbegin == '\n') curtextbegin ++;
						curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
						if (*curtextend == '\n') curtextend ++;
                                        }
                                        else {
                                                curtextbegin = backward_delimiter(text, textbegin, D_pattern, D_length, OUTTAIL);
                                                curtextend = forward_delimiter(text+1, textend, D_pattern, D_length, OUTTAIL);
                                        }
					if (!OUTTAIL || INVERSE) textbegin = curtextend;
					else if (DELIMITER) textbegin = curtextend - D_length;
					else textbegin = curtextend - 1;
                                }
                                /* else prev curtextbegin is OK: if full AND isn't found, DOWITHMASK is 0-ed so that we search at most 1 line below */
				DOWITHMASK = 1;

				amatched_terminals[pat_index - 1] = 1;
				if (AComplexBoolean) {
					/* Can output only after all the matches in the current record have been identified: just like filter_output */
					oldtext = text;
					CurrentByteOffset += (oldtext + pat_len[pat_index] - 1 - text);
					text = oldtext + pat_len[pat_index] - 1;
					MATCHED = 0;
					goto skip_output;
				}
				else if ((long)AParse & AND_EXP) {
					for (j=0; j<anum_terminals; j++) if (!amatched_terminals[j]) break;
					if (j<anum_terminals) goto skip_output;
				}

				MATCHED = 1;
				oldtext = text; /* used only if MULTI_OUTPUT */

#undef	DO_OUTPUT
#define DO_OUTPUT(change_text)\
				num_of_matched++;\
				if(FILENAMEONLY || SILENT)  return 0;\
				if (!COUNT) {\
					PRINTED = print_options(pat_index, text, curtextbegin, curtextend);\
					if(!INVERSE) {\
						if (PRINTRECORD) {\
						if (agrep_finalfp != NULL) {\
							fwrite(curtextbegin, 1, curtextend - curtextbegin, agrep_finalfp);\
						}\
						else {\
							if (agrep_outpointer + curtextend - curtextbegin >= agrep_outlen) {\
								OUTPUT_OVERFLOW;\
								return -1;\
							}\
							else {\
								memcpy(agrep_outbuffer + agrep_outpointer, curtextbegin, curtextend-curtextbegin);\
								agrep_outpointer += curtextend - curtextbegin;\
							}\
						}\
						}\
						else if (PRINTED) {\
							if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);\
							else agrep_outbuffer[agrep_outpointer ++] = '\n';\
							PRINTED = 0;\
						}\
                                                if ((change_text) && MULTI_OUTPUT) {     /* next match starting from end of current */\
							CurrentByteOffset += (oldtext + pat_len[pat_index] - 1 - text);\
                                                        text = oldtext + pat_len[pat_index] - 1;\
                                                        MATCHED = 0;\
                                                }\
						else if (change_text) {\
							CurrentByteOffset += textbegin - text;\
							text = textbegin;\
						}\
					}\
					else {\
                                                /* if(lastout < curtextbegin) OUT=1; */\
						if (agrep_finalfp != NULL)\
							fwrite(lastout, 1, curtextbegin-lastout, agrep_finalfp);\
						else {\
							if (curtextbegin - lastout + agrep_outpointer >= agrep_outlen) {\
								OUTPUT_OVERFLOW;\
								return -1;\
							}\
							memcpy(agrep_outbuffer+agrep_outpointer, lastout, curtextbegin-lastout);\
							agrep_outpointer += (curtextbegin-lastout);\
						}\
                                                lastout=textbegin;\
						if (change_text) {\
							CurrentByteOffset += textbegin - text;\
							text = textbegin;\
						}\
					}\
				}\
				else if (change_text) {\
					CurrentByteOffset += textbegin - text;\
					text = textbegin;\
				}\
				if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||\
				    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */\

				DO_OUTPUT(1)
			}

		skip_output:
                        if(MATCHED && !MULTI_OUTPUT && !AComplexBoolean) break;     /* else look for more possible matches */
			if (DOWITHMASK && (text >= curtextend - 1)) {
				DOWITHMASK = 0;
				if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
					DO_OUTPUT(0)
				}
				if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
			}
		}
		/* If I found some match and I am about to cross over a delimiter, then set DOWITHMASK to 0 and zero out the amatched_terminals */
		if (DOWITHMASK && (text >= curtextend - 1)) {
			DOWITHMASK = 0;
			if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
				DO_OUTPUT(0)
			}
			if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
		}
		if (MATCHED) text --;
		MATCHED = 0;
	} /* while */
	CurrentByteOffset ++;

	/* Do residual stuff: check if there was a match at the end of the line | check if rest of the buffer needs to be output due to inverse */

	if (DOWITHMASK && (text >= curtextend - 1)) {
		DOWITHMASK = 0;
		if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
			DO_OUTPUT(0)
		}
		if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	}

        if(INVERSE && !COUNT && (lastout <= textend)) {
                if (agrep_finalfp != NULL) {
                        while(lastout <= textend) fputc(*lastout++, agrep_finalfp);
                }
                else {
                        if (textend - lastout + 1 + agrep_outpointer >= agrep_outlen) {
                                OUTPUT_OVERFLOW;
                                return -1;
                        }
                        memcpy(agrep_outbuffer+agrep_outpointer, lastout, text-lastout+1);
                        agrep_outpointer += (text-lastout+1);
                        lastout = textend;
                }
        }

        return 0;
}

#if	DOTCOMPRESSED
/* shift is always 1: slight change in MATCHED semantics: it is set to 1 even if COUNT is set: previously, it wasn't set. Will it effect m_short? */
int
tc_m_short(text, start, end)
int start, end; register uchar *text;
{
	int PRINTED = 0;
	int pat_index;
        unsigned char *oldtext;
	register uchar *textend;
	unsigned char *textbegin;
	unsigned char *curtextend;
	unsigned char *curtextbegin;
	register int p, p_end;
	int MATCHED=0;
	/* int OUT=0; */
	uchar *lastout;
	uchar *qx;
	uchar *px;
	int j;
	int DOWITHMASK;
	struct timeval initt, finalt;
	int newlen;

	DOWITHMASK = 0;
	if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	textend = text + end;
	lastout = text + start;
	text = text + start - 1 ;
	textbegin = text + start;
	/* WORDBOUND adjustment not required */
	while (++text <= textend) {
		CurrentByteOffset ++;
		p = tc_HASH[tc_tr[*text]];
		p_end = tc_HASH[tc_tr[*text]+1];
		while(p++ < p_end) {
			if (((pat_index = tc_pat_indices[p]) <= 0) || (tc_pat_len[pat_index] <= 0)) continue;
#ifdef	debug
			printf("m_short(): p=%d pat_index=%d off=%d\n", p, pat_index, textend - text);
#endif
			px = tc_PatPtr[p];
			qx = text;
			while((*px!=0)&&(tc_tr[*px] == tc_tr[*qx])) {
				px++;
				qx++;
			}
			if (*px == 0) {
				if(text >= textend) return 0;

				if (!DOWITHMASK) {
					/* Don't update CurrentByteOffset here: only before outputting properly */
					if (!DELIMITER) {
						curtextbegin = text; while((curtextbegin > textbegin) && (*(--curtextbegin) != '\n'));
						if (*curtextbegin == '\n') curtextbegin ++;
						curtextend = text+1; while((curtextend < textend) && (*curtextend != '\n')) curtextend ++;
						if (*curtextend == '\n') curtextend ++;
					}
					else {
						curtextbegin = backward_delimiter(text, textbegin, tc_D_pattern, tc_D_length, OUTTAIL);
						curtextend = forward_delimiter(text+1, textend, tc_D_pattern, tc_D_length, OUTTAIL);
					}
				}
				/* else prev curtextbegin is OK: if full AND isn't found, DOWITHMASK is 0-ed so that we search at most 1 line below */
#if	MEASURE_TIMES
				gettimeofday(&initt, NULL);
#endif	/*MEASURE_TIMES*/
				/* Was it really a match in the compressed line from prev line in text to text + strlen(tc_pat_len[pat_index]? */
				if (-1 == exists_tcompressed_word(tc_PatPtr[p], tc_pat_len[pat_index], curtextbegin, text - curtextbegin + tc_pat_len[pat_index], EASYSEARCH))
					goto skip_output;
#if     MEASURE_TIMES
				gettimeofday(&finalt, NULL);
				FILTERALGO_ms +=  (finalt.tv_sec *1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);
#endif  /*MEASURE_TIMES*/

				if (!DOWITHMASK) {
					if (!OUTTAIL || INVERSE) textbegin = curtextend;
					else if (DELIMITER) textbegin = curtextend - D_length;
					else textbegin = curtextend - 1;
				}
				DOWITHMASK = 1;
				amatched_terminals[pat_index-1] = 1;
				if (AComplexBoolean) {
					/* Can output only after all the matches in the current record have been identified: just like filter_output */
					oldtext = text;
					CurrentByteOffset += (oldtext + pat_len[pat_index] - 1 - text);
					text = oldtext + pat_len[pat_index] - 1;
					MATCHED = 0;
					goto skip_output;
				}
				else if ((long)AParse & AND_EXP) {
					for (j=0; j<anum_terminals; j++) if (!amatched_terminals[j]) break;
					if (j<anum_terminals) goto skip_output;
				}

				MATCHED = 1;
				oldtext = text; /* used only if MULTI_OUTPUT */

#undef	DO_OUTPUT
#define DO_OUTPUT(change_text)\
				num_of_matched++;\
				if(FILENAMEONLY || SILENT)  return 0;\
				if (!COUNT) {\
					PRINTED = print_options(pat_index, text, curtextbegin, curtextend);\
					if(!INVERSE) {\
						if (PRINTRECORD) {\
/* #if     MEASURE_TIMES\
						gettimeofday(&initt, NULL);\
#endif  MEASURE_TIMES*/\
						if (agrep_finalfp != NULL)\
							newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_finalfp, -1, EASYSEARCH);\
						else {\
							if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, curtextbegin, curtextend-curtextbegin, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {\
								if (newlen + agrep_outpointer >= agrep_outlen) {\
									OUTPUT_OVERFLOW;\
									return -1;\
								}\
								agrep_outpointer += newlen;\
							}\
						}\
/*#if     MEASURE_TIMES\
						gettimeofday(&finalt, NULL);\
						OUTFILTER_ms +=  (finalt.tv_sec* 1000 + finalt.tv_usec/1000) - (initt.tv_sec*1000 + initt.tv_usec/1000);\
#endif  MEASURE_TIMES*/\
						}\
						else if (PRINTED) {\
							if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);\
							else agrep_outbuffer[agrep_outpointer ++] = '\n';\
							PRINTED = 0;\
						}\
						if ((change_text) && MULTI_OUTPUT) {     /* next match starting from end of current */\
							CurrentByteOffset += (oldtext + tc_pat_len[pat_index] - 1 - text);\
							text = oldtext + tc_pat_len[pat_index] - 1;\
							MATCHED = 0;\
						}\
						else if (change_text) {\
							CurrentByteOffset += textbegin - text;\
							text = textbegin;\
						}\
					}\
					else {	/* INVERSE: Don't care about filtering time */\
						/* if(lastout < curtextbegin) OUT=1; */\
						if (agrep_finalfp != NULL)\
							newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_finalfp, -1, EASYSEARCH);\
						else {\
							if ((newlen = quick_tuncompress(FREQ_FILE, STRING_FILE, lastout, curtextbegin - lastout, agrep_outbuffer, agrep_outlen - agrep_outpointer, EASYSEARCH)) > 0) {\
								if (newlen + agrep_outpointer >= agrep_outlen) {\
									OUTPUT_OVERFLOW;\
									return -1;\
								}\
								agrep_outpointer += newlen;\
							}\
						}\
						lastout=textbegin;\
						if (change_text) {\
							CurrentByteOffset += textbegin - text;\
							text = textbegin;\
						}\
					}\
				}\
				else if (change_text) {\
					CurrentByteOffset += textbegin - text;\
					text = textbegin;\
				}\
				if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||\
				    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) return 0;	/* done */\

				DO_OUTPUT(1)
			}

		skip_output:
                        if(MATCHED && !MULTI_OUTPUT && !AComplexBoolean) break;     /* else look for more possible matches */
			if (DOWITHMASK && (text >= curtextend - 1)) {
				DOWITHMASK = 0;
				if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
					DO_OUTPUT(0)
				}
				if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
			}
		}
		/* If I found some match and I am about to cross over a delimiter, then set DOWITHMASK to 0 and zero out the amatched_terminals */
		if (DOWITHMASK && (text >= curtextend - 1)) {
			DOWITHMASK = 0;
			if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
				DO_OUTPUT(0)
			}
			if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
		}
		if (MATCHED) text--;
		MATCHED = 0;
	} /* while */
	CurrentByteOffset ++;

	/* Do residual stuff: check if there was a match at the end of the line | check if rest of the buffer needs to be output due to inverse */

	if (DOWITHMASK && (text >= curtextend - 1)) {
		DOWITHMASK = 0;
		if (AComplexBoolean && dd(curtextbegin, curtextend) && eval_tree(AParse, amatched_terminals)) {
			DO_OUTPUT(0)
		}
		if (AParse != 0) memset(amatched_terminals, '\0', anum_terminals);
	}

	if (INVERSE && !COUNT && (lastout <= textend)) {
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

        return 0;
}
#endif	/*DOTCOMPRESSED*/

static void
f_prep(pat_index, Pattern)
uchar *Pattern;   int pat_index;
{
int i, m;
register unsigned hash=0;
#ifdef debug
	puts(Pattern);
#endif
	m = p_size;
		for (i=m-1; i>=(1+LONG); i--) {
				hash = (tr1[Pattern[i]]);
				hash = (hash << Hbits) + (tr1[Pattern[i-1]]);
		if(LONG) hash = (hash << Hbits) + (tr1[Pattern[i-2]] );
		if(SHIFT1[hash] >= m-1-i) SHIFT1[hash] = m-1-i;
	}
	i=m-1;
		hash = (tr1[Pattern[i]]);
		hash = (hash << Hbits) + (tr1[Pattern[i-1]]);
	if(LONG) hash = (hash << Hbits) + (tr1[Pattern[i-2]] );
		if(SHORT) hash=tr[Pattern[0]];
#ifdef debug
	printf("hash = %d\n", hash);
#endif
		HASH[hash]++;
		return;
}

#if	DOTCOMPRESSED
static void
tc_f_prep(pat_index, Pattern)
uchar *Pattern;   int pat_index;
{
int i, m;
register unsigned hash=0;
#ifdef debug
	puts(Pattern);
#endif
	m = tc_p_size;
		for (i=m-1; i>=(1+tc_LONG); i--) {
				hash = (tc_tr1[Pattern[i]]);
				hash = (hash << Hbits) + (tc_tr1[Pattern[i-1]]);
		if(tc_LONG) hash = (hash << Hbits) + (tc_tr1[Pattern[i-2]] );
		if(tc_SHIFT1[hash] >= m-1-i) tc_SHIFT1[hash] = m-1-i;
	}
	i=m-1;
		hash = (tc_tr1[Pattern[i]]);
		hash = (hash << Hbits) + (tc_tr1[Pattern[i-1]]);
	if(tc_LONG) hash = (hash << Hbits) + (tc_tr1[Pattern[i-2]] );
		if(tc_SHORT) hash=tc_tr[Pattern[0]];
#ifdef debug
	printf("hash = %d\n", hash);
#endif
		tc_HASH[hash]++;
		return;
}
#endif	/*DOTCOMPRESSED*/

static void
f_prep1(pat_index, Pattern)
uchar *Pattern;   int pat_index;
{
int i, m;
int hash2;
register unsigned hash;
	m = p_size;
#ifdef debug
	puts(Pattern);
#endif
		for (i=m-1; i>=(1+LONG); i--) {
				hash = (tr1[Pattern[i]]);
				hash = (hash << Hbits) + (tr1[Pattern[i-1]]);
		if(LONG) hash = (hash << Hbits) + (tr1[Pattern[i-2]] );
		if(SHIFT1[hash] >= m-1-i) SHIFT1[hash] = m-1-i;
	}
	i=m-1;
		hash = (tr1[Pattern[i]]);
		hash = (hash << Hbits) + (tr1[Pattern[i-1]]);
	if(LONG) hash = (hash << Hbits) + (tr1[Pattern[i-2]] );
		if(SHORT) hash=tr[Pattern[0]];
	hash2 = (tr[Pattern[0]] << 8) + tr[Pattern[1]];
#ifdef debug
	printf("hash = %d, HASH[hash] = %d\n", hash, HASH[hash]);
#endif
		PatPtr[HASH[hash]] = Pattern;
		pat_indices[HASH[hash]] = pat_index;
	Hash2[HASH[hash]] = hash2;
		HASH[hash]--;
		return;
}

#if	DOTCOMPRESSED
static void
tc_f_prep1(pat_index, Pattern)
uchar *Pattern;   int pat_index;
{
int i, m;
int hash2;
register unsigned hash;
	m = tc_p_size;
#ifdef debug
	puts(Pattern);
#endif
		for (i=m-1; i>=(1+tc_LONG); i--) {
				hash = (tc_tr1[Pattern[i]]);
				hash = (hash << Hbits) + (tc_tr1[Pattern[i-1]]);
		if(tc_LONG) hash = (hash << Hbits) + (tc_tr1[Pattern[i-2]] );
		if(tc_SHIFT1[hash] >= m-1-i) tc_SHIFT1[hash] = m-1-i;
	}
	i=m-1;
		hash = (tc_tr1[Pattern[i]]);
		hash = (hash << Hbits) + (tc_tr1[Pattern[i-1]]);
	if(tc_LONG) hash = (hash << Hbits) + (tc_tr1[Pattern[i-2]] );
		if(tc_SHORT) hash=tc_tr[Pattern[0]];
	hash2 = (tc_tr[Pattern[0]] << 8) + tc_tr[Pattern[1]];
#ifdef debug
	printf("hash = %d, tc_HASH[hash] = %d\n", hash, tc_HASH[hash]);
#endif
		tc_PatPtr[tc_HASH[hash]] = Pattern;
		tc_pat_indices[tc_HASH[hash]] = pat_index;
	tc_Hash2[tc_HASH[hash]] = hash2;
		tc_HASH[hash]--;
		return;
}
#endif	/*DOTCOMPRESSED*/

static void
accumulate()
{
	int i;

	for(i=1; i<MAXHASH; i++)  {
	/*
	printf("%d, ", HASH[i]);
	*/
	HASH[i] = HASH[i-1] + HASH[i];
	}
	HASH[0] = 0;
	return;
}

#if	DOTCOMPRESSED
static void
tc_accumulate()
{
	int i;

	for(i=1; i<MAXHASH; i++)  {
	/*
	printf("%d, ", HASH[i]);
	*/
	tc_HASH[i] = tc_HASH[i-1] + tc_HASH[i];
	}
	tc_HASH[0] = 0;
	return;
}
#endif	/*DOTCOMPRESSED*/
