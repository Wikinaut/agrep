/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
#ifndef _AGREP_H_
#define _AGREP_H_

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "re.h"
#include "defs.h"
#include "config.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAXNUM_PAT  16	/* 32 parts of a pattern = width of expression-tree */
#define CHAR	unsigned char
#define MAXPAT 256
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
#define BlockSize  49152/* BlockSize is always >= Max_record */
#define Max_record 49152
#define SIZE 16384       /* BlockSIze in sgrep */
#define MAXLINE   1024  /* maxline in sgrep */
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
#define MAX_DASHF_FILES	40000

#if	1
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
#define MAXSYM 256 /* ASCII */
#define WORDB     241    /* -w option */
#define LPARENT   242    /* ( */
#define RPARENT   243    /* ) */
#define LRANGE    244    /* [ */
#define RRANGE    245    /* ] */
#define LANGLE    246    /* < */
#define RANGLE    247    /* > */
#define NOTSYM    248    /* ^ */
#define WILDCD    249    /* wildcard */
#define ORSYM     250   /* | */
#define ORPAT     251   /* , */
#define ANDPAT    252   /* ; */
#define STAR      253   /* closure */
#define HYPHEN    237   /* - */
#define NOCARE    238   /* . */
#define NNLINE    239   /* special symbol for newline in begin of pattern*/
					   /* matches '\n' and NNLINE */
#define USERRANGE_MIN 236 	/* min char in pattern of user: give warning */
#define USERRANGE_MAX 255	/* max char in pattern of user: give warning */
#endif

#define OUTPUT_OVERFLOW	\
{ /* fprintf(stderr, "Output buffer overflow after %d bytes @ %s:%d !!\n", agrep_outpointer, __FILE__, __LINE__) */\
	errno = ERANGE;\
}

extern unsigned char *forward_delimiter(), *backward_delimiter();
extern int exists_delimiter();
extern void preprocess_delimiter();
unsigned char *forward_delimiter(), *backward_delimiter();
int exists_tcompressed_word();
unsigned char * forward_tcompressed_word(), *backward_tcompressed_word();
void alloc_buf(), free_buf();
extern char *aprint_file_time();

#define AGREP_VERSION	"3.0"
#define AGREP_DATE	"1994"


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
                        int    attribute;	/* never used in agrep */
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

extern int my_open();
extern FILE *my_fopen();
extern int my_stat();
extern int my_fstat();
extern int my_lstat();
extern int special_get_name();

#endif /* _AGREP_H_ */
