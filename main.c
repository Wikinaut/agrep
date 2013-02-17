/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
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

#include "dummysyscalls.c"

int
main(argc, argv)
int argc;
char *argv[];
{
	int	ret;

#if	ISO_CHAR_SET
	setlocale(LC_ALL,"");	/* support for 8bit character set: ew@senate.be, Henrik.Martin@eua.ericsson.se, "O.Bartunov" <megera@sai.msu.su>, S.Nazin (leng@sai.msu.su) */
#endif

	EXITONERROR = 1;	/* the only place where it is set to 1 */
	ret = fileagrep(argc, argv, 0, stdout);

#if	MEASURE_TIMES
	fprintf(stderr, "ret = %d infilter = %d ms\toutfilter = %d ms\tfilteralgo = %d ms\n", ret, INFILTER_ms, OUTFILTER_ms, FILTERALGO_ms);
#endif	/*MEASURE_TIMES*/
	if(ret<0) exit(2);
	if(ret==0) exit(1);
	exit(0);
}
