/*	AGREPHLP.C
	module for giving some pages of on-line help information and examples.

[chg]	TG 29.04.04 updated address info after almost 8  years
[chg]	TG 17.02.96 AGREPs homepage is now http://www.geocities.com/SiliconValley/Lakes/4889
[ini]	TG 25.09.96

*/

#ifdef _WIN32
#include "conio.h"
int get_current_codepage();  /* codepage.c */
#endif

#define	CUL	0x4B00
#define CUP	0x4800
#define CUD	0x5000
#define CUR	0x4D00
#define PgDn	0x5100
#define PgUp	0x4900
#define Home	0x4700
#define End	0x4F00

#define lastpage 7

#define userw	userwants=wf_user(); \
		switch (userwants) { \
		case Home: \
		case '1': pg=1; break; \
		case '2': pg=2; break; \
		case '3': pg=3; break; \
		case '4': pg=4; break; \
		case '5': pg=5; break; \
		case '6': pg=6; break; \
		case End: \
		case 'Q': \
		case 'q': pg=lastpage; break ; \
		case CUL: \
		case CUP: \
		case PgUp: \
		case '-': if (pg > 1) pg--; break; \
		default : if (pg < lastpage) pg++; break; \
		}; \
		compugoto
	
#define		compugoto \
		switch (pg) { \
		case 1: goto PAGE1; break; \
		case 2: goto PAGE2; break; \
		case 3: goto PAGE3; break; \
		case 4: goto PAGE4; break; \
		case 5: goto PAGE5; break; \
		case 6: goto PAGE6; break; \
		case lastpage: goto PAGE7; break; \
		default : break; }
				

#include <stdio.h>

#ifdef __EMX__
#include <sys/kbdscan.h>
#endif

#include "agrep.h"
#include "version.h"

extern	AGREPOPT_STR[MAX_LINE_LEN];
extern	VERBOSE;

#ifdef __EMX__
extern	unsigned int _emx_env;
#endif

void one_line_help(void)
{
fprintf(stderr,"\nAGREP [-#cdehi[a|#]klnprstvwxyABDGIRS] [-f patternfile] [-H dir] pattern [files]");
return;
}

unsigned int wf_user(void)
/* read from keyboard; process also some navigational functions keys */
{
unsigned int ch;

#ifdef __EMX__
	ch=_read_kbd(0,1,1);
	if (ch=='\0') ch=(_read_kbd(0,1,1) << 8 );
	/* no echo; wait for keystroke; Ctrl-C is not ignored */
#else
#ifndef _WIN32
	ch=getchar();
#else
	ch=getch();
#endif
#endif
return ch;
}

void agrep_online_help(void)
{
int		cpage, pg=1;
unsigned int	userwants;

PAGE1:
#ifdef __EMX__ 
fprintf(stderr,"\nAGREP %s for %s compiled with EMX 0.9c. Manber/Wu/Gries et al.(%s)\n",AGREP_VERSION,AGREP_OS,AGREP_DATE);
#else
#ifdef _WIN32
fprintf(stderr,"\nAGREP %s for %s compiled with MS VC 5.0 (%s)\n",AGREP_VERSION,AGREP_OS,AGREP_DATE);
#endif
#endif 
fprintf(stderr,"\n           Approximate Pattern Matching GREP -- Get Regular Expression\n");
fprintf(stderr,"Usage:");
one_line_help();
fprintf(stderr,"\n-#  find matches with at most # errors     -A  always output filenames\n");
fprintf(stderr,"-b  print byte offset of match\n");
fprintf(stderr,"-c  output the number of matched records   -B  find best match to the pattern\n");
fprintf(stderr,"-d  define record delimiter                -Dk deletion cost is k\n");
fprintf(stderr,"-e  for use when pattern begins with -     -G  output the files with a match\n");
fprintf(stderr,"-f  name of file containing patterns       -Ik insertion cost is k\n");
fprintf(stderr,"-h  do not display file names              -Sk substitution cost is k\n");
fprintf(stderr,"-i  case-insensitive search; ISO <> ASCII  -ia ISO chars mapped to lower ASCII\n");
fprintf(stderr,"-i# digits-match-digits, letters-letters   -i0 case-sensitive search\n");
fprintf(stderr,"-k  treat pattern literally - no meta-characters\n");
fprintf(stderr,"-l  output the names of files that contain a match\n");
fprintf(stderr,"-n  print line numbers of matches  -q print buffer byte offsets\n");
fprintf(stderr,"-p  supersequence search                   -CP 850|437 set codepage\n");
fprintf(stderr,"-r  recurse subdirectories (UNIX style)    -s silent\n");
fprintf(stderr,"-t  for use when delimiter is at the end of records\n");
fprintf(stderr,"-v  output those records without matches   -V[012345V] version / verbose more\n");
fprintf(stderr,"-w  pattern has to match as a word: \"win\" will not match \"wind\"\n");
fprintf(stderr,"-u  unterdruecke record output             -x  pattern must match a whole line\n");
fprintf(stderr,"-y  suppresses the prompt when used with -B best match option\n");
fprintf(stderr,"@listfile  use the filenames in listfile                              <1>23456Q");

userw;

PAGE2:
one_line_help();
fprintf(stderr,"\nThe pattern MUST BE ENCLOSED in \"DOUBLE QUOTES\" if it contains one of the\n");
fprintf(stderr,"following METASYMBOLS. Good practice is always to include it in double quotes.\n\n");

fprintf(stderr,"METASYMBOLS:\n");
fprintf(stderr,"\\z          turns off any special meaning of character z (\\# matches #)\n");
fprintf(stderr,"^           begin-of-line symbol\n");
fprintf(stderr,"$           end-of-line symbol\n");
fprintf(stderr,".           matches any single character (except newline)\n");
fprintf(stderr,"#           matches any number > 0 of arbitrary characters\n");
fprintf(stderr,"(a)*        matches zero or more instances of preceding token a (Kleene closure)\n");
fprintf(stderr,"a(a)*       matches one or more instances of preceding token a\n");
fprintf(stderr,"            (Use this as replacement for (a)+ which is not implemented yet.)\n\n");

fprintf(stderr,"[b-dq-tz]   matches characters b c d q r s t z\n");
fprintf(stderr,"[^b-diq-tz] matches all characters EXCEPT b c d i q r s t z\n");
fprintf(stderr,"ab|cd       matches \"ab\" OR \"cd\"\n");
fprintf(stderr,"<abcd>      matches exactly, no errors allowed in string \"abcd\"\n");
fprintf(stderr,"            (overrides the -1 option)\n\n");

fprintf(stderr,"cat,dog     matches records having \"cat\" OR \"dog\"\n");
fprintf(stderr,"cat;dog     matches records having \"cat\" AND \"dog\"\n");
fprintf(stderr,"            (operators  ;  and  ,  must not appear together in a pattern)\n");
fprintf(stderr,"                                                                      1<2>3456Q");

userw;

PAGE3:
one_line_help();
fprintf(stderr,"\nagrep \"colo#r\" foo\n");
fprintf(stderr,"     show lines in file foo having strings \"color\" or \"colour\" or\n");
fprintf(stderr,"     \"colonizer\" or \"coloniser\" etc.\n");
fprintf(stderr,"agrep -2 -ci miscellaneous foo\n");
fprintf(stderr,"     count lines in file foo having string \"miscellaneous\", within 2 errors,\n");
fprintf(stderr,"     case insensitive\n");
fprintf(stderr,"agrep -niuV0By neeedle foo 2>nul\n");
fprintf(stderr,"     show line numbers in file foo having string \"neeedle\", within least errors,\n");
fprintf(stderr,"     case insensitive\n");
fprintf(stderr,"agrep \"^From#\\.edu$\" foo\n");
fprintf(stderr,"     show lines in file foo having string \"From\" at the beginning of a line\n");
fprintf(stderr,"     and string \".edu\" at the end of the line\n");
fprintf(stderr,"agrep \"abc[0-9](de|fg)*[x-z]\" foo\n");
fprintf(stderr,"     show lines in file foo having string beginning \"abc\", followed by\n");
fprintf(stderr,"     one digit, then zero or more repetitions of \"de\" or \"fg\", and\n");
fprintf(stderr,"     finally x, y or z.\n");
fprintf(stderr,"agrep -d \"^From \" \"search;retriev\" mbox\n");
fprintf(stderr,"     show messages in file mbox having string \"search\" and string \"retriev\"\n");
fprintf(stderr,"     (Messages are delimited by the string \"From \" at the beginning of a line)\n");
fprintf(stderr,"agrep -1 -d \"$$\" \"<bug> <report>\" foo\n");
fprintf(stderr,"     show lines in file foo having string \"bug report\", or string \"bug\" at\n");
fprintf(stderr,"     end of a line and the string \"report\" at the beginning of the next line\n");
fprintf(stderr,"agrep -p \"ACME\" foo\n");
fprintf(stderr,"     find records in file foo that contain a supersequence of the pattern:\n");
fprintf(stderr,"     \"ACME\" will match \"A Company that Manufactures Everything\"\n");
fprintf(stderr,"agrep -i# \"11zz11\" foo\n");
fprintf(stderr,"     matches \"74LS04\" because of the digit-digit-letter(..) pattern   12<3>456Q");

userw;

PAGE4:
one_line_help();
fprintf(stderr,"\nAnd, how to search for double quotes \" ?\n\n");

fprintf(stderr,"   To search for string\" in all files *.c and to pipe the result\n");
fprintf(stderr,"   into a file x.x, use the following command:\n\n");

fprintf(stderr,"   >x.x AGREP \"string\\\\\\\"\" *.c\n\n");

fprintf(stderr,"   Comment: The sequence \\\\\\\" appears in AGREP as \\\" (search for \").\n\n");

fprintf(stderr,"The current default options as defined in the environment variable %s:\n\n",AGREP_ENV_OPTS);

fprintf(stderr,"   %s\n\n",AGREPOPT_STR);

fprintf(stderr,"   You could use \"SET %s=<your options>\" to change the default options.\n",AGREP_ENV_OPTS);
fprintf(stderr,"   The actual options in the command line take precedence.\n\n");

fprintf(stderr,"The current codepage ");
if ((cpage=get_current_codepage()) != -1) fprintf(stderr,"is CP %d.\n\n",cpage);
else fprintf(stderr,"could not be detected. AGREP will use CP850 by default.\n\n");

fprintf(stderr,"   The codepage setting affects the uppercase-lowercase translation table\n");
fprintf(stderr,"   built-in AGREP when you use one of the options -i, -ia or -i# .\n");
fprintf(stderr,"   The translation table can be printed by using verbose option -V5.\n\n");

fprintf(stderr,"The default verbose option is %d                                       123<4>56Q",VERBOSE);

userw;

PAGE5:
one_line_help();
fprintf(stderr,"\n   AGREP (C) COPYRIGHT\n\n");

fprintf(stderr,"   Glimpse was developed by Udi Manber and Burra Gopal of the University of\n");
fprintf(stderr,"   Arizona, and Sun Wu of the National Chung-Cheng University, Taiwan.\n");
fprintf(stderr,"   Permission is granted to copy this software, to redistribute it\n");
fprintf(stderr,"   on a nonprofit basis, and to use it for any purpose, subject to\n");
fprintf(stderr,"   the following restrictions and understandings.\n\n");

fprintf(stderr,"1. Any copy made of this software must include this copyright notice in full.\n\n");

fprintf(stderr,"2. All materials developed as a consequence of the use of this\n");
fprintf(stderr,"   software shall duly acknowledge such use, in accordance with the usual\n");
fprintf(stderr,"   standards of acknowledging credit in academic research.\n\n");

fprintf(stderr,"3. The authors have made no warranty or representation that the\n");
fprintf(stderr,"   operation of this software will be error-free or suitable for any\n");
fprintf(stderr,"   application, and they are under under no obligation to provide any\n");
fprintf(stderr,"   services, by way of maintenance, update, or otherwise.  The software\n");
fprintf(stderr,"   is an experimental prototype offered on an as-is basis.\n\n");

fprintf(stderr,"4. Redistribution for profit requires the express, written permission\n");
fprintf(stderr,"   of the authors.  Non-exclusive licenses can be obtained for a small fee.\n");
fprintf(stderr,"   Contact the authors at glimpse@cs.arizona.edu.\n\n");
fprintf(stderr,"                                                                      1234<5>6Q");

userw;

PAGE6:
one_line_help();
fprintf(stderr,"\nAGREP is a powerful tool for searching a file or many files for a string or\n");
fprintf(stderr,"regular expression, with approximate matching capabilities and user-definable\n");
fprintf(stderr,"records. AGREP was developed 1989-1991 by Sun Wu and Udi Manber and many others\n");
fprintf(stderr,"(please read CONTRIB.TXT and MANUAL.DOC).\n\n");

fprintf(stderr,"AGREP is the search engine and part of the GLIMPSE tool for searching and\n");
fprintf(stderr,"indexing whole file systems. GLIMPSE stands for GLobal IMPlicit SEarch and is\n");
fprintf(stderr,"part of the HARVEST Information Discovery and Access System.");

fprintf(stderr,"\n\nAGREP as of %s:\n",AGREP_DATE);
fprintf(stderr,"===============================================\n");
fprintf(stderr,"The home page for AGREP and GLIMPSE in general            http://webglimpse.net\n");
fprintf(stderr,"Home page AGREP                                      http://www.tgries.de/agrep\n\n");

fprintf(stderr,"Thank you for using AGREP.\n");
fprintf(stderr,"                                                                      12345<6>Q");

userw;

#ifdef __EMX__
if ((_emx_env & 0x0A00) == 0x0800) fprintf(stderr,"\n");
/* if running under pure DOS */
#endif

PAGE7:
return;
}
