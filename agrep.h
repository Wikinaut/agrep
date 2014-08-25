/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/*	see also agrephlp.c and agrep.c for a more precise history of changes.

	3.11	[new] option -i0				[TG] 23.09.96
	3.10	[chg] in AGREP.H, ISO_CHAR.H, AGREP.C		[TG] 22.09.96
		handling of meta symbols
	3.09	[fix] in BITAP.C (type CHAR)			[TG] 22.09.96
		[new] in AGREP.C (Grand Total)
	3.08 	un-commenting code defined in OUTPUT_OVERFLOW	[TG] 16.09.96
*/

#ifndef _AGREP_H_
#define _AGREP_H_
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "re.h"
#include "defs.h"
#include "config.h"
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <string.h>

#define	AGREP_ENV_OPTS	"AGREPOPTS"	/* name of the environment variable
					   holding default options for AGREP */

#define MAXNUM_PAT  16	/* 32 parts of a pattern = width of expression-tree */
#define CHAR	unsigned char
#define MAXPAT	256
#define MAXPATT 256
#define MAXDELIM 8	/* Max size of a delimiter pattern */
#define SHORTREG 15
#define MAXREG   30
#define MAXNAME  256
#define Max_Pats 12	/* max num of patterns */
#define Max_Keys 12     /* max num of keywords */
#define Max_Psize 128   /* max size of a pattern counting all the characters */
#define Max_Keyword 31  /* the max size of a keyword */
#define WORD 32         /* the size of a word */
#define MaxError 8      /* the max number of errors allowed */
#define MaxRerror 4     /* the max number of erros for regular expression */
#define MaxDelimit 16   /* the max raw length of a user defined delimiter */

#define BlockSize  49152
#define Max_record 49152

#define SIZE 16384		/* BlockSize in sgrep */
#define MAXLINE   1024		/* maxline in sgrep */
#define MAX_LINE_LEN 1024
#define Maxline   1024
#define RBLOCK    8192
#define RMAXLINE  1024
#define MaxNext   66000
#define ON 1
#define OFF 0
#define Compl 1
#define Maxresult 10000
#define MaxCan 2500

#if ( ! (defined(__EMX__) && defined(ISO_CHAR_SET)))

#define MAXSYM 256 /* ASCII */
#define WORDB     133    /* -w option */
#define LPARENT   134    /* ( */
#define RPARENT   135    /* ) */
#define LRANGE    136    /* [ */
#define RRANGE    137    /* ] */
#define LANGLE    138    /* < */
#define RANGLE    139    /* > */
#define NOTSYM    140    /* ^ */
#define WILDCD    141    /* wildcard */
#define ORSYM     142   /* | */
#define ORPAT     143   /* , */
#define ANDPAT    144   /* ; */
#define STAR      145   /* closure */
#define HYPHEN    129   /* - */
#define NOCARE    130   /* . */
#define NNLINE    131   /* special symbol for newline in begin of pattern*/
					   /* matches '\n' and NNLINE */
#define USERRANGE_MIN 128 	/* min char in pattern of user: give warning */
#define USERRANGE_MAX 145	/* max char in pattern of user: give warning */

#else

/* The following characters cannot be searched, because they are used
   as meta symbols. We need 16 meta symbols.
   
   The table is optimised for codepage 850; only some code of graphical
   characters are now used as meta symbols and cannot be searched therefore.

   These meta symbols are also flagged as such in table UL850 (module ISO_CHAR.H).
   (is_metasymbol == 1)
      
   [TG] 22.09.96
   
*/
   
#define MAXSYM	256 /* ASCII */
extern unsigned char metasymb[16];

#define WORDB     metasymb[0]	/* -w option */
#define LPARENT   metasymb[1]	/* ( */
#define RPARENT   metasymb[2]	/* ) */
#define LRANGE    metasymb[3]	/* [ */
#define RRANGE    metasymb[4]	/* ] */
#define LANGLE    metasymb[5]	/* < */
#define RANGLE    metasymb[6]	/* > */
#define NOTSYM    metasymb[7]	/* ^ */
#define WILDCD    metasymb[8]	/* # wildcard */
#define ORSYM     metasymb[9]	/* | */
#define ORPAT     metasymb[10]	/* , */
#define ANDPAT    metasymb[11]	/* ; */
#define STAR      metasymb[12]	/* * closure */
#define HYPHEN    metasymb[13]	/* - */
#define NOCARE    metasymb[14]	/* . */
#define NNLINE    metasymb[15]	/* special symbol for newline in begin of pattern*/
				/* matches '\n' and NNLINE */

/* not used anymore: [TG] */			
#define USERRANGE_MIN 0 	/* min char in pattern of user: give warning */
#define USERRANGE_MAX 0		/* max char in pattern of user: give warning */

#endif

/* not a comment anylonger [TG] 16.09.96 */
#define OUTPUT_OVERFLOW	fprintf(stderr, "Output buffer overflow after %d bytes @ %s:%d !!\n", agrep_outpointer, __FILE__, __LINE__)

extern int exists_delimiter(unsigned char *begin, unsigned char *end, unsigned char *delim, int len);
extern void preprocess_delimiter(unsigned char *src, int srclen, unsigned char *dest, int *pdestlen);
extern unsigned char * backward_delimiter(unsigned char *end, unsigned char *begin, unsigned char *delim, int len, int outtail);
extern unsigned char * forward_delimiter(unsigned char *begin, unsigned char *end, unsigned char *delim, int len, int outtail);

int exists_tcompressed_word();
unsigned char * forward_tcompressed_word(), *backward_tcompressed_word();
extern void alloc_buf(int fd, unsigned char **sbuf, int size);
extern void free_buf(int fd, char *sbuf);


/* To parse patterns in asplit.c */
#define AND_EXP 0x1	/* boolean ; -- remains set throughout */
#define OR_EXP 0x2	/* boolean , -- remains set throughout */
#define ATTR_EXP 0x4	/* set when = is next non-alpha char, remains set until next , or ; --> never used in agrep */
#define VAL_EXP 0x8	/* set all the time except when = is seen for first time --> never used in agrep */
#define ENDSUB_EXP 0x10	/* set when , or ; is seen: must unset ATTR_EXP now --> never used in agrep */

#define INTERNAL 1
#define LEAF 2
#define NOTPAT 0x1000
#define OPMASK 0x00ff

typedef struct _ParseTree {
        short   op;
        char    type;
        char    terminalindex;
        union {
                struct {
                        struct _ParseTree *left, *right;
                } internal;
                struct {
                        unsigned char    *attribute;	/* never used in agrep */
                        unsigned char    *value;
                } leaf;
        } data;
} ParseTree;

#define unget_token_bool(bufptr, tokenlen) (*(bufptr)) -= (tokenlen)

#define dd(a,b)	1
#define AGREP_ERROR	123	/* errno = 123 means that glimpse should quit searching files: used for errors glimpse itself cannot detect but agrep can */

#if	ISO_CHAR_SET	/* From Henrik.Martin@eua.ericsson.se (Henrik Martin) */
#define IS_LOCALE_CHAR(c) ((isalnum((c)) || isxdigit((c)) || \
	isspace((c)) || ispunct((c)) || iscntrl((c))) ? 1 : 0)
#define ISASCII(c)	IS_LOCALE_CHAR(c)
#else
#define ISASCII(c)	isascii(c)
#endif
#endif /* _AGREP_H_ */

