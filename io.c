/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
#include "agrep.h"

/* AGREP_POINTER must be defined to be 1 always */
/* #define AGREP_POINTER	1 */
/* Removed since we now have a -DAGREP_POINTER=1 option in the Makefile */

fill_buf(fd, buf, record_size)
int fd, record_size; 
unsigned char *buf;
{
	int num_read=1;
	int total_read=0;

	if (fd >= 0) {
		while(total_read < record_size && num_read > 0) {
			num_read = read(fd, buf+total_read, record_size - total_read);
			total_read = total_read + num_read;
		}
	}
#if	AGREP_POINTER
	else return 0;	/* should not call this function if buf is a pointer to a user-specified region! */
#else	/*AGREP_POINTER*/
	else {	/* simulate a file */
		total_read = (record_size > (agrep_inlen - agrep_inpointer)) ? (agrep_len - agrep_inpointer) : record_size;
		memcpy(buf, agrep_inbuffer + agrep_inpointer, total_read);
		agrep_inpointer += total_read;
	}
#endif	/*AGREP_POINTER*/
	return(total_read);
}

/*
 * In these functions no allocs/copying is done when
 * fd == -1, i.e., agrep is called to search within memory.
 */

alloc_buf(fd, buf, size)
	int fd;
	char **buf;
	int size;
{
#if	AGREP_POINTER
	if (fd != -1)
#endif	/*AGREP_POINTER*/
		*buf = (char *)malloc(size);
}

free_buf(fd, buf)
	int fd;
	char *buf;
{
#if	AGREP_POINTER
	if (fd != -1)
#endif	/*AGREP_POINTER*/
		free(buf);
}
