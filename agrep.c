
/*
 * bgopal: (1993-4) added a library interface and removed some bugs: also
 * selectively modified many routines to work with our text-compression algo.
 */
#include <sys/stat.h>
#include "agrep.h"
#include "checkfile.h"
#include <errno.h>

#define PRINT(s)

extern char **environ;
extern int errno;
int pattern_index;	/* index in argv where the pattern is */

int glimpse_isserver=0;	/* so that there is no user interaction */
int glimpse_call = 0;	/* So that usage message is not printed twice */
int glimpse_clientdied=0;/* to quit search if glimpseserver's client dies */
int  agrep_initialfd;	/* Where does input come from? File/Memory? */
CHAR *agrep_inbuffer;
int  agrep_inlen;
int  agrep_inpointer;

FILE *agrep_finalfp;	/* Where does output go to? File/Memory? */
CHAR *agrep_outbuffer;
int  agrep_outlen;
int  agrep_outpointer;

int  execfd;	/* used by exec called within agrep_search, set in agrep_init */
int  multifd = -1; /* fd for multipattern search used in ^^ , set in   ^^^^^^^^ */
extern char *pat_spool;
#if	DOTCOMPRESSED
extern char *tc_pat_spool;
#endif	/* DOTCOMPRESSED */
char *multibuf=NULL; /* buffer to put the multiple patterns in */
int  multilen = 0; /* length of the multibuf: not the #of multi-patterns! */

extern int pos_cnt;	/* to re-initialize it to 0 for reg-exp search */
unsigned Mask[MAXSYM];
unsigned Init1, NO_ERR_MASK, Init[MaxError];
unsigned Bit[WORD+1];
CHAR buffer[BlockSize+Maxline+1];	/* should not be used anywhere: 10/18/93 */
unsigned Next[MaxNext], Next1[MaxNext];
unsigned wildmask, endposition, D_endpos; 
int  LIMITOUTPUT;	/* maximum number of matches we are going to allow */
int  LIMITPERFILE;	/* maximum number of matches per file we are going to allow */
int  LIMITTOTALFILE;	/* maximum number of files we are going to allow */
int  EXITONERROR;	/* return -1 or exit on error? */
int  REGEX, FASTREGEX, RE_ERR, FNAME, WHOLELINE, SIMPLEPATTERN;
int  COUNT, HEAD, TAIL, LINENUM, INVERSE, I, S, DD, AND, SGREP, JUMP; 
int  NOOUTPUTZERO;
int  Num_Pat, PSIZE, prev_num_of_matched, num_of_matched, files_matched, SILENT, NOPROMPT, BESTMATCH, NOUPPER;
int  NOMATCH, TRUNCATE, FIRST_IN_RE, FIRSTOUTPUT;
int  WORDBOUND, DELIMITER, D_length, tc_D_length, original_D_length;
int  EATFIRST, OUTTAIL;
int  BYTECOUNT;
int  PRINTOFFSET;
int  PRINTRECORD;
int  PRINTNONEXISTENTFILE;
int  FILEOUT;
int  DNA;
int  APPROX;
int  PAT_FILE;	/* multiple patterns from a given file */
char PAT_FILE_NAME[MAX_LINE_LEN];
int  PAT_BUFFER; /* multiple patterns from a given buffer */
int  CONSTANT;
int  RECURSIVE;
int  total_line; /* used in mgrep */
int  D;
int  M;
int  TCOMPRESSED;
int  EASYSEARCH;	/* 1 used only for compressed files: LITTLE/BIG */
int  ALWAYSFILENAME = OFF;
int  POST_FILTER = OFF;
int  NEW_FILE = OFF;	/* only when post-filter is used */
int  PRINTFILENUMBER = OFF;
int  PRINTFILETIME = OFF;
int  PRINTPATTERN = OFF;
int  MULTI_OUTPUT = OFF; /* should mgrep print the matched line multiple times for each matched pattern or just once? */
/* invisible to the user, used only by glimpse: cannot use -l since it is incompatible with stdin and -A is used for the index search (done next) */

/* Stuff to handle complicated boolean patterns */
int  AComplexBoolean = 0;
ParseTree *AParse = NULL;
int anum_terminals = 0;
ParseTree aterminals[MAXNUM_PAT];
char amatched_terminals[MAXNUM_PAT];
char aduplicates[MAXNUM_PAT][MAXNUM_PAT];	/* tells what other patterns are exactly equal to the i-th one */
char tc_aduplicates[MAXNUM_PAT][MAXNUM_PAT];	/* tells what other patterns are exactly equal to the i-th one */

#if	MEASURE_TIMES
/* timing variables */
int OUTFILTER_ms;
int FILTERALGO_ms;
int INFILTER_ms;
#endif	/*MEASURE_TIMES*/

CHAR **Textfiles = NULL;     /* array of filenames to be searched */
int Numfiles = 0;    /* indicates how many files in Textfiles */
int copied_from_argv = 0; /* were filenames copied from argv (should I free 'em)? */
CHAR old_D_pat[MaxDelimit * 2] = "\n";  /* to hold original D_pattern */
CHAR original_old_D_pat[MaxDelimit * 2] = "\n";
CHAR Pattern[MAXPAT], OldPattern[MAXPAT];
CHAR CurrentFileName[MAX_LINE_LEN];
long CurrentFileTime;
int SetCurrentFileName = 0;	/* dirty glimpse trick to make filters work: output seems to come from another file */
int SetCurrentFileTime = 0;	/* dirty glimpse trick to avoid doing a stat to find the time */
int CurrentByteOffset;
int SetCurrentByteOffset = 0;
CHAR Progname[MAXNAME]; 
CHAR D_pattern[MaxDelimit * 2] = "\n; "; /* string which delimits records -- defaults to newline */
CHAR tc_D_pattern[MaxDelimit * 2] = "\n";
CHAR original_D_pattern[MaxDelimit * 2] = "\n; ";
char COMP_DIR[MAX_LINE_LEN];
char FREQ_FILE[MAX_LINE_LEN], HASH_FILE[MAX_LINE_LEN], STRING_FILE[MAX_LINE_LEN];	/* interfacing with tcompress */

int  NOFILENAME,  /* Boolean flag, set for -h option */
     FILENAMEONLY;/* Boolean flag, set for -l option */
extern int init();
int table[WORD][WORD];
CHAR *agrep_saved_pattern = NULL;	/* to prevent multiple prepfs for each boolean search: crd@hplb.hpl.hp.com */

long
aget_file_time(stbuf, name)
	struct stat *stbuf;
	char	*name;
{
	long	ret = 0;
	struct stat mystbuf;
	if (stbuf != NULL) ret = stbuf->st_mtime;
	else {
		if (my_stat(name, &mystbuf) == -1) ret = 0;
		else ret = mystbuf.st_mtime;
	}
	return ret;
}

char *
aprint_file_time(thetime)
	time_t	thetime;
{
#if	0
	char	s[256], s1[16], s2[16], s3[16], s4[16], s5[16];
	static char buffer[256];

	strcpy(s, ctime(&thetime));	/* of the form: Sun Sep 16 01:03:52 1973\n\0 */
	s[strlen(s) - 1] = '\0';
	sscanf(s, "%s%s%s%s%s", s1, s2, s3, s4, s5);
	sprintf(buffer, ": %s %s %s", s2, s3, s5);	/* ditch Sun 01:03:52 */
#else
	static char buffer[256];
	buffer[0] = ':';
	buffer[1] = ' ';
	strftime(&buffer[2], 256, "%h %e %Y", gmtime(&thetime));
#endif
	return &buffer[0];
}

/* Called when multipattern search and pattern has not changed */
void
reinit_value_partial()
{
	num_of_matched = prev_num_of_matched = 0;
	errno = 0;
	FIRST_IN_RE = ON;
}

/* This must be called before every agrep_search to reset agrep globals */
void
reinit_value()
{
        int i, j;

	/* Added on 7th Oct 194 */
	if (AParse) {
		if (AComplexBoolean) destroy_tree(AParse);
		AComplexBoolean = 0;
		AParse = 0;
		PAT_BUFFER = 0;
		if (multibuf != NULL) free(multibuf);	/* this was allocated for arbit booleans, not multipattern search */
		multibuf = NULL;
		multilen = 0;
		/* Cannot free multifd here since that is always allocated for multipattern search */
	}
	for (i=0; i<anum_terminals; i++) {
		free(aterminals[i].data.leaf.value);
		memset(&aterminals[i], '\0', sizeof(ParseTree));
	}
	anum_terminals = 0;
	for (i=0; i<MAXNUM_PAT; i++) memset(aduplicates[i], '\0', MAXNUM_PAT);
	for (i=0; i<MAXNUM_PAT; i++) memset(tc_aduplicates[i], '\0', MAXNUM_PAT);

        Bit[WORD] = 1;
        for (i = WORD - 1; i > 0  ; i--)  Bit[i] = Bit[i+1] << 1;
        for (i=0; i< MAXSYM; i++) Mask[i] = 0;

        /* bg: new things added on Mar 13 94 */
        Init1 = 0;
        NO_ERR_MASK = 0;
        memset(Init, '\0', MaxError * sizeof(unsigned));
        memset(Next, '\0', MaxNext * sizeof(unsigned));
        memset(Next1, '\0', MaxNext * sizeof(unsigned));
        wildmask = endposition = D_endpos = 0;
        for (i=0; i<WORD; i++)
                for (j=0; j<WORD; j++)
                        table[i][j] = 0;

        strcpy(D_pattern, original_D_pattern);
        D_length = original_D_length;
        strcpy(old_D_pat, original_old_D_pat);

	/* Changed on Dec 26th: bg */
	FASTREGEX = REGEX = 0;
	HEAD = TAIL = ON;	/* were off initially */
	RE_ERR = 0;
	AND = 0;
	M = 0;
	pos_cnt = 0;	/* added 31 Jan 95 */

	reinit_value_partial();
}

/* This must be called before every agrep_init to reset agrep options */
void
initial_value()
{
	SetCurrentFileName = 0;	/* 16/9/94 */
	SetCurrentFileTime = 0;
	SetCurrentByteOffset = 0;	/* 23/9/94 */

	/* courtesy: crd@hplb.hpl.hp.com */
	if (agrep_saved_pattern) {
		free(agrep_saved_pattern);
		agrep_saved_pattern= NULL;
	}
	/* bg: new stuff on 17/Feb/94 */
	if (multifd != -1) close(multifd);
	multifd = -1;
	if (multibuf != NULL) free(multibuf);
	multibuf = NULL;
	multilen = 0;
	if (pat_spool != NULL) free(pat_spool);
	pat_spool = NULL;
#if	DOTCOMPRESSED
	if (tc_pat_spool != NULL) free(tc_pat_spool);
	tc_pat_spool = NULL;
#endif	/* DOTCOMPRESSED */
	LIMITOUTPUT = 0;	/* means infinity = current semantics */
	LIMITPERFILE = 0;	/* means infinity = current semantics */
	LIMITTOTALFILE = 0;	/* means infinity = current semantics */
	EASYSEARCH = 1;
	DNA = APPROX = PAT_FILE = PAT_BUFFER = CONSTANT = total_line = D = TCOMPRESSED = 0;
	PAT_FILE_NAME[0] = '\0';
	EXITONERROR = NOFILENAME = FILENAMEONLY = FILEOUT = ALWAYSFILENAME = NEW_FILE = POST_FILTER = 0;

        original_old_D_pat[0] = old_D_pat[0] = '\n';
        original_old_D_pat[1] = old_D_pat[1] = '\0';
        original_D_pattern[0] = D_pattern[0] = '\n';
        original_D_pattern[1] = D_pattern[1] = ';';
        original_D_pattern[2] = D_pattern[2] = ' ';
        original_D_pattern[3] = D_pattern[3] = '\0';

	strcpy(tc_D_pattern, "\n");
	tc_D_length = 1;

	/* the functions agrep_init and agrep_search take care of Textfiles and Numfiles */
	agrep_inpointer = 0;
	agrep_outpointer = 0;
	agrep_outlen = 0;
#if	MEASURE_TIMES
	OUTFILTER_ms = FILTERALGO_ms = INFILTER_ms = 0;
#endif	/*MEASURE_TIMES*/

	MULTI_OUTPUT = 0;
	PRINTPATTERN = 0;
	PRINTFILENUMBER = 0;
	PRINTFILETIME = 0;
	JUMP = FNAME = BESTMATCH = NOPROMPT = NOUPPER = 0;
	RECURSIVE = 0;
	COUNT = LINENUM = WHOLELINE = SGREP = 0;
	NOOUTPUTZERO = 0;
	EATFIRST = INVERSE = TRUNCATE = OUTTAIL = 0; 
	NOMATCH = FIRSTOUTPUT = ON;	/* were off initally */
	I = DD = S = 1;	/* were off initially */
	original_D_length = D_length = 2;	/* was 0 initially */
	SILENT = Num_Pat = PSIZE = SIMPLEPATTERN = prev_num_of_matched = num_of_matched = files_matched = 0;
	WORDBOUND = DELIMITER = 0;

	COMP_DIR[0] = '\0';
	FREQ_FILE[0] = '\0';
	HASH_FILE[0] = '\0';
	STRING_FILE[0] = '\0';
	BYTECOUNT = OFF;
	PRINTOFFSET = OFF;
	PRINTRECORD = ON;
	PRINTNONEXISTENTFILE = OFF;

	glimpse_clientdied = 0;	/* added 15th Feb 95 */

	/* Pattern, OldPattern, execfd, Numfiles are set in agrep_init: so no need to initialize */
	reinit_value();
}

void
compute_next(M, Next, Next1)
int M; 
unsigned *Next, *Next1;
{
	int i, j=0, n,  k, temp;
	int mid, pp;
	int MM, base;
	unsigned V[WORD];

	base = WORD - M;
	temp = Bit[base]; 
	Bit[base] = 0;
	for (i=0; i<WORD; i++) V[i] = 0;
	for (i=1; i<M; i++)
	{  
		j=0;
		while (table[i][j] > 0 && j < 10) {
			V[i] = V[i] | Bit[base + table[i][j++]];
		}
	}
	Bit[base]=temp;
	if(M <= SHORTREG)
	{
		k = exponen(M);
		pp = 2*k;
		for(i=k; i<pp ; i++)
		{   
			n = i;
			Next[i]= (k>>1);
			for(j=M; j>=1; j--)
			{
				if(n & Bit[WORD]) Next[i] = Next[i] | V[j];
				n = (n>>1);
			}
		}      
		return;
	}
	if(M > MAXREG) fprintf(stderr, "%s: regular expression too long\n", Progname);
	MM = M;
	if(M & 1) M=M+1;
	k = exponen(M/2);
	pp = 2*k;
	mid = MM/2;
	for(i=k; i<pp ; i++)
	{     
		n = i;
		Next[i]= (Bit[base]>>1);
		for(j=MM; j>mid ; j--)
		{
			if(n & Bit[WORD]) Next[i] = Next[i] | V[j-mid];
			n = (n>>1);
		}
		n=i-k;
		Next1[i-k] = 0;
		for(j = 0; j<mid; j++)
		{
			if(n & Bit[WORD]) Next1[i-k] = Next1[i-k] | V[MM-j];
			n = (n>>1);
		}
	}      
	return;
}

int
exponen(m)
int m;
{ 
	int i, ex;
	ex= 1;
	for (i=0; i<m; i++) ex <<= 1;	/* was ex *= 2 */
	return(ex);
}

int
re1(Text, M, D)
int Text, M, D;
{
	register unsigned i, c, r0, r1, r2, r3, CMask, Newline, Init0, r_NO_ERR; 
	register unsigned end;
	register unsigned hh, LL=0, k;  /* Lower part */
	int  FIRST_TIME=ON, num_read , j=0, base;
	unsigned A[MaxRerror+1], B[MaxRerror+1];
	unsigned Next[MaxNext], Next1[MaxNext];
	CHAR *buffer;
	int FIRST_LOOP = 1;

	r_NO_ERR = NO_ERR_MASK;
	if(M > MAXREG) {
		fprintf(stderr, "%s: regular expression too long, max is %d\n", Progname,MAXREG);
		if (!EXITONERROR){
			errno = AGREP_ERROR;
			return -1;
		}
		else exit(2);
	}
	base = WORD - M;
	hh = M/2;
	for(i=WORD, j=0; j < hh ; i--, j++) LL = LL | Bit[i];
	if(FIRST_IN_RE) compute_next(M, Next, Next1); 
	/*SUN: try: change to memory allocation */
	FIRST_IN_RE = 0;
	Newline = '\n';
	Init[0] = Bit[base];
	if(HEAD) Init[0] = Init[0] | Bit[base+1];
	for(i=1; i<= D; i++) Init[i] = Init[i-1] | Next[Init[i-1]>>hh] | Next1[Init[i-1]&LL];
	Init1 = Init[0] | 1; 
	Init0 = Init[0];
	r2 = r3 = Init[0];
	for(k=0; k<= D; k++) { 
		A[k] = B[k] = Init[k]; 
	}

	if ( D == 0 )
	{
#if	AGREP_POINTER
	    if (Text != -1)
	    {
#endif	/*AGREP_POINTER*/
		alloc_buf(Text, &buffer, BlockSize+Maxline+1);
		while ((num_read = fill_buf(Text, buffer + Maxline, BlockSize)) > 0)
		{
			i=Maxline; 
			end = num_read + Maxline;
#if 0
			/* pab: Don't do this here; it's done in bitap.fill_buf,
			 * where we can handle eof on a block boundary right */
			if((num_read < BlockSize) && buffer[end-1] != '\n') buffer[end++] = '\n';
#endif /* 0 */
			if(FIRST_LOOP) {         /* if first time in the loop add a newline */
				buffer[i-1] = '\n';  /* in front the  text.  */
				i--;
				CurrentByteOffset --;
				FIRST_LOOP = 0;
			}

			/* RE1_PROCESS_WHEN_DZERO: the while-loop below */
			while ( i < end )
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				if(c != Newline)
				{
					if(CMask != 0) {
						r1 = Init1 & r3;
						r2 = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | r1;
					}
					else  {
						r2 = r3 & Init1;
					}
				}
				else {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & r3;            /* match against endofline */
					r2 = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | r1;
					if(TAIL) r2 = (Next[r2>>hh] | Next1[r2&LL]) | r2;                                        /* epsilon move */
					if(( r2 & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					r3 = Init0;
					r2 = (Next[r3>>hh] | Next1[r3&LL]) & CMask | Init0;
					/* match begin of line */
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				if(c != Newline)
				{
					if(CMask != 0) {
						r1 = Init1 & r2;
						r3 = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
					}
					else   r3 = r2 & Init1;
				} /* if(NOT Newline) */
				else {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & r2;            /* match against endofline */
					r3 = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
					if(TAIL) r3 = ( Next[r3>>hh] | Next1[r3&LL] ) | r3;
					/* epsilon move */
					if(( r3 & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					r2 = Init0;
					r3 = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | Init0;
					/* match begin of line */
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
			} /* while i < end ... */

			strncpy(buffer, buffer+num_read, Maxline);
		} /* end while fill_buf()... */

		free_buf(Text, buffer);
		return 0;
#if	AGREP_POINTER
	    }
	    else {	/* within the memory buffer: assume it starts with a newline at position 0, the actual pattern follows that, and it ends with a '\n' */
		num_read = agrep_inlen;
		buffer = (CHAR *)agrep_inbuffer;
		end = num_read;
		/* buffer[end-1] = '\n';*/ /* at end of the text. */
		/* buffer[0] = '\n';*/  /* in front of the  text. */
		i = 0;

			/* An exact copy of the above RE1_PROCESS_WHEN_DZERO: the while-loop below */
			while ( i < end )
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				if(c != Newline)
				{
					if(CMask != 0) {
						r1 = Init1 & r3;
						r2 = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | r1;
					}
					else  {
						r2 = r3 & Init1;
					}
				}
				else {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & r3;            /* match against endofline */
					r2 = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | r1;
					if(TAIL) r2 = (Next[r2>>hh] | Next1[r2&LL]) | r2;                                        /* epsilon move */
					if(( r2 & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					r3 = Init0;
					r2 = (Next[r3>>hh] | Next1[r3&LL]) & CMask | Init0;
					/* match begin of line */
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				if(c != Newline)
				{
					if(CMask != 0) {
						r1 = Init1 & r2;
						r3 = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
					}
					else   r3 = r2 & Init1;
				} /* if(NOT Newline) */
				else {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & r2;            /* match against endofline */
					r3 = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
					if(TAIL) r3 = ( Next[r3>>hh] | Next1[r3&LL] ) | r3;
					/* epsilon move */
					if(( r3 & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					r2 = Init0;
					r3 = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | Init0;
					/* match begin of line */
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
			} /* while i < end ... */

		return 0;
	    }
#endif	/*AGREP_POINTER*/
	} /*  end if (D == 0) */

#if	AGREP_POINTER
	if (Text != -1)
	{
#endif	/*AGREP_POINTER*/
		while ((num_read = fill_buf(Text, buffer + Maxline, BlockSize)) > 0)
		{
			i=Maxline; 
			end = Maxline + num_read;
#if 0
			/* pab: Don't do this here; it's done in bitap.fill_buf,
			 * where we can handle eof on a block boundary right */
			if((num_read < BlockSize) && buffer[end-1] != '\n') buffer[end++] = '\n';
#endif /* 0 */
			if(FIRST_TIME) {         /* if first time in the loop add a newline */
				buffer[i-1] = '\n';  /* in front the  text.  */
				i--;
				CurrentByteOffset --;
				FIRST_TIME = 0;
			}

			/* RE1_PROCESS_WHEN_DNOTZERO: the while loop below */
			while (i < end )
			{
				c = buffer[i];
				CMask = Mask[c];
				if(c !=  Newline)
				{
					if(CMask != 0) {  
						r2 = B[0];
						r1 = Init1 & r2;
						A[0] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
						r3 = B[1];
						r1 = Init1 & r3;
						r0 = r2 | A[0];     /* A[0] | B[0] */
						A[1] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | (( r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextcharfile;
						r2 = B[2];
						r1 = Init1 & r2;
						r0 = r3 | A[1];
						A[2] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextcharfile;
						r3 = B[3];
						r1 = Init1 & r3;
						r0 = r2 | A[2];
						A[3] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextcharfile;
						r2 = B[4];
						r1 = Init1 & r2;
						r0 = r3 | A[3];
						A[4] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4)  goto Nextcharfile;
					}  /* if(CMask) */
					else  {
						r2 = B[0];
						A[0] = r2 & Init1; 
						r3 = B[1];
						r1 = Init1 & r3;
						r0 = r2 | A[0];
						A[1] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextcharfile;
						r2 = B[2];
						r1 = Init1 & r2;
						r0 = r3 | A[1];
						A[2] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextcharfile;
						r3 = B[3];
						r1 = Init1 & r3;
						r0 = r2 | A[2];
						A[3] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextcharfile;
						r2 = B[4];
						r1 = Init1 & r2;
						r0 = r3 | A[3];
						A[4] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4) goto Nextcharfile;
					}
				}
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & B[D];            /* match against endofline */
					A[D] = ((Next[B[D]>>hh] | Next1[B[D]&LL]) & CMask) | r1;
					if(TAIL) A[D] = ( Next[A[D]>>hh] | Next1[A[D]&LL] ) | A[D]; 
					/* epsilon move */
					if(( A[D] & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<=D; k++)  B[k] = Init[0];
					r1 = Init1 & B[0];
					A[0] = (( Next[B[0]>>hh] | Next1[B[0]&LL]) & CMask) | r1;
					for(k=1; k<=D; k++) {
						r3 = B[k];
						r1 = Init1 & r3;
						r2 = A[k-1] | B[k-1];
						A[k] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((B[k-1] | Next[r2>>hh] | Next1[r2&LL]) & r_NO_ERR) | r1;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextcharfile: 
				i=i+1;
				CurrentByteOffset ++;
				c = buffer[i];
				CMask = Mask[c];
				if(c != Newline)
				{
					if(CMask != 0) {  
						r2 = A[0];
						r1 = Init1 & r2;
						B[0] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
						r3 = A[1];
						r1 = Init1 & r3;
						r0 = B[0] | r2;
						B[1] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((r2 | Next[r0>>hh] | Next1[r0&LL]) & r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextchar1file;
						r2 = A[2];
						r1 = Init1 & r2;
						r0 = B[1] | r3;
						B[2] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextchar1file;
						r3 = A[3];
						r1 = Init1 & r3;
						r0 = B[2] | r2;
						B[3] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextchar1file;
						r2 = A[4];
						r1 = Init1 & r2;
						r0 = B[3] | r3;
						B[4] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4)   goto Nextchar1file;
					}  /* if(CMask) */
					else  {
						r2 = A[0];
						B[0] = r2 & Init1; 
						r3 = A[1];
						r1 = Init1 & r3;
						r0 = B[0] | r2;
						B[1] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextchar1file;
						r2 = A[2];
						r1 = Init1 & r2;
						r0 = B[1] | r3;
						B[2] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextchar1file;
						r3 = A[3];
						r1 = Init1 & r3;
						r0 = B[2] | r2;
						B[3] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextchar1file;
						r2 = A[4];
						r1 = Init1 & r2;
						r0 = B[3] | r3;
						B[4] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4) goto Nextchar1file;
					}
				} /* if(NOT Newline) */
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & A[D];            /* match against endofline */
					B[D] = ((Next[A[D]>>hh] | Next1[A[D]&LL]) & CMask) | r1;
					if(TAIL) B[D] = ( Next[B[D]>>hh] | Next1[B[D]&LL] ) | B[D]; 
					/* epsilon move */
					if(( B[D] & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<=D; k++) A[k] = Init0; 
					r1 = Init1 & A[0];
					B[0] = ((Next[A[0]>>hh] | Next1[A[0]&LL]) & CMask) | r1;
					for(k=1; k<=D; k++) {
						r3 = A[k];
						r1 = Init1 & r3;
						r2 = A[k-1] | B[k-1];
						B[k] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((A[k-1] | Next[r2>>hh] | Next1[r2&LL]) & r_NO_ERR) | r1;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextchar1file: 
				i=i+1;
				CurrentByteOffset ++;
			} /* while i < end */

			strncpy(buffer, buffer+num_read, Maxline);
		} /* while fill_buf... */
		free_buf(Text, buffer);
		return 0;
#if	AGREP_POINTER
	}
	else {	/* within the memory buffer: assume it starts with a newline at position 0, the actual pattern follows that, and it ends with a '\n' */
		num_read = agrep_inlen;
		buffer = (CHAR *)agrep_inbuffer;
		end = num_read;
		/* buffer[end-1] = '\n';*/ /* at end of the text. */
		/* buffer[0] = '\n';*/  /* in front of the  text. */
		i = 0;

			/* An exact copy of the above RE1_PROCESS_WHEN_DNOTZERO: the while loop below */
			while (i < end )
			{
				c = buffer[i];
				CMask = Mask[c];
				if(c !=  Newline)
				{
					if(CMask != 0) {  
						r2 = B[0];
						r1 = Init1 & r2;
						A[0] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
						r3 = B[1];
						r1 = Init1 & r3;
						r0 = r2 | A[0];     /* A[0] | B[0] */
						A[1] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | (( r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextcharmem;
						r2 = B[2];
						r1 = Init1 & r2;
						r0 = r3 | A[1];
						A[2] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextcharmem;
						r3 = B[3];
						r1 = Init1 & r3;
						r0 = r2 | A[2];
						A[3] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextcharmem;
						r2 = B[4];
						r1 = Init1 & r2;
						r0 = r3 | A[3];
						A[4] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4)  goto Nextcharmem;
					}  /* if(CMask) */
					else  {
						r2 = B[0];
						A[0] = r2 & Init1; 
						r3 = B[1];
						r1 = Init1 & r3;
						r0 = r2 | A[0];
						A[1] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextcharmem;
						r2 = B[2];
						r1 = Init1 & r2;
						r0 = r3 | A[1];
						A[2] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextcharmem;
						r3 = B[3];
						r1 = Init1 & r3;
						r0 = r2 | A[2];
						A[3] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextcharmem;
						r2 = B[4];
						r1 = Init1 & r2;
						r0 = r3 | A[3];
						A[4] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4) goto Nextcharmem;
					}
				}
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & B[D];            /* match against endofline */
					A[D] = ((Next[B[D]>>hh] | Next1[B[D]&LL]) & CMask) | r1;
					if(TAIL) A[D] = ( Next[A[D]>>hh] | Next1[A[D]&LL] ) | A[D]; 
					/* epsilon move */
					if(( A[D] & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<=D; k++)  B[k] = Init[0];
					r1 = Init1 & B[0];
					A[0] = (( Next[B[0]>>hh] | Next1[B[0]&LL]) & CMask) | r1;
					for(k=1; k<=D; k++) {
						r3 = B[k];
						r1 = Init1 & r3;
						r2 = A[k-1] | B[k-1];
						A[k] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((B[k-1] | Next[r2>>hh] | Next1[r2&LL]) & r_NO_ERR) | r1;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextcharmem: 
				i=i+1;
				CurrentByteOffset ++;
				c = buffer[i];
				CMask = Mask[c];
				if(c != Newline)
				{
					if(CMask != 0) {  
						r2 = A[0];
						r1 = Init1 & r2;
						B[0] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | r1;
						r3 = A[1];
						r1 = Init1 & r3;
						r0 = B[0] | r2;
						B[1] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((r2 | Next[r0>>hh] | Next1[r0&LL]) & r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextchar1mem;
						r2 = A[2];
						r1 = Init1 & r2;
						r0 = B[1] | r3;
						B[2] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextchar1mem;
						r3 = A[3];
						r1 = Init1 & r3;
						r0 = B[2] | r2;
						B[3] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextchar1mem;
						r2 = A[4];
						r1 = Init1 & r2;
						r0 = B[3] | r3;
						B[4] = ((Next[r2>>hh] | Next1[r2&LL]) & CMask) | ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4)   goto Nextchar1mem;
					}  /* if(CMask) */
					else  {
						r2 = A[0];
						B[0] = r2 & Init1; 
						r3 = A[1];
						r1 = Init1 & r3;
						r0 = B[0] | r2;
						B[1] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 1) goto Nextchar1mem;
						r2 = A[2];
						r1 = Init1 & r2;
						r0 = B[1] | r3;
						B[2] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 2) goto Nextchar1mem;
						r3 = A[3];
						r1 = Init1 & r3;
						r0 = B[2] | r2;
						B[3] = ((r2 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 3) goto Nextchar1mem;
						r2 = A[4];
						r1 = Init1 & r2;
						r0 = B[3] | r3;
						B[4] = ((r3 | Next[r0>>hh] | Next1[r0&LL])&r_NO_ERR) | r1 ;  
						if(D == 4) goto Nextchar1mem;
					}
				} /* if(NOT Newline) */
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & A[D];            /* match against endofline */
					B[D] = ((Next[A[D]>>hh] | Next1[A[D]&LL]) & CMask) | r1;
					if(TAIL) B[D] = ( Next[B[D]>>hh] | Next1[B[D]&LL] ) | B[D]; 
					/* epsilon move */
					if(( B[D] & 1 ) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<=D; k++) A[k] = Init0; 
					r1 = Init1 & A[0];
					B[0] = ((Next[A[0]>>hh] | Next1[A[0]&LL]) & CMask) | r1;
					for(k=1; k<=D; k++) {
						r3 = A[k];
						r1 = Init1 & r3;
						r2 = A[k-1] | B[k-1];
						B[k] = ((Next[r3>>hh] | Next1[r3&LL]) & CMask) | ((A[k-1] | Next[r2>>hh] | Next1[r2&LL]) & r_NO_ERR) | r1;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextchar1mem: 
				i=i+1;
				CurrentByteOffset ++;
			} /* while i < end */

		return 0;
	}
#endif	/*AGREP_POINTER*/
} /* re1 */

int
re(Text, M, D)
int Text, M, D;
{
	register unsigned i, c, r1, r2, r3, CMask, k, Newline, Init0, Init1, end; 
	register unsigned r_even, r_odd, r_NO_ERR ;
	unsigned RMask[MAXSYM];
	unsigned A[MaxRerror+1], B[MaxRerror+1];
	int num_read, j=0, lasti, base, ResidueSize; 
	int FIRST_TIME; /* Flag */
	CHAR *buffer;

	base = WORD - M;
	k = 2*exponen(M);
	if(FIRST_IN_RE) {
		compute_next(M, Next, Next1); 
		FIRST_IN_RE = 0;    
	}
	for(i=0; i< MAXSYM; i++) RMask[i] = Mask[i];
	r_NO_ERR = NO_ERR_MASK;
	Newline = '\n';
	Init0 = Init[0] = Bit[base];
	if(HEAD) Init0  = Init[0] = Init0 | Bit[base+1] ;
	for(i=1; i<= D; i++) Init[i] = Init[i-1] | Next[Init[i-1]]; /* can be out? */
	Init1 = Init0 | 1; 
	r2 = r3 = Init0;
	for(k=0; k<= D; k++) { 
		A[k] = B[k] = Init[0]; 
	}  /* can be out? */
	FIRST_TIME = ON;
	alloc_buf(Text, &buffer, BlockSize+Maxline+1);
	if ( D == 0 )
	{
#if	AGREP_POINTER
	    if(Text != -1) {
#endif	/*AGREP_POINTER*/
		lasti = Maxline;
		while ((num_read = fill_buf(Text, buffer + Maxline, BlockSize)) > 0)
		{
			i=Maxline;
			end = Maxline + num_read ;
#if 0
			/* pab: Don't do this here; it's done in bitap.fill_buf,
			 * where we can handle eof on a block boundary right */
			if((num_read < BlockSize) && buffer[end-1] != '\n') buffer[end++] = '\n';
#endif /* 0 */
			if(FIRST_TIME) {
				buffer[i-1] = '\n';
				i--;
				CurrentByteOffset --;
				FIRST_TIME = 0;
			}

			/* RE_PROCESS_WHEN_DZERO: the while-loop below */
			while (i < end) 
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = RMask[c];
				if(c != Newline)
				{  
					r1 = Init1 & r3;
					r2 = (Next[r3] & CMask) | r1;
				}
				else {  
					r1 = Init1 & r3;            /* match against '\n' */
					r2 = Next[r3] & CMask | r1;
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(TAIL) r2 = Next[r2] | r2 ;   /* epsilon move */
					if(( r2 & 1) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - 1;
					r3 = Init0;
					r2 = (Next[r3] & CMask) | Init0;
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];   
				CurrentByteOffset ++;
				CMask = RMask[c];
				if(c != Newline)
				{
					r1 = Init1 & r2;
					r3 = (Next[r2] & CMask) | r1;
				}
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & r2;            /* match against endofline */
					r3 = Next[r2] & CMask | r1;
					if(TAIL) r3 = Next[r3] | r3;
					if(( r3 & 1) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - 1;
					r2 = Init0; 
					r3 = (Next[r2] & CMask) | Init0;  /* match the newline */
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
			} /* while */

			ResidueSize = Maxline + num_read - lasti;
			if(ResidueSize > Maxline) {
				ResidueSize = Maxline;  
			}
			strncpy(buffer+Maxline-ResidueSize, buffer+lasti, ResidueSize);
			lasti = Maxline - ResidueSize;
		} /* while fill_buf() */
		free_buf(Text, buffer);
		return 0;
#if	AGREP_POINTER
	    }
	    else {
		num_read = agrep_inlen;
		buffer = (CHAR *)agrep_inbuffer;
		end = num_read;
		/* buffer[end-1] = '\n';*/ /* at end of the text. */
		/* buffer[0] = '\n';*/  /* in front of the  text. */
		i = 0;
		lasti = 1;

			/* An exact copy of the above RE_PROCESS_WHEN_DZERO: the while-loop below */
			while (i < end) 
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = RMask[c];
				if(c != Newline)
				{  
					r1 = Init1 & r3;
					r2 = (Next[r3] & CMask) | r1;
				}
				else {  
					r1 = Init1 & r3;            /* match against '\n' */
					r2 = Next[r3] & CMask | r1;
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(TAIL) r2 = Next[r2] | r2 ;   /* epsilon move */
					if(( r2 & 1) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - 1;
					r3 = Init0;
					r2 = (Next[r3] & CMask) | Init0;
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];   
				CurrentByteOffset ++;
				CMask = RMask[c];
				if(c != Newline)
				{
					r1 = Init1 & r2;
					r3 = (Next[r2] & CMask) | r1;
				}
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & r2;            /* match against endofline */
					r3 = Next[r2] & CMask | r1;
					if(TAIL) r3 = Next[r3] | r3;
					if(( r3 & 1) ^ INVERSE) {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - 1;
					r2 = Init0; 
					r3 = (Next[r2] & CMask) | Init0;  /* match the newline */
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
			} /* while */

		/* If a residue is left for within-memory-buffer, since nothing can be "read" after that, we can ignore it: as if only 1 iteration of while */
		return 0;
	    }
#endif	/*AGREP_POINTER*/
	} /* end if(D==0) */

#if	AGREP_POINTER
	if (Text != -1) {
#endif	/*AGREP_POINTER*/
		while ((num_read = fill_buf(Text, buffer + Maxline, BlockSize)) > 0)
		{
			i=Maxline; 
			end = Maxline+num_read;
#if 0
			/* pab: Don't do this here; it's done in bitap.fill_buf,
			 * where we can handle eof on a block boundary right */
			if((num_read < BlockSize) && buffer[end-1] != '\n') buffer[end++] = '\n';
#endif /* 0 */
			if(FIRST_TIME) {
				buffer[i-1] = '\n';
				i--;
				CurrentByteOffset --;
				FIRST_TIME = 0;
			}

			/* RE_PROCESS_WHEN_DNOTZERO: the while-loop below */
			while (i < end)
			{   
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = RMask[c];
				if (c != Newline)
				{  
					r_even = B[0];
					r1 = Init1 & r_even;
					A[0] = (Next[r_even] & CMask) | r1;
					r_odd = B[1];
					r1 = Init1 & r_odd;
					r2 = (r_even | Next[r_even|A[0]]) &r_NO_ERR;
					A[1] = (Next[r_odd] & CMask) | r2 | r1 ;  
					if(D == 1) goto Nextcharfile;
					r_even = B[2];
					r1 = Init1 & r_even;
					r2 = (r_odd | Next[r_odd|A[1]]) &r_NO_ERR;
					A[2] = (Next[r_even] & CMask) | r2 | r1 ;  
					if(D == 2) goto Nextcharfile;
					r_odd = B[3];
					r1 = Init1 & r_odd;
					r2 = (r_even | Next[r_even|A[2]]) &r_NO_ERR;
					A[3] = (Next[r_odd] & CMask) | r2 | r1 ;  
					if(D == 3) goto Nextcharfile;
					r_even = B[4];
					r1 = Init1 & r_even;
					r2 = (r_odd | Next[r_odd|A[3]]) &r_NO_ERR;
					A[4] = (Next[r_even] & CMask) | r2 | r1 ;  
					goto Nextcharfile;
				} /* if NOT Newline */
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & B[D];               /* match endofline */
					A[D] = (Next[B[D]] & CMask) | r1;
					if(TAIL) A[D] = Next[A[D]] | A[D];
					if((A[D] & 1) ^ INVERSE )  {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;    

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}  
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<= D; k++) { 
						A[k] = B[k] = Init[k]; 
					}
					r1 = Init1 & B[0]; 
					A[0] = (Next[B[0]] & CMask) | r1;
					for(k=1; k<= D; k++) {
						r1 = Init1 & B[k];
						r2 = (B[k-1] | Next[A[k-1]|B[k-1]]) &r_NO_ERR;
						A[k] = (Next[B[k]] & CMask) | r1 | r2;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextcharfile: 
				c = buffer[i];
				CMask = RMask[c];
				if(c != Newline)
				{ 
					r1 = Init1 & A[0];
					B[0] = (Next[A[0]] & CMask) | r1;
					r1 = Init1 & A[1];
					B[1] = (Next[A[1]] & CMask) | ((A[0] | Next[A[0] | B[0]]) & r_NO_ERR) | r1 ;  
					if(D == 1) goto Nextchar1file;
					r1 = Init1 & A[2];
					B[2] = (Next[A[2]] & CMask) | ((A[1] | Next[A[1] | B[1]]) &r_NO_ERR) | r1 ;  
					if(D == 2) goto Nextchar1file;
					r1 = Init1 & A[3];
					B[3] = (Next[A[3]] & CMask) | ((A[2] | Next[A[2] | B[2]])&r_NO_ERR) | r1 ;  
					if(D == 3) goto Nextchar1file;
					r1 = Init1 & A[4];
					B[4] = (Next[A[4]] & CMask) | ((A[3] | Next[A[3] | B[3]])&r_NO_ERR) | r1 ;  
					goto Nextchar1file;
				} /* if(NOT Newline) */
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & A[D];               /* match endofline */
					B[D] = (Next[A[D]] & CMask) | r1;
					if(TAIL) B[D] = Next[B[D]] | B[D];
					if((B[D] & 1) ^ INVERSE )  {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<= D; k++) { 
						A[k] = B[k] = Init[k]; 
					}
					r1 = Init1 & A[0]; 
					B[0] = (Next[A[0]] & CMask) | r1;
					for(k=1; k<= D; k++) {
						r1 = Init1 & A[k];
						r2 = (A[k-1] | Next[A[k-1]|B[k-1]])&r_NO_ERR;
						B[k] = (Next[A[k]] & CMask) | r1 | r2;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextchar1file: 
				i++;
				CurrentByteOffset ++;
			} /* while i < end */

			strncpy(buffer, buffer+num_read, Maxline);
		} /* while  fill_buf() */
		free_buf(Text, buffer);
		return 0;
#if	AGREP_POINTER
	}
	else {
		num_read = agrep_inlen;
		buffer = (CHAR *)agrep_inbuffer;
		end = num_read;
		/* buffer[end-1] = '\n';*/ /* at end of the text. */
		/* buffer[0] = '\n';*/  /* in front of the  text. */
		i = 0;

			/* An exact copy of the above RE_PROCESS_WHEN_DNOTZERO: the while-loop below */
			while (i < end)
			{   
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = RMask[c];
				if (c != Newline)
				{  
					r_even = B[0];
					r1 = Init1 & r_even;
					A[0] = (Next[r_even] & CMask) | r1;
					r_odd = B[1];
					r1 = Init1 & r_odd;
					r2 = (r_even | Next[r_even|A[0]]) &r_NO_ERR;
					A[1] = (Next[r_odd] & CMask) | r2 | r1 ;  
					if(D == 1) goto Nextcharmem;
					r_even = B[2];
					r1 = Init1 & r_even;
					r2 = (r_odd | Next[r_odd|A[1]]) &r_NO_ERR;
					A[2] = (Next[r_even] & CMask) | r2 | r1 ;  
					if(D == 2) goto Nextcharmem;
					r_odd = B[3];
					r1 = Init1 & r_odd;
					r2 = (r_even | Next[r_even|A[2]]) &r_NO_ERR;
					A[3] = (Next[r_odd] & CMask) | r2 | r1 ;  
					if(D == 3) goto Nextcharmem;
					r_even = B[4];
					r1 = Init1 & r_even;
					r2 = (r_odd | Next[r_odd|A[3]]) &r_NO_ERR;
					A[4] = (Next[r_even] & CMask) | r2 | r1 ;  
					goto Nextcharmem;
				} /* if NOT Newline */
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & B[D];               /* match endofline */
					A[D] = (Next[B[D]] & CMask) | r1;
					if(TAIL) A[D] = Next[A[D]] | A[D];
					if((A[D] & 1) ^ INVERSE )  {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;    

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}  
						if (-1 == r_output(buffer, i-1, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<= D; k++) { 
						A[k] = B[k] = Init[k]; 
					}
					r1 = Init1 & B[0]; 
					A[0] = (Next[B[0]] & CMask) | r1;
					for(k=1; k<= D; k++) {
						r1 = Init1 & B[k];
						r2 = (B[k-1] | Next[A[k-1]|B[k-1]]) &r_NO_ERR;
						A[k] = (Next[B[k]] & CMask) | r1 | r2;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextcharmem: 
				c = buffer[i];
				CMask = RMask[c];
				if(c != Newline)
				{ 
					r1 = Init1 & A[0];
					B[0] = (Next[A[0]] & CMask) | r1;
					r1 = Init1 & A[1];
					B[1] = (Next[A[1]] & CMask) | ((A[0] | Next[A[0] | B[0]]) & r_NO_ERR) | r1 ;  
					if(D == 1) goto Nextchar1mem;
					r1 = Init1 & A[2];
					B[2] = (Next[A[2]] & CMask) | ((A[1] | Next[A[1] | B[1]]) &r_NO_ERR) | r1 ;  
					if(D == 2) goto Nextchar1mem;
					r1 = Init1 & A[3];
					B[3] = (Next[A[3]] & CMask) | ((A[2] | Next[A[2] | B[2]])&r_NO_ERR) | r1 ;  
					if(D == 3) goto Nextchar1mem;
					r1 = Init1 & A[4];
					B[4] = (Next[A[4]] & CMask) | ((A[3] | Next[A[3] | B[3]])&r_NO_ERR) | r1 ;  
					goto Nextchar1mem;
				} /* if(NOT Newline) */
				else {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = Init1 & A[D];               /* match endofline */
					B[D] = (Next[A[D]] & CMask) | r1;
					if(TAIL) B[D] = Next[B[D]] | B[D];
					if((B[D] & 1) ^ INVERSE )  {
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										free_buf(Text, buffer);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "\n");
							else {
								if (agrep_outpointer+1>=agrep_outlen) {
									OUTPUT_OVERFLOW;
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						} 
						if (-1 == r_output(buffer, i, end, j)) {free_buf(Text, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					for(k=0; k<= D; k++) { 
						A[k] = B[k] = Init[k]; 
					}
					r1 = Init1 & A[0]; 
					B[0] = (Next[A[0]] & CMask) | r1;
					for(k=1; k<= D; k++) {
						r1 = Init1 & A[k];
						r2 = (A[k-1] | Next[A[k-1]|B[k-1]])&r_NO_ERR;
						B[k] = (Next[A[k]] & CMask) | r1 | r2;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
	Nextchar1mem: 
				i++;
				CurrentByteOffset ++;
			} /* while i < end */

		return 0;
	}
#endif	/*AGREP_POINTER*/
} /* re */

int
r_output (buffer, i, end, j) 
int i, end, j; 
CHAR *buffer;
{
	int PRINTED = 0;
	int bp;
	if(i >= end) return 0;
	if ((j < 1) || (CurrentByteOffset < 0)) return 0;
	num_of_matched++;
	if(COUNT)  return 0;
	if (SILENT) return 0;
	if(FNAME && (NEW_FILE || !POST_FILTER)) {
		char	nextchar = (POST_FILTER == ON)?'\n':' ';
		char	*prevstring = (POST_FILTER == ON)?"\n":"";

		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%s%s", prevstring, CurrentFileName);
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
			if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			agrep_outpointer += outindex;
		}
		if (PRINTFILETIME) {
			char *s = aprint_file_time(CurrentFileTime);
			if (agrep_finalfp != NULL)
				fprintf(agrep_finalfp, "%s", s);
			else {
				int outindex;
				for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
						(s[outindex] != '\0'); outindex++) {
					agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
				}
				if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				agrep_outpointer += outindex;
			}
		}
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, ":%c", nextchar);
		else {
			if (agrep_outpointer+2>= agrep_outlen) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			else {
				agrep_outbuffer[agrep_outpointer++] = ':';
				agrep_outbuffer[agrep_outpointer++] = nextchar;
			}
		}

		NEW_FILE = OFF;
		PRINTED = 1;
	}
	bp = i-1;
	while ((buffer[bp] != '\n') && (bp > 0)) bp--;
	if(LINENUM) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%d: ", j-1); 
		else {
			char s[32];
			int  outindex;

			sprintf(s, "%d: ", j-1);
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

	if(buffer[bp] != '\n') bp = Maxline-1;
	bp++;

	if (PRINTOFFSET) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "@%d{%d} ", CurrentByteOffset - (i-bp), i-bp);
		else {
			char s[32];
			int outindex;
			sprintf(s, "@%d{%d} ", CurrentByteOffset - (i-bp), i-bp);
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
	if (agrep_finalfp != NULL)
		while(bp <= i) fputc(buffer[bp++], agrep_finalfp);
	else {
		if (i - bp + 1 + agrep_outpointer >= agrep_outlen) {
			OUTPUT_OVERFLOW;
			return -1;
		}
		while(bp <= i) agrep_outbuffer[agrep_outpointer ++] = buffer[bp++];
	}
	}
	else if (PRINTED) {
		if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
		else agrep_outbuffer[agrep_outpointer ++] = '\n';
		PRINTED = 0;
	}

	return 0;
}

/*
 * Processes the options specified in argc and argv, and fetches the pattern.
 * Also sets the set of filenames to be searched for internally. Returns: -1
 * if there is a serious error, 0 if there is no pattern or an error in getting
 * the file names, the length (> 0) of the pattern if there is no error. When a
 * 0 is returned, it means that at least the options were processed correctly.
 */
int
agrep_init(argc, argv, initialfd, pattern_len, pattern_buffer)
	int	argc;
	char	*argv[];
	int	initialfd;
	int	pattern_len;
	CHAR	*pattern_buffer;
{
	int i, j, seenlsq = 0;
	char c, *p;
	int filetype;
	char **original_argv = argv;
	char *home;
	int quitwhile;
	int NOOUTTAIL=OFF;

	initial_value();
	if (pattern_len < 1) {
		fprintf(stderr, "agrep_init: pattern length %d too small\n", pattern_len);
		errno = 3;
		return -1;
	}
	agrep_initialfd = initialfd;
	strncpy(Progname, argv[0], MAXNAME);
	if (argc < 2) return agrep_usage();
        printf("");     /* dummy statement which avoids program crash with
                           SYS3175 when piping the output of complex AGREP
                           results into a file.

                           This bug is regarded as COMPILER-UNSPECIFIC.

                           For sure, the problem SHOULD BE FIXED somewhere
                           else in AGREP, later.

                           [TG] 16.09.96
			   Thomas Gries  gries@epo.e-mail.com, gries@ibm.net

                        */




	Pattern[0] = '\0';

	while(--argc > 0 && (*++argv)[0] == '-') { /* argv is incremented automatically here */
		p = argv[0]+1;			     /* ptr to first character after '-' */
		c = *(argv[0]+1); 
		quitwhile = OFF;
		while(!quitwhile && (*p != '\0')) {
			c = *p;
			switch(c) {
			case 'z' :
				NOOUTPUTZERO = ON;	/* don't output files with 0 matches */
				PRINT(printf("z\n");
				)
					break;

			case 'c' : 
				COUNT = ON;    /* output the # of matches */
				PRINT(printf("c\n");
				)
					break;

			case 's' : 
				SILENT = ON;   /* silent mode  */
				PRINT(printf("s\n");
				)
					break;

			case 'p' : 
				I = 0;         /* insertion cost is 0 */
				PRINT(printf("p\n");
				)
					break; 
			case 'P' :
				PRINTPATTERN = 1;	/* print pattern before every matched line */
				PRINT(printf("p\n");
				)
					break;

			case 'x' : 
				WHOLELINE = ON;  /* match the whole line */
				PRINT(printf("x\n");
				)
					if(WORDBOUND) {
						fprintf(stderr, "%s: illegal option combination (-x and -w)\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
				break;

			case 'b' : 
				BYTECOUNT = ON;
				PRINT(printf("b\n");
				)
				break;

			case 'q' :
				PRINTOFFSET = ON;
				PRINT(printf("q\n");
				)
				break;

			case 'u' :
				PRINTRECORD = OFF;
				PRINT(printf("u\n");
				)
				break;

			case 'X' :
				PRINTNONEXISTENTFILE = ON;
				PRINT(printf("X\n");
				)
				break;

			case 'g' :
				PRINTFILENUMBER = ON;
				PRINT(printf("g\n");
				)
				break;

			case 'j' :
				PRINTFILETIME = ON;
				PRINT(printf("@\n");
				)
				break;

			case 'L' :
				if ( *(p + 1) == '\0') {/* space after -L option */
					if(argc <= 1) {
						fprintf(stderr, "%s: the -L option must have an output-limit argument\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					argv++;
					LIMITOUTPUT = LIMITTOTALFILE = LIMITPERFILE = 0;
					sscanf(argv[0], "%d:%d:%d", &LIMITOUTPUT, &LIMITTOTALFILE, &LIMITPERFILE);
					if ((LIMITOUTPUT < 0) || (LIMITTOTALFILE < 0) || (LIMITPERFILE < 0)) {
						fprintf(stderr, "%s: invalid output limit %s\n", Progname, argv[0]);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					argc--;
				} 
				else {
					LIMITOUTPUT = LIMITTOTALFILE = LIMITPERFILE = 0;
					sscanf(p+1, "%d:%d:%d", &LIMITOUTPUT, &LIMITTOTALFILE, &LIMITPERFILE);
					if ((LIMITOUTPUT < 0) || (LIMITTOTALFILE < 0) || (LIMITPERFILE < 0)) {
						fprintf(stderr, "%s: invalid output limit %s\n", Progname, p+1);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
				} /* else */
				PRINT(printf("L\n");
				)
				quitwhile = ON;
				break;

			case 'd' : 
				DELIMITER = ON;  /* user defines delimiter */
				PRINT(printf("d\n");
				)
				if ( *(p + 1) == '\0') {/* space after -d option */
					if(argc <= 1) {
						fprintf(stderr, "%s: the -d option must have a delimiter argument\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					argv++;
					if ((D_length = strlen(argv[0])) > MaxDelimit) {
						fprintf(stderr, "%s: delimiter pattern too long (has > %d chars)\n", Progname, MaxDelimit);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					D_pattern[0] = '<';
					strcpy(D_pattern+1, argv[0]);
					if (((argv[0][D_length-1] == '\n') || (argv[0][D_length-1] == '$') || (argv[0][D_length-1] == '^')) && (D_length == 1))
						OUTTAIL = ON;
					argc--;
					PRINT(printf("space\n");
					)
				} 
				else {
					if ((D_length = strlen(p + 1)) > MaxDelimit) {
						fprintf(stderr, "%s: delimiter pattern too long (has > %d chars)\n", Progname, MaxDelimit);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					D_pattern[0] = '<';
					strcpy(D_pattern+1, p + 1);
					if ((((p+1)[D_length-1] == '\n') || ((p+1)[D_length-1] == '$') || ((p+1)[D_length-1] == '^')) && (D_length == 1))
						OUTTAIL = ON;
				} /* else */
				strcat(D_pattern, ">; ");
				D_length++;   /* to count '<' as one */
				PRINT(printf("D_pattern=%s\n", D_pattern);
				)
				strcpy(original_D_pattern, D_pattern);
				original_D_length = D_length;
				quitwhile = ON;
				break;

			case 'H':
				if (*(p + 1) == '\0') {/* space after - option */
					if (argc <= 1) {
						fprintf(stderr, "%s: a directory name must follow the -H option\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return agrep_usage();
						}
						else exit(2);
					}
					argv ++;
					strcpy(COMP_DIR, argv[0]);
					argc --;
				}
				else {
					strcpy(COMP_DIR, p+1);
				}
				quitwhile = ON;
				break;

			case 'e' : 
				if ( *(p + 1) == '\0') {/* space after -e option */
					if(argc <= 1) {
						fprintf(stderr, "%s: the -e option must have a pattern argument\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					argv++;
					if(argv[0][0] == '-') {	/* not strictly necessary but no harm done */
						Pattern[0] = '\\';
						strcat(Pattern, (argv)[0]);
					}
					else strcat(Pattern, argv[0]);
					argc--;
				}
				else {
					if (*(p+1) == '-') {	/* not strictly necessary but no harm done */
						Pattern[0] = '\\';
						strcat(Pattern, p+1);
					}
					else strcat (Pattern, p+1);
				} /* else */

				PRINT(printf("Pattern=%s\n", Pattern);
				)
				pattern_index = abs(argv - original_argv);
				quitwhile = ON;
				break;

			case 'k' : 
				CONSTANT = ON;
				if ( *(p + 1) == '\0') {/* space after -e option */
					if(argc <= 1) {
						fprintf(stderr, "%s: the -k option must have a pattern argument\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					argv++;
					strcat(Pattern, argv[0]);
					if((argc > 2) && (argv[1][0] == '-')) {
						fprintf(stderr, "%s: -k should be the last option in the command\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					argc--;
				}
				else {
					if((argc > 1) && (argv[1][0] == '-')) {
						fprintf(stderr, "%s: -k should be the last option in the command\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					strcat (Pattern, p+1);
				} /* else */

				pattern_index = abs(argv - original_argv);
				quitwhile = ON;
				break;

			case 'f' : 
				if (PAT_FILE == ON) {
					fprintf(stderr, "%s: multiple -f options\n", Progname);
					if (multifd >= 0) close(multifd);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
				if (PAT_BUFFER == ON) {
					fprintf(stderr, "%s: -f and -m are incompatible\n", Progname);
					if (multibuf != NULL) free(multibuf);
					multibuf = NULL;
					multilen = 0;
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
				PAT_FILE = ON;
				PRINT(printf("f\n");
				)
				argv++;
				argc--;

				if (argv[0] == NULL) {
					/* A -f option with a NULL file name is a NO-OP: stupid, but simplifies glimpse :-) */
					PAT_FILE = OFF;
					quitwhile = ON;
					break;
				}

				if((multifd = open(argv[0], O_RDONLY)) < 0) {
					PAT_FILE = OFF;
					fprintf(stderr, "%s: can't open pattern file for reading: %s\n", Progname, argv[0]);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
				PRINT(printf("file=%s\n", argv[0]);
				)
				strcpy(PAT_FILE_NAME, argv[0]);
				if (prepf(multifd, NULL, 0) <= -1) {
					close(multifd);
					PAT_FILE = OFF;
					fprintf(stderr, "%s: error in processing pattern file: %s\n", Progname, argv[0]);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
				quitwhile = ON;
				break;

			case 'm' :
				if (PAT_BUFFER == ON) {
					fprintf(stderr, "%s: multiple -m options\n", Progname);
					if (multibuf != NULL) free(multibuf);
					multibuf = NULL;
					multilen = 0;
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
				if (PAT_FILE == ON) {
					fprintf(stderr, "%s: -f and -m are incompatible\n", Progname);
					if (multifd >= 0) close(multifd);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return -1;
					}
					else exit(2);
				}
				PAT_BUFFER = ON;
				PRINT(printf("m\n");
				)
				argv ++;
				argc --;


				if ((argv[0] == NULL) || ((multilen = strlen(argv[0])) <= 0)) {
					/* A -m option with a NULL or empty pattern buffer is a NO-OP: stupid, but simplifies glimpse :-) */
					PAT_BUFFER = OFF;
					if (multibuf != NULL) free(multibuf);
					multilen = 0;
					multibuf = NULL;
				}
				else {
					multibuf = (char *)malloc(multilen + 2);
					strcpy(multibuf, argv[0]);
					PRINT(printf("patterns=%s\n", multibuf);
					)
					if (prepf(-1, multibuf, multilen) <= -1) {
						free(multibuf);
						multibuf = NULL;
						multilen = 0;
						PAT_BUFFER = OFF;
						fprintf(stderr, "%s: error in processing pattern buffer\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
				}
				quitwhile = ON;
				break;

			case 'h' : 
				NOFILENAME = ON;
				PRINT(printf("h\n");
				)
					break;

			case 'i' : 
				NOUPPER = ON;
				PRINT(printf("i\n");
				)
					break;

			case 'l' : 
				FILENAMEONLY = ON;
				PRINT(printf("l\n");
				)
					break;

			case 'n' : 
				LINENUM = ON;  /* output prefixed by line no*/
				PRINT(printf("n\n");
				)
					break;

			case 'r' : 
				RECURSIVE = ON;
				PRINT(printf("r\n");
				)
					break;
			
			case 'V' :
				printf("\nThis is agrep version %s, %s.\n\n", AGREP_VERSION, AGREP_DATE);
				return 0;

			case 'v' : 
				INVERSE = ON;  /* output no-matched lines */
				PRINT(printf("v\n");
				)
					break;

			case 't' : 
				OUTTAIL = ON;  /* output from tail of delimiter */
				PRINT(printf("t\n");
				)
					break;

			case 'o' : 
				NOOUTTAIL = ON;  /* output from front of delimiter */
				PRINT(printf("t\n");
				)
					break;

			case 'B' : 
				BESTMATCH = ON;
				PRINT(printf("B\n");
				)
					break;

			case 'w' : 
				WORDBOUND = ON;/* match to words */
				PRINT(printf("w\n");
				)
					if(WHOLELINE) {
						fprintf(stderr, "%s: illegal option combination (-w and -x)\n", Progname);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
				break;

			case 'y' : 
				NOPROMPT = ON;
				PRINT(printf("y\n");
				)
					break;

			case 'I' : 
				I = atoi(p + 1);  /* Insertion Cost */
				JUMP = ON;
				quitwhile = ON;
				break;

			case 'S' : 
				S = atoi(p + 1);  /* Substitution Cost */
				JUMP = ON;
				quitwhile = ON;
				break;

			case 'D' : 
				DD = atoi(p + 1); /* Deletion Cost */
				JUMP = ON;
				quitwhile = ON;
				break;

			case 'G' : 
				FILEOUT = ON; 
				COUNT = ON;
				break;

			case 'A':
				ALWAYSFILENAME = ON;
				break;

			case 'O':
				POST_FILTER = ON;
				break;

			case 'M':
				MULTI_OUTPUT = ON;
				break;

			case 'Z': break;	/* no-op: used by glimpse */
			
			default  : 
				if (isdigit(c)) {
					APPROX = ON;
					D = atoi(p);
					if (D > MaxError) {
						fprintf(stderr,"%s: the maximum number of errors is %d\n", Progname, MaxError);
						if (!EXITONERROR) {
							errno = AGREP_ERROR;
							return -1;
						}
						else exit(2);
					}
					quitwhile = ON;	/* note that even a number should occur at the end of a group of options, as f & e */
				}
				else {
					fprintf(stderr, "%s: illegal option  -%c\n",Progname, c);
					return agrep_usage();
				}

			} /* switch(c) */
			p ++;
		}
	} /* while (--argc > 0 && (*++argv)[0] == '-') */

	if (NOOUTTAIL == ON) OUTTAIL = OFF;

	if (COMP_DIR[0] == '\0') {
		if ((home = (char *)getenv("HOME")) == NULL) {
			getcwd(COMP_DIR, MAX_LINE_LEN-1);
			fprintf(stderr, "using working-directory '%s' to locate dictionaries\n", COMP_DIR);
		}
		else strncpy(COMP_DIR, home, MAX_LINE_LEN);
	}

	strcpy(FREQ_FILE, COMP_DIR);
	strcat(FREQ_FILE, "/");
	strcat(FREQ_FILE, DEF_FREQ_FILE);
	strcpy(HASH_FILE, COMP_DIR);
	strcat(HASH_FILE, "/");
	strcat(HASH_FILE, DEF_HASH_FILE);
	strcpy(STRING_FILE, COMP_DIR);
	strcat(STRING_FILE, "/");
	strcat(STRING_FILE, DEF_STRING_FILE);
	initialize_common(FREQ_FILE, 0);	/* no error msgs */

	if (FILENAMEONLY && NOFILENAME) {
		fprintf(stderr, "%s: -h and -l options are mutually exclusive\n",Progname);
	}
	if (COUNT && (FILENAMEONLY || NOFILENAME)) {
		FILENAMEONLY = OFF; 
		if(!FILEOUT) NOFILENAME = OFF;
	}

	if (SILENT) {
		FILEOUT = 0;
		NOFILENAME = 1;
		PRINTRECORD = 0;
		FILENAMEONLY = 0;
		PRINTFILETIME = 0;
		BYTECOUNT = 0;
		PRINTOFFSET = 0;
	}

	if (!(PAT_FILE || PAT_BUFFER) && Pattern[0] == '\0') { /* Pattern not set with -e option */
		if (argc <= 0) {
			agrep_usage();
			return 0;
		}
		strcpy(Pattern, *argv); 
		pattern_index = abs(argv - original_argv);
		argc--;
		argv++;
	}
	/* if multi-pattern search, just ignore any specified pattern altogether: treat it as a filename */

	if (copied_from_argv) {
		for (i=0; i<Numfiles; i++) free(Textfiles[i]);
		if (Textfiles != NULL) free(Textfiles);
	}
	copied_from_argv = 0;
	Numfiles = 0; Textfiles = NULL;
	execfd = agrep_initialfd;
	if (argc <= 0)  /* check Pattern against stdin */
		execfd = 0;
	else {	/* filenames were specified as a part of the command line */
		if (!(Textfiles = (CHAR **)malloc(argc * sizeof(CHAR *) ))) {
			fprintf(stderr, "%s: malloc failure in %s:%d\n", Progname, __FILE__, __LINE__);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return 0;
			}
			else exit(2);
		}
		copied_from_argv = 1;	/* should I free Textfiles next time? */
		while (argc--)
		{	/* one or more filenames on command line -- put the valid filenames in a array of strings */

			if (!glimpse_call && ((filetype = check_file(*argv)) == NOSUCHFILE) && !PRINTNONEXISTENTFILE) {
				fprintf(stderr,"%s: '%s' no such file or directory\n",Progname,*argv);
				argv++;
			}
			else { /* file is ascii*/
				if (!(Textfiles[Numfiles] = (CHAR *)malloc((strlen(*argv)+2)))) {
					fprintf(stderr, "%s: malloc failure in %s:%d\n", Progname, __FILE__, __LINE__);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return 0;
					}
					else exit(2);
				}
				strcpy(Textfiles[Numfiles++], *argv++);
			} /* else */
		} /* while (argc--) */
		if (Numfiles <= 0) return 0;
		/* If user specified wrong filenames, then we quit rather than taking input from stdin! */
	} /* else */

	M = strlen(Pattern);
	if (M<=0) return 0;
	for (i=0; i<M; i++) {
		if (( ((unsigned char *)Pattern)[i] > USERRANGE_MIN) && ( ((unsigned char *)Pattern)[i] <= USERRANGE_MAX)) {
			fprintf(stderr, "Warning: pattern has some meta-characters interpreted by agrep!\n");
			break;
		}
		else if (Pattern[i] == '\\') i++;	/* extra */
		else if (Pattern[i] == '[') seenlsq = 1;
		else if ((Pattern[i] == '-') && !seenlsq) {
			for (j=M; j>=i; j--)
				Pattern[j+1] = Pattern[j];	/* right shift including '\0' */
			Pattern[i] = '\\';	/* escape the - */
			M ++;
			i++;
		}
		else if (Pattern[i] == ']') seenlsq = 0;
	}

	if (M > pattern_len - 1) {
		fprintf(stderr, "%s: pattern '%s' does not fit in specified buffer\n", Progname, Pattern);
		errno = 3;
		return 0;
	}
	if (pattern_buffer != Pattern)	/* not from mem/file-agrep() */
		strncpy(pattern_buffer, Pattern, M+1); /* copy \0 */
	return M;
}

/*
 * User need not bother about initialfd.
 * Both functions return -1 on error, 0 if there was no pattern,
 * length (>=1) of pattern otherwise.
 */

int
memagrep_init(argc, argv, pattern_len, pattern_buffer)
	int	argc;
	char	*argv[];
	int	pattern_len;
	char	*pattern_buffer;
{
	return (agrep_init(argc, argv, -1, pattern_len, pattern_buffer));
}

int
fileagrep_init(argc, argv, pattern_len, pattern_buffer)
	int	argc;
	char	*argv[];
	int	pattern_len;
	char	*pattern_buffer;
{
	return (agrep_init(argc, argv, 3, pattern_len, pattern_buffer));
}

/* returns -1 on error, num of matches (>=0) otherwise */
int
agrep_search(pattern_len, pattern_buffer, initialfd, input_len, input, output_len, output)
int pattern_len; 
CHAR *pattern_buffer;
int initialfd;
int input_len;
void *input;
int output_len;
void *output;
{
	int	i;
	int	filetype;
	int	ret;
	int	pattern_has_changed = 1;

	if ((multifd == -1) && (multibuf == NULL) && (pattern_len < 1)) {
		fprintf(stderr, "%s: pattern length %d too small\n", Progname, pattern_len);
		errno = 3;
		return -1;
	}
	if (pattern_len >= MAXPAT) {
		fprintf(stderr, "%s: pattern '%s' too long\n", Progname, pattern_buffer);
		errno = 3;
		return -1;
	}

	/* courtesy: crd@hplb.hpl.hp.com */
	if (agrep_saved_pattern) {
		if (strcmp(agrep_saved_pattern, pattern_buffer)) {
			free(agrep_saved_pattern);
			agrep_saved_pattern = NULL;
		}
		else {
			pattern_has_changed = 0;
		}
	}
	if (! agrep_saved_pattern) {
		agrep_saved_pattern = (CHAR *)malloc(pattern_len+1);
		memcpy(agrep_saved_pattern, pattern_buffer, pattern_len);
		agrep_saved_pattern[pattern_len] = '\0';

	}
	if (!pattern_has_changed) {
		reinit_value_partial();
	}
	else {
		reinit_value();
		if (pattern_buffer != Pattern)	/* not from mem/file-agrep() */
			strncpy(Pattern, pattern_buffer, pattern_len+1); /* copy \0 */
		M = strlen(Pattern);
	}

	if (output == NULL) {
		fprintf(stderr, "%s: invalid output descriptor\n", Progname);
		return -1;
	}
	if (output_len <= 0) {
		agrep_finalfp = (FILE *)output;
		agrep_outlen = 0;
		agrep_outbuffer = NULL;
		agrep_outpointer = 0;
	}
	else {
		agrep_finalfp = NULL;
		agrep_outlen = output_len;
		agrep_outbuffer = (CHAR *)output;
		agrep_outpointer = 0;
	}

	agrep_initialfd = initialfd;
	execfd = initialfd;
	if (initialfd == -1) {
		agrep_inbuffer = (CHAR *)input;
		agrep_inlen = input_len;
		agrep_inpointer = 0;
	}
	else if ((input_len > 0) && (input != NULL)) {
		/* Copy the set of filenames into Textfiles */
		if (copied_from_argv) {
			for (i=0; i<Numfiles; i++) free(Textfiles[i]);
			if (Textfiles != NULL) free(Textfiles);
		}
		copied_from_argv = 0;
		Numfiles = 0; Textfiles = NULL;
#if	0
		if (!(Textfiles = (CHAR **)malloc(input_len * sizeof(CHAR *) ))) {
			fprintf(stderr, "%s: malloc failure in %s:%d\n", Progname, __FILE__, __LINE__);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return 0;
			}
			else exit(2);
		}
#else	/*0*/
		Textfiles = (CHAR **)input;
#endif	/*0*/

		while (input_len --)
		{	/* one or more filenames on command line -- put the valid filenames in a array of strings */

			if (!glimpse_call && ((filetype = check_file(*(char **)input)) == NOSUCHFILE) && !PRINTNONEXISTENTFILE) {
				fprintf(stderr,"%s: '%s' no such file or directory\n",Progname,*(char **)input);
				input = (void *)(((long)input) + sizeof(char *));
			} 
			else { /* file is ascii*/
#if	0
				if (!(Textfiles[Numfiles] = (CHAR *)malloc((strlen(*(char **)input)+2)))) {
					fprintf(stderr, "%s: malloc failure in %s:%d\n", Progname, __FILE__, __LINE__);
					if (!EXITONERROR) {
						errno = AGREP_ERROR;
						return 0;
					}
					else exit(2);
				}
				strcpy(Textfiles[Numfiles++], *((char **)input));
#else	/*0*/
				Textfiles[Numfiles++] = *((CHAR **)input);
#endif	/*0*/
				input = (void *)(((long)input) + sizeof(char *));
			} /* else */
		} /* while (argc--) */
		if (Numfiles <= 0) return 0;
		/* If user specified wrong filenames, then we quit rather than taking input from stdin! */
	}
	else if (Numfiles <= 0) execfd = 0;
	/* the old set of files (agrep_init) are used */

#if	0
	printf("agrep_search: pat=%s buf=%s\n", Pattern, ((agrep_initialfd == -1) && (agrep_inlen < 20)) ? agrep_inbuffer : NULL);
#endif	/*0*/

	if (pattern_has_changed) {
		if (-1 == checksg(Pattern, D, 1)) return -1;       /* check if the pattern is simple */
		strcpy(OldPattern, Pattern);

		if (SGREP == 0) {	/* complex pattern search */
			if (-1 == preprocess(D_pattern, Pattern)) return -1;
			strcpy(old_D_pat, D_pattern);
			/* fprintf(stderr, "agrep_search: len=%d pat=%s\n", strlen(Pattern), Pattern); */
			if(!AParse &&  ((M = maskgen(Pattern, D)) == -1)) return -1;
		}
		else {	/* sgrep too can handle delimiters */
			if (DELIMITER) {
				/* D_pattern is "<PAT>; ", D_length is 1 + length of string PAT: see agrep.c/'d' */
				preprocess_delimiter(D_pattern+1, D_length - 1, D_pattern, &D_length);
				/* D_pattern is the exact stuff we want to match, D_length is its strlen */
				if ((tc_D_length = quick_tcompress(FREQ_FILE,HASH_FILE,D_pattern,D_length,tc_D_pattern,MaxDelimit*2,TC_EASYSEARCH)) <= 0) {
					strcpy(tc_D_pattern, D_pattern);
					tc_D_length = D_length;
				}
				/* printf("sgrep's delim=%s,%d tc_delim=%s,%d\n", D_pattern, D_length, tc_D_pattern, tc_D_length); */
			}
			M = strlen(OldPattern);
		}
	}

	if (AParse)  {	/* boolean converted to multi-pattern search */
		int prepf_ret= 0;
		if (pattern_has_changed)
			prepf_ret= prepf(-1, multibuf, multilen);
		if (prepf_ret  <= -1) {
			if (AComplexBoolean) destroy_tree(AParse);
			AParse = 0;
			PAT_BUFFER = 0;
			if (multibuf != NULL) free(multibuf);	/* this was allocated for arbit booleans, not multipattern search */
			multibuf = NULL;
			multilen = 0;
			/* Cannot free multifd here since that is always allocated for multipattern search */
			return -1;
		}
	}
	if (Numfiles > 1) FNAME = ON;
	if (NOFILENAME) FNAME = 0;
	if (ALWAYSFILENAME) FNAME = ON;	/* used by glimpse ONLY: 15/dec/93 */

	if (agrep_initialfd == -1) ret = exec(execfd, NULL);
	else if(RECURSIVE) ret = (recursive(Numfiles, Textfiles));
	else  ret = (exec(execfd, Textfiles));
	return ret;
}

/*
 * User need not bother about initialfd.
 * Both functions return -1 on error, 0 otherwise.
 */

int
memagrep_search(pattern_len, pattern_buffer, input_len, input_buffer, output_len, output)
int pattern_len;
char *pattern_buffer;
int input_len;
char *input_buffer;
int output_len;
void *output;
{
	return(agrep_search(pattern_len, pattern_buffer, -1, input_len, input_buffer, output_len, output));
}

int
fileagrep_search(pattern_len, pattern_buffer, file_num, file_buffer, output_len, output)
int pattern_len;
char *pattern_buffer;
int file_num;
char **file_buffer;
int output_len;
void *output;
{
	return(agrep_search(pattern_len, pattern_buffer, 3, file_num, file_buffer, output_len, output));
}

/*
 * The original agrep_run() routine was split into agrep_search and agrep_init
 * so that the interface with glimpse could be made cleaner: see glimpse.
 * Now, the user can specify an initial set of options, and use them in future
 * searches. If agrep_init does not find the pattern, options are still SET.
 * In fileagrep_search, the user can specify a NEW set of files to be searched
 * after the options are processed (this is used in glimpse).
 *
 * Both functions return -1 on error, 0 otherwise.
 *
 * The arguments are self explanatory. The pattern should be specified in
 * one of the argvs. Options too can be specified in one of the argvs -- it
 * is exactly as if the options are being given to agrep at run time.
 * The only restrictions are that the input_buffer should begin with a '\n'
 * and after its end, there must be valid memory to store a copy of the pattern.
 */

int
memagrep(argc, argv, input_len, input_buffer, output_len, output)
int argc; 
char *argv[];
int input_len;
char *input_buffer;
int output_len;
void *output;
{
	int	ret;

	if ((ret = memagrep_init(argc, argv, MAXPAT, Pattern)) < 0) return -1;
	else if ((ret == 0) && (multifd == -1) && (multibuf == NULL)) return -1;
	/* ^^^ because one need not specify the pattern on the cmd line if -f OR -m */
	return memagrep_search(ret, Pattern, input_len, input_buffer, output_len, output);
}

int
fileagrep(argc, argv, output_len, output)
int argc; 
char *argv[];
int output_len;
void *output;
{
	int	ret;

	if ((ret = fileagrep_init(argc, argv, MAXPAT, Pattern)) < 0) return -1;
	else if ((ret == 0) && (multifd == -1) && (multibuf == NULL)) return -1;
	/* ^^^ because one need not specify the pattern on the cmd line if -f OR -m */
	return fileagrep_search(ret, Pattern, 0, NULL, output_len, output);
}

/*
 * RETURNS: total number of matched lines in all files that were searched.
 *
 * The pattern(s) remain(s) constant irrespective of the number of files.
 * Hence, essentially, all the interface routines below have to be changed
 * so that they DONT do that preprocessing again and again for multiple
 * files. This bug was found while interfacing agrep with cast.
 *
 * At present, sgrep() has been modified to have another parameter,
 * "samepattern" that tells it whether the pattern is the same as before.
 * Other funtions too should have such a parameter and should not repeat
 * preprocessing for all patterns. Since preprocessing for a pattern to
 * be searched in compressed files is siginificant, this bug was found.
 *
 * - bgopal on 15/Nov/93.
 */
int
exec(fd, file_list)
int fd;
char **file_list;
{ 
	int i;
	char c[8];
	int ret = 0;	/* no error */

	if ((Numfiles > 1) && (NOFILENAME == OFF)) FNAME = ON;
	if ((-1 == compat())) return -1; /* check compatibility between options */

	if (fd <= 0) {
		TCOMPRESSED = ON;	/* there is a possibility that the data might be tuncompressible */
		if (!SetCurrentByteOffset) CurrentByteOffset = 0;
		if((fd == 0) && FILENAMEONLY) {
			fprintf(stderr, "%s: -l option is not compatible with standard input\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);  
		}
		if(PAT_FILE || PAT_BUFFER) mgrep(fd, AParse);
		else {
			if(SGREP) ret = sgrep(OldPattern, strlen(OldPattern), fd, D, 0);
			else      ret = bitap(old_D_pat, Pattern, fd, M, D);
		}
		if (ret <= -1) return -1;
		if (COUNT /* && ret */) {	/* dirty solution for glimpse's -b! */
			if(INVERSE && (PAT_FILE || PAT_BUFFER)) {	/* inverse will never be set in glimpse */
				if (agrep_finalfp != NULL)
					fprintf(agrep_finalfp, "%d\n", total_line-(num_of_matched - prev_num_of_matched));
				else {
					char s[32];
					int  outindex;

					sprintf(s, "%d\n", total_line-(num_of_matched - prev_num_of_matched));

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
			}
			else {
				if (agrep_finalfp != NULL)
					fprintf(agrep_finalfp, "%d\n", (num_of_matched - prev_num_of_matched));
				else {
					char s[32];
					int  outindex;

					sprintf(s, "%d\n", (num_of_matched - prev_num_of_matched));

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
			}
		}
	}
	else {
		/* fd > 0 => Numfiles > 0 */

		for (i = 0; i < Numfiles; i++, close(fd)) {
			prev_num_of_matched = num_of_matched;
			if (!SetCurrentByteOffset) CurrentByteOffset = 0;
			if (!SetCurrentFileName) {
				if (PRINTFILENUMBER) sprintf(CurrentFileName, "%d", i);
				else strcpy(CurrentFileName, file_list[i]);
			}
			if (!SetCurrentFileTime) {
				if (PRINTFILETIME) CurrentFileTime = aget_file_time(NULL, file_list[i]);
			}
			TCOMPRESSED = ON;
			if (!tuncompressible_filename(file_list[i], strlen(file_list[i]))) TCOMPRESSED = OFF;
			NEW_FILE = ON;
			if ((fd = my_open(file_list[i], O_RDONLY)) < /*=*/ 0) {
				if (PRINTNONEXISTENTFILE) printf("%s\n", CurrentFileName);
				else if (!glimpse_call) fprintf(stderr, "%s: can't open file for reading: %s\n",Progname, file_list[i]);
			} 
			else { 
				if(PAT_FILE || PAT_BUFFER) mgrep(fd, AParse);
				else {
					if(SGREP) ret = sgrep(OldPattern, strlen(OldPattern), fd, D, i);
					else      ret = bitap(old_D_pat, Pattern, fd, M, D);
				}
				if (ret <= -1) {
					close(fd);
					return -1;
				}
				if (num_of_matched - prev_num_of_matched > 0) {
					NOMATCH = OFF;
					files_matched ++;
				}

				if (COUNT && !FILEOUT) {
					if( (INVERSE && (PAT_FILE || PAT_BUFFER)) && ((total_line - (num_of_matched - prev_num_of_matched)> 0) || !NOOUTPUTZERO) ) {
						if(FNAME && (NEW_FILE || !POST_FILTER)) {

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									close(fd);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										close(fd);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, ": %d\n", total_line - (num_of_matched - prev_num_of_matched));
							else {
								char s[32];
								int  outindex;
								sprintf(s, ": %d\n", total_line - (num_of_matched - prev_num_of_matched));
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(s[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									close(fd);
									return -1;
								}
								agrep_outpointer += outindex;
							}

							NEW_FILE = OFF;
						}
						else if (!FNAME) {
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%d\n", total_line - (num_of_matched - prev_num_of_matched));
							else {
								char s[32];
								int  outindex;

								sprintf(s, "%d\n", total_line - (num_of_matched - prev_num_of_matched));

								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(s[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									close(fd);
									return -1;
								}
								agrep_outpointer += outindex;
							}
						}
					}
					else if ( !(INVERSE && (PAT_FILE || PAT_BUFFER)) && (((num_of_matched - prev_num_of_matched) > 0) || !NOOUTPUTZERO) ) {
						/* inverse is always 0 in glimpse, so we always come here */
						if(FNAME && (NEW_FILE || !POST_FILTER)) {

							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%s", CurrentFileName);
							else {
								int outindex;
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(CurrentFileName[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = CurrentFileName[outindex];
								}
								if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
									OUTPUT_OVERFLOW;
									close(fd);
									return -1;
								}
								agrep_outpointer += outindex;
							}
							if (PRINTFILETIME) {
								char *s = aprint_file_time(CurrentFileTime);
								if (agrep_finalfp != NULL)
									fprintf(agrep_finalfp, "%s", s);
								else {
									int outindex;
									for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
											(s[outindex] != '\0'); outindex++) {
										agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
									}
									if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
										OUTPUT_OVERFLOW;
										close(fd);
										return -1;
									}
									agrep_outpointer += outindex;
								}
							}
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, ": %d\n", (num_of_matched - prev_num_of_matched));
							else {
								char s[32];
								int  outindex;
								sprintf(s, ": %d\n", (num_of_matched - prev_num_of_matched));
								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(s[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									close(fd);
									return -1;
								}
								agrep_outpointer += outindex;
							}

							NEW_FILE = OFF;
						}
						else if (!FNAME) {
							if (agrep_finalfp != NULL)
								fprintf(agrep_finalfp, "%d\n", (num_of_matched - prev_num_of_matched));
							else {
								char s[32];
								int  outindex;

								sprintf(s, "%d\n", (num_of_matched - prev_num_of_matched));

								for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
										(s[outindex] != '\0'); outindex++) {
									agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
								}
								if (s[outindex] != '\0') {
									OUTPUT_OVERFLOW;
									close(fd);
									return -1;
								}
								agrep_outpointer += outindex;
							}
						}
					}
				}  /* if COUNT */
				if(FILEOUT && (num_of_matched - prev_num_of_matched)) {
					if (-1 == file_out(file_list[i])) {
						close(fd);
						return -1;
					}
				}
			} /* else */
			if (glimpse_clientdied) {
				close(fd);
				return -1;
			}
			if (agrep_finalfp != NULL) fflush(agrep_finalfp);
			if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
			    ((LIMITTOTALFILE > 0) && (LIMITTOTALFILE <= files_matched))) {
				close(fd);
				break;	/* done */
			}
		}  /* for i < Numfiles */

		if(NOMATCH && BESTMATCH) {
			if(WORDBOUND || WHOLELINE || INVERSE) { 
				SGREP = 0;	
				if(-1 == preprocess(D_pattern, Pattern)) return -1;
				strcpy(old_D_pat, D_pattern);
				if((M = maskgen(Pattern, D)) == -1) return -1;
			}
			COUNT=ON; 
			D=1;
			while(D<M && D<=MaxError && (num_of_matched - prev_num_of_matched == 0)) {
				for (i = 0; i < Numfiles; i++, close(fd)) {
					prev_num_of_matched = num_of_matched;
					CurrentByteOffset = 0;
					if (PRINTFILENUMBER) sprintf(CurrentFileName, "%d", i);
					else strcpy(CurrentFileName, file_list[i]);
					if (!SetCurrentFileTime) if (PRINTFILETIME) CurrentFileTime = aget_file_time(NULL, file_list[i]);
					NEW_FILE = ON;
					if ((fd = my_open(Textfiles[i], O_RDONLY)) > 0) {
						if(PAT_FILE || PAT_BUFFER) mgrep(fd, AParse);
						else {
							if(SGREP) ret = sgrep(OldPattern,strlen(OldPattern),fd,D, i);
							else ret = bitap(old_D_pat,Pattern,fd,M,D);
						}
						if (ret <= -1) return -1;
					}
					/* else don't have to process PRINTNONEXISTENTFILE since must print only once */
					if (glimpse_clientdied) {
						close(fd);
						return -1;
					}
					if (agrep_finalfp != NULL) fflush(agrep_finalfp);
					if ((((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
					     ((LIMITTOTALFILE > 0) && (LIMITTOTALFILE <= files_matched))) &&
					    (num_of_matched > prev_num_of_matched)) {
						close(fd);
						break;
					}
				}  /* for i < Numfiles */
				D++;
			} /* while */

			if(num_of_matched - prev_num_of_matched > 0) {
				D--; 
				errno = D;	/* #of errors if proper return */
				COUNT = 0;

				if(num_of_matched - prev_num_of_matched == 1) fprintf(stderr,"%s: 1 word matches ", Progname);
				else fprintf(stderr,"%s: %d words match ", Progname, num_of_matched - prev_num_of_matched);
				if(D==1) fprintf(stderr, "within 1 error");
				else fprintf(stderr, "within %d errors", D);

				fflush(stderr);

				if(NOPROMPT) fprintf(stderr, "\n");
				else {
					if(num_of_matched - prev_num_of_matched == 1) fprintf(stderr,"; search for it? (y/n)");
					else fprintf(stderr,"; search for them? (y/n)");
					c[0] = 'y';
					if (!glimpse_isserver && (fgets(c, 4, stdin) == NULL)) goto CONT;
					if(c[0] != 'y') goto CONT;
				}

				for (i = 0; i < Numfiles; i++, close(fd)) {
					prev_num_of_matched = num_of_matched;
					CurrentByteOffset = 0;
					if (PRINTFILENUMBER) sprintf(CurrentFileName, "%d", i);
					else strcpy(CurrentFileName, file_list[i]);
					if (!SetCurrentFileTime) if (PRINTFILETIME) CurrentFileTime = aget_file_time(NULL, file_list[i]);
					NEW_FILE = ON;
					if ((fd = my_open(Textfiles[i], O_RDONLY)) > 0) {
						if(PAT_FILE || PAT_BUFFER) mgrep(fd, AParse);
						else {
							if(SGREP) ret = sgrep(OldPattern,strlen(OldPattern),fd,D, i);
							else ret = bitap(old_D_pat,Pattern,fd,M,D);
						}
						if (ret <= -1) {
							close(fd);
							return -1;
						}
					}
					/* else don't have to process PRINTNONEXISTENTFILE since must print only once */
					if (glimpse_clientdied) {
						close(fd);
						return -1;
					}
					if (agrep_finalfp != NULL) fflush(agrep_finalfp);
					if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
					    ((LIMITTOTALFILE > 0) && (LIMITTOTALFILE <= files_matched))) {
						close(fd);
						break;	/* done */
					}
				}  /* for i < Numfiles */
				NOMATCH = 0;
			}
		}
	}
CONT:
	if(EATFIRST) {
		if (agrep_finalfp != NULL) fprintf(agrep_finalfp, "\n");
		else if (agrep_outpointer >= agrep_outlen) {
			OUTPUT_OVERFLOW;
			return -1;
		}
		else agrep_outbuffer[agrep_outpointer++] = '\n';
		EATFIRST = OFF;
	}
	if(num_of_matched - prev_num_of_matched > 0) NOMATCH = OFF;
	/* if(NOMATCH) return(0); */
	/*printf("exec=%d\n", num_of_matched);*/
	return(num_of_matched);

} /* end of exec() */


/* Just output the contents of the file fname onto the std output */
int
file_out(fname)
char *fname;
{
	int num_read;
	int fd;
	int i, len;
	CHAR buf[SIZE+2];
	if(FNAME) {
		len = strlen(fname);
		if (agrep_finalfp != NULL) {
			fputc('\n', agrep_finalfp);
			for(i=0; i< len; i++) fputc(':', agrep_finalfp);
			fputc('\n', agrep_finalfp);
			fprintf(agrep_finalfp, "%s\n", fname);
			for(i=0; i< len; i++) fputc(':', agrep_finalfp);
			fputc('\n', agrep_finalfp);
			fflush(agrep_finalfp);
		}
		else {
			if (1+len+1+len+1+len+1+agrep_outpointer >= agrep_outlen) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			agrep_outbuffer[agrep_outpointer++] = '\n';
			for (i=0; i<len; i++) agrep_outbuffer[agrep_outpointer++] = ':';
			agrep_outbuffer[agrep_outpointer++] = '\n';
			for (i=0; i<len; i++) agrep_outbuffer[agrep_outpointer++] = fname[i];
			agrep_outbuffer[agrep_outpointer++] = '\n';
			for (i=0; i<len; i++) agrep_outbuffer[agrep_outpointer++] = ':';
			agrep_outbuffer[agrep_outpointer++] = '\n';
		}
	}
	fd = my_open(fname, O_RDONLY);
	if (agrep_finalfp != NULL) {
		while((num_read = fill_buf(fd, buf, SIZE)) > 0) 
			write(1, buf, num_read);
		if (glimpse_clientdied) {
			close(fd);
			return -1;
		}
	}
	else {
		if ((num_read = fill_buf(fd, agrep_outbuffer + agrep_outpointer, agrep_outlen - agrep_outpointer)) > 0)
			agrep_outpointer += num_read;
	}
	close(fd);
	return 0;
}

int
output(buffer, i1, i2, j)  
register CHAR *buffer; 
int i1, i2, j;
{
	int PRINTED = 0;
	register CHAR *bp, *outend;
	if(i1 > i2) return 0;
	num_of_matched++;
	if(COUNT)  return 0;
	if(SILENT) return 0;
	if(OUTTAIL || (!DELIMITER && (D_length == 1) && (D_pattern[0] == '\n')) ) {
		if (j>1) i1 = i1 + D_length;
		i2 = i2 + D_length;
	}
	if(DELIMITER) j = j+1;
	if(FIRSTOUTPUT) {
		if (buffer[i1] == '\n')  {
			i1++;
			EATFIRST = ON;
		}
		FIRSTOUTPUT = 0;
	}
	if(TRUNCATE) {
		fprintf(stderr, "WARNING!  some lines have been truncated in output record #%d\n", num_of_matched-1);
	}

	/* Why do we have to do this? */
	while ((buffer[i1] == '\n') && (i1 <= i2)) {
		if (agrep_finalfp != NULL) fprintf(agrep_finalfp, "\n");
		else {
			if (agrep_outpointer < agrep_outlen)
				agrep_outbuffer[agrep_outpointer ++] = '\n';
			else {
				OUTPUT_OVERFLOW;
				return -1;
			}
		}
		i1++;
	}

	if(FNAME && (NEW_FILE || !POST_FILTER)) {
		char	nextchar = (POST_FILTER == ON)?'\n':' ';
		char	*prevstring = (POST_FILTER == ON)?"\n":"";

		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%s%s", prevstring, CurrentFileName);
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
			if ((CurrentFileName[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			agrep_outpointer += outindex;
		}
		if (PRINTFILETIME) {
			char *s = aprint_file_time(CurrentFileTime);
			if (agrep_finalfp != NULL)
				fprintf(agrep_finalfp, "%s", s);
			else {
				int outindex;
				for(outindex=0; (outindex+agrep_outpointer<agrep_outlen) &&
						(s[outindex] != '\0'); outindex++) {
					agrep_outbuffer[agrep_outpointer+outindex] = s[outindex];
				}
				if ((s[outindex] != '\0') || (outindex+agrep_outpointer>=agrep_outlen)) {
					OUTPUT_OVERFLOW;
					return -1;
				}
				agrep_outpointer += outindex;
			}
		}
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, ":%c", nextchar);
		else {
			if (agrep_outpointer+2>= agrep_outlen) {
				OUTPUT_OVERFLOW;
				return -1;
			}
			else {
				agrep_outbuffer[agrep_outpointer++] = ':';
				agrep_outbuffer[agrep_outpointer++] = nextchar;
			}
		}

		NEW_FILE = OFF;
		PRINTED = 1;
	}
	if(LINENUM) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%d: ", j-1); 
		else {
			char s[32];
			int  outindex;

			sprintf(s, "%d: ", j-1);
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
	if(BYTECOUNT) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "%d= ", CurrentByteOffset-1);
		else {
			char s[32];
			int  outindex;
			sprintf(s, "%d= ", CurrentByteOffset-1);
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

	bp = buffer + i1;
	outend = buffer + i2;

	if (PRINTOFFSET) {
		if (agrep_finalfp != NULL)
			fprintf(agrep_finalfp, "@%d{%d}\n", CurrentByteOffset - (i2-i1), i2-i1);
		else {
			char s[32];
			int outindex;
			sprintf(s, "@%d{%d}\n", CurrentByteOffset - (i2-i1), i2-i1);
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
	if (agrep_finalfp != NULL)
		while(bp <= outend) fputc(*bp++, agrep_finalfp);
	else {
		if (outend - bp + 1 + agrep_outpointer >= agrep_outlen) {
			OUTPUT_OVERFLOW;
			return -1;
		}
		while(bp <= outend) agrep_outbuffer[agrep_outpointer ++] = *bp++;
	}
	}
	else if (PRINTED) {
		if (agrep_finalfp != NULL) fputc('\n', agrep_finalfp);
		else agrep_outbuffer[agrep_outpointer ++] = '\n';
		PRINTED = 0;
	}
	return 0;
}

int
agrep_usage()
{
	if (glimpse_call) return -1;
	fprintf(stderr, "usage: %s [-@#abcdehiklnoprstvwxyBDGIMSV] [-f patternfile] [-H dir] pattern [files]\n", Progname); 
	fprintf(stderr, "\n");	
	fprintf(stderr, "summary of frequently used options:\n");
	fprintf(stderr, "(For a more detailed listing see 'man agrep'.)\n");
	fprintf(stderr, "-#: find matches with at most # errors\n");
	fprintf(stderr, "-c: output the number of matched records\n");
	fprintf(stderr, "-d: define record delimiter\n");
	fprintf(stderr, "-h: do not output file names\n");
	fprintf(stderr, "-i: case-insensitive search, e.g., 'a' = 'A'\n");
	fprintf(stderr, "-l: output the names of files that contain a match\n");
	fprintf(stderr, "-n: output record prefixed by record number\n");
	fprintf(stderr, "-v: output those records that have no matches\n");
	fprintf(stderr, "-w: pattern has to match as a word, e.g., 'win' will not match 'wind'\n");
	fprintf(stderr, "-B: best match mode. find the closest matches to the pattern\n"); 
	fprintf(stderr, "-G: output the files that contain a match\n");
	fprintf(stderr, "-H 'dir': the cast-dictionary is located in directory 'dir'\n");
	fprintf(stderr, "\n");	

	if (!EXITONERROR) {
		errno = AGREP_ERROR;
		return -1;
	}
	else exit(2);
}
