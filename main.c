/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/* [TG]	3.29 14.10.96 */

#include <stdio.h>
#include "agrep.h"

#if	ISO_CHAR_SET
#include <locale.h>	/* support for 8bit character set: ew@senate.be */
#endif

#if	MEASURE_TIMES
extern int INFILTER_ms, OUTFILTER_ms, FILTERALGO_ms;
#endif	/*MEASURE_TIMES*/

extern  char Pattern[MAXPAT];
extern  int EXITONERROR;

#ifdef __EMX__
extern unsigned int _emx_env;	/* this variable denotes the operating system DOS, OS/2 */
#endif

int fileagrep();   /* agrep.c */

#ifndef _WIN32
int
#else
void
#endif
main(argc, argv)
int argc;
char *argv[];
{
	int	ret;


#ifdef 	__EMX__

/*	The emx-compiler specific variable _emx_env describes the environment
	the program is running in. See also: the makefiles.

	values of _emx_env	  OS/2:	  OS/2 DOS box:		   DOS:	   Win DOS box:

	AGREP2.EXE		 0x0220		      -		      -		      -
	AGREPDOS.EXE	+EMX.DLL=0x0A20		      -		 0x0803		      -
	AGREP.EXE	+EMX.DLL=0x0A20	+RSX.EXE=0x18A0	+EMX.EXE=0x0003	+RSX.EXE=0x18A0

 	fprintf(stderr,"AGREP -- INFO: _emx_env = 0x%04lX\n",_emx_env);
*/

#ifdef __DOS	/* check if compiled for DOS but running under OS/2 */
/*
	if ((_emx_env & 0x0A00)!=0x0800) {
		fprintf(stderr,"This program cannot be run in an OS/2 session.\nPlease use the OS/2 version.");

		ret = -1;
		goto ABORT;
	}
*/
#endif

#ifdef	__OS2	/* check if compiled for OS/2 but running under DOS or in a DOS session */
/*
	if ((_emx_env & 0x0A00)!=0x0200) {
		fprintf(stderr,"This program cannot be run in a DOS session.\nPlease use the DOS version.\n");

		ret = -1;
		goto ABORT;
	}
*/
#endif


#endif

	EXITONERROR = 1;	/* the only place where it is set to 1 */
	ret = fileagrep(argc, argv, 0, stdout);

#if     ISO_CHAR_SET
	setlocale(LC_ALL,"");       /* support for 8bit character set: ew@senate.be, Henrik.Martin@eua.ericsson.se */
#endif

#if	MEASURE_TIMES
	fprintf(stderr, "ret = %d infilter = %d ms\toutfilter = %d ms\tfilteralgo = %d ms\n", ret, INFILTER_ms, OUTFILTER_ms, FILTERALGO_ms);
#endif	/*MEASURE_TIMES*/

/*	The original return codes were:

	if(ret<0) exit(2);
	if(ret==0) exit(1);
	exit(0);
*/

ABORT:	exit(ret);	/* changed by [TG] 23.09.96 */
}
