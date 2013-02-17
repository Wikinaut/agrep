/* Copyright (c) 1994 Burra Gopal, Udi Manber.  All Rights Reserved. */

#include "agrep.h"
extern int EASYSEARCH, TCOMPRESSED;

/* Accesses src completely before dest, so that dest can be = src */
void
preprocess_delimiter(src, srclen, dest, pdestlen)
	unsigned char *src, *dest;
	int	srclen, *pdestlen;
{
	CHAR temp[Maxline];
	int i, j;

	strcpy(temp, src);
	temp[srclen] = '\0';
	for (i=0, j=0; i<srclen; i++) {
		if (temp[i] == '\\') {
			i++;
			dest[j++] = temp[i];
		}
		else if (temp[i] == '^') dest[j++] = '\n';
		else if (temp[i] == '$') dest[j++] = '\n';
		else dest[j++] = temp[i];
	}
	dest[j] = '\0';
	*pdestlen = strlen(dest);
}

/* Simple quadratic search for delimiters: from, to, delim, len: offset after begin where delimiter begins, -1 otherwise */
int
exists_delimiter(begin, end, delim, len)
	unsigned char *begin, *end, *delim;
	int len;
{
	register unsigned char	*curbegin, *curbuf, *curdelim;

	if (begin + len > end) return -1;
	if (TCOMPRESSED == ON) return (exists_tcompressed_word(delim, len, begin, end - begin, EASYSEARCH));
	for (curbegin = begin; curbegin + len <= end; curbegin ++) {
		for (curbuf = curbegin, curdelim = delim; curbuf < curbegin + len; curbuf ++, curdelim++)
			if (*curbuf != *curdelim) break;
		if (curbuf >= curbegin + len) return (curbegin - begin);
	}
	return -1;
}

/* return where delimiter begins or ends (=outtail): range = [begin, end) */
unsigned char *
forward_delimiter(begin, end, delim, len, outtail)
	unsigned char *begin, *end, *delim;
	int len, outtail;
{
	register unsigned char	*curbegin, *curbuf, *curdelim;
	unsigned char *oldbegin = begin, *retval = begin;

	if (begin + len > end) {
		retval = end + 1;
		goto _ret;
	}
	if ((len == 1) && (*delim == '\n')) {
		begin ++;
		while ((begin < end) && (*begin != '\n')) begin ++;
		if (outtail && (*begin == '\n')) begin++;
		retval = begin;
		goto _ret;
	}
	if (TCOMPRESSED == ON) return forward_tcompressed_word(begin, end, delim, len, outtail, EASYSEARCH);
	for (curbegin = begin; curbegin + len <= end; curbegin ++) {
		for (curbuf = curbegin, curdelim = delim; curbuf < curbegin + len; curbuf ++, curdelim++)
			if (*curbuf != *curdelim) break;
		if (curbuf >= curbegin + len) break;
	}
	if (!outtail) retval = (curbegin <= end - len ? curbegin: end + 1);
	else retval = (curbegin <= end - len ? curbegin + len : end + 1);

_ret:
	/* Gurantee that this skips at least one character */
	if (retval <= oldbegin) return oldbegin + 1;
	else return retval;
}

/* return where the delimiter begins or ends (=outtail): range = [begin, end) */
unsigned char *
backward_delimiter(end, begin, delim, len, outtail)
	unsigned char *end, *begin, *delim;
	int len, outtail;
{
	register unsigned char	*curbegin, *curbuf, *curdelim;

	if (end - len < begin) return begin;
	if ((len == 1) && (*delim == '\n')) {
		end --;
		while ((end > begin) && (*end != '\n')) end --;
		if (outtail && (*end == '\n')) end++;
		return end;
	}
	if (TCOMPRESSED == ON) return backward_tcompressed_word(end, begin, delim, len, outtail, EASYSEARCH);
	for (curbegin = end-len; curbegin >= begin; curbegin --) {
		for (curbuf = curbegin, curdelim = delim; curbuf < curbegin + len; curbuf ++, curdelim++)
			if (*curbuf != *curdelim) break;
		if (curbuf >= curbegin + len) break;
	}
	if (!outtail) return (curbegin >= begin ? curbegin : begin);
	else return (curbegin >= begin ? curbegin + len : begin);
}
