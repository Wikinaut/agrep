/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
/* if the pattern is not simple fixed pattern, then after preprocessing */
/* and generating the masks, the program goes here. four cases:  1.     */ 
/* the pattern is simple regular expression and no error, then do the   */
/* matching here.  2. the pattern is simple regular expression and      */
/* unit cost errors are allowed: then go to asearch().                  */
/* 3. the pattern is simple regular expression, and the edit cost is    */
/* not uniform, then go to asearch1().                                  */
/* if the pattern is regular expression then go to re() if M < 14,      */
/* else go to re1()                                                     */
/* input parameters: old_D_pat: delimiter pattern.                      */
/* fd, input file descriptor, M: size of pattern, D: # of errors.       */

#include <autoconf.h>
#include "agrep.h"
#include "memory.h"
#include <errno.h>

extern int CurrentByteOffset;
extern unsigned Init1, D_endpos, endposition, Init[], Mask[], Bit[];
extern int LIMITOUTPUT, LIMITPERFILE;
extern int DELIMITER, FILENAMEONLY, D_length, I, AND, REGEX, JUMP, INVERSE, PRINTFILETIME; 
extern char D_pattern[];
extern int TRUNCATE, DD, S;
extern char Progname[], CurrentFileName[];
extern long CurrentFileTime;
extern int num_of_matched, prev_num_of_matched;
extern int agrep_initialfd;
extern int EXITONERROR;
extern int agrep_inlen;
extern CHAR *agrep_inbuffer;
extern int agrep_inpointer;
extern CHAR *agrep_outbuffer;
extern int agrep_outlen;
extern int agrep_outpointer;
extern FILE *agrep_finalfp;
extern int errno;

extern int NEW_FILE, POST_FILTER;

/* bitap dispatches job */

int
bitap(old_D_pat, Pattern, fd, M, D)
char old_D_pat[], *Pattern;  
int fd, M, D;  
{
	unsigned char c;	/* Patch to fix -n with ISO characters, "O.Bartunov" <megera@sai.msu.su>, S.Nazin (leng@sai.msu.su) */
	register unsigned r1, r2, r3, CMask, i;
	register unsigned end, endpos, r_Init1;
	register unsigned D_Mask;
	int  ResidueSize , FIRSTROUND, lasti, print_end, j, num_read;
	int  k;
	CHAR *buffer;
	int NumBufferFills;

	D_length = strlen(old_D_pat);
	for(i=0; i<D_length; i++) if(old_D_pat[i] == '^' || old_D_pat[i] == '$')
		old_D_pat[i] = '\n';
	if (REGEX) { 
		if (D > 4) {
			fprintf(stderr, "%s: the maximum number of erorrs allowed for full regular expressions is 4\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		if (M <= SHORTREG) { 
			return re(fd, M, D);   /* SUN: need to find a even point */
		}
		else { 
			return re1(fd, M, D);
		}
	}   
	if (D > 0 && JUMP == ON) 
	{ 
		return asearch1(old_D_pat, fd, D); 
	}
	if (D > 0) 
	{ 
		return asearch(old_D_pat, fd, D); 
	}
	if(I == 0) Init1 = (unsigned)037777777777;

	j=0;

	r_Init1 = Init1;
	r1 = r2 = r3 = Init[0];
	endpos = D_endpos;

	D_Mask = D_endpos;
	for(i=1 ; i<D_length; i++) D_Mask = (D_Mask << 1) | D_Mask;
	D_Mask = ~D_Mask;
	FIRSTROUND = ON;

#if	AGREP_POINTER
	if (fd != -1) {
#endif	/*AGREP_POINTER*/
		alloc_buf(fd, &buffer, 2*Max_record+BlockSize+1);
		buffer[Max_record-1] = '\n';
		lasti = Max_record;
		NumBufferFills = 0;
		while ((num_read = fill_buf(fd, buffer + Max_record, BlockSize)) > 0)
		{
			NumBufferFills++;
			i=Max_record; 
			end = Max_record + num_read; 
			if(FIRSTROUND) {  
				i = Max_record - 1 ;

				if(DELIMITER) {
					for(k=0; k<D_length; k++) {
						if(old_D_pat[k] != buffer[Max_record+k]) break;
					}
					if(k>=D_length) j--;
				}

				FIRSTROUND = OFF;  
			}
			if(num_read < BlockSize) {
				strncpy(buffer+Max_record+num_read, old_D_pat, D_length);
				end = end + D_length;
				buffer[end] = '\0';
			}

			/* BITAP_PROCESS: the while-loop below */
			while (i < end)
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1 = r_Init1 & r3;
				r2 = (( r3 >> 1 ) & CMask) | r1;
				if ( r2 & endpos ) {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((r2 & endposition) == endposition)) || ((AND == 0) && (r2 & endposition)) ^ INVERSE )
					{ 
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
									free_buf(fd, buffer);
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
										free_buf(fd, buffer);
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
									free_buf(fd, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(fd, buffer);
							NEW_FILE = OFF;
							return 0; 
						}

						print_end = i - D_length - 1;
						if ( ((fd != -1) && !(lasti >= Max_record+num_read - 1)) || ((fd == -1) && !(lasti >= num_read)) )
							if (-1 == output(buffer, lasti, print_end, j - (NumBufferFills - 1))) { free_buf(fd, buffer); return -1;} 
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(fd, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length; 
					TRUNCATE = OFF;
					r2 = r3 = r1 = Init[0];
					r1 = r_Init1 & r3;
					r2 = ((( r2 >> 1) & CMask) | r1 ) & D_Mask;
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1 = r_Init1 & r2;
				r3 = (( r2 >> 1 ) & CMask) | r1; 
				if ( r3 & endpos ) {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((r3 & endposition) == endposition)) || ((AND == 0) && (r3 & endposition)) ^ INVERSE )
					{ 
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
									free_buf(fd, buffer);
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
										free_buf(fd, buffer);
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
									free_buf(fd, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(fd, buffer);
							NEW_FILE = OFF;
							return 0; 
						}

						print_end = i - D_length - 1;
						if ( ((fd != -1) && !(lasti >= Max_record+num_read - 1)) || ((fd == -1) && !(lasti >= num_read)) )
							if (-1 == output(buffer, lasti, print_end, j - (NumBufferFills - 1))) { free_buf(fd, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(fd, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length ;
					TRUNCATE = OFF;
					r2 = r3 = r1 = Init[0]; 
					r1 = r_Init1 & r2;
					r3 = ((( r2 >> 1) & CMask) | r1 ) & D_Mask;
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}   
			}

			ResidueSize = num_read + Max_record - lasti;
			if(ResidueSize > Max_record) {
				ResidueSize = Max_record;
				TRUNCATE = ON;   
			}
			strncpy(buffer+Max_record-ResidueSize, buffer+lasti, ResidueSize);
			lasti = Max_record - ResidueSize;
			if(lasti < 0) {
				lasti = 1;
			} 
			if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
			    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
				free_buf(fd, buffer);
				return 0;	/* done */
			}
		}
		free_buf(fd, buffer);
		return 0;
#if	AGREP_POINTER
	}
	else {
		buffer = agrep_inbuffer;
		num_read = agrep_inlen;
		end = num_read;
		/* buffer[end-1] = '\n';*/ /* at end of the text. */
		/* buffer[0] = '\n';*/  /* in front of the  text. */
		i = 0;
		lasti = 1;

		if(DELIMITER) {
			for(k=0; k<D_length; k++) {
				if(old_D_pat[k] != buffer[k]) break;
			}
			if(k>=D_length) j--;
		}

			/* An exact copy of the above: BITAP_PROCESS: the while-loop below */
			while (i < end)
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1 = r_Init1 & r3;
				r2 = (( r3 >> 1 ) & CMask) | r1;
				if ( r2 & endpos ) {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((r2 & endposition) == endposition)) || ((AND == 0) && (r2 & endposition)) ^ INVERSE )
					{ 
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
									free_buf(fd, buffer);
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
										free_buf(fd, buffer);
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
									free_buf(fd, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(fd, buffer);
							NEW_FILE = OFF;
							return 0; 
						}

						print_end = i - D_length - 1;
						if ( ((fd != -1) && !(lasti >= Max_record+num_read - 1)) || ((fd == -1) && !(lasti >= num_read)) )
							if (-1 == output(buffer, lasti, print_end, j)) { free_buf(fd, buffer); return -1;} 
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(fd, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length; 
					TRUNCATE = OFF;
					r2 = r3 = r1 = Init[0];
					r1 = r_Init1 & r3;
					r2 = ((( r2 >> 1) & CMask) | r1 ) & D_Mask;
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1 = r_Init1 & r2;
				r3 = (( r2 >> 1 ) & CMask) | r1; 
				if ( r3 & endpos ) {
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((r3 & endposition) == endposition)) || ((AND == 0) && (r3 & endposition)) ^ INVERSE )
					{ 
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
									free_buf(fd, buffer);
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
										free_buf(fd, buffer);
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
									free_buf(fd, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer++] = '\n';
							}

							free_buf(fd, buffer);
							NEW_FILE = OFF;
							return 0; 
						}

						print_end = i - D_length - 1;
						if ( ((fd != -1) && !(lasti >= Max_record+num_read - 1)) || ((fd == -1) && !(lasti >= num_read)) )
							if (-1 == output(buffer, lasti, print_end, j)) { free_buf(fd, buffer); return -1;}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(fd, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length ;
					TRUNCATE = OFF;
					r2 = r3 = r1 = Init[0]; 
					r1 = r_Init1 & r2;
					r3 = ((( r2 >> 1) & CMask) | r1 ) & D_Mask;
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}   
			}

		return 0;
	}
#endif	/*AGREP_POINTER*/
}

fill_buf(fd, buf, record_size)
int fd, record_size; 
unsigned char *buf;
{
	int num_read=1;
	int total_read=0;
	extern int glimpse_clientdied;
	static int havePending = 0;
	static int pendingChar = 0;

	if (fd >= 0) {
		/* Decrement record size so we have room for an appended
		 * newline, if we might need one. */
		if (0 == DELIMITER) {
			--record_size;
                }
		if (havePending) {
			havePending = 0;
			buf [total_read++] = pendingChar;
		}
		while(total_read < record_size && num_read > 0) {
			if (glimpse_clientdied) return 0;
			num_read = read(fd, buf+total_read, record_size - total_read);
			total_read = total_read + num_read;
		}
		if (0 < num_read) {
			/* We're stopping because the buffer is full.  Save
			 * the last char for the next time through.  This
			 * guarantees, if we just read the last char, that
			 * on the next call we'll know that we still need to
			 * append a delimiter, even though we didn't "read"
			 * anything. */
			havePending = 1;
			pendingChar = buf [--total_read];
		} else {
			/* Stopping because we read the last char.  This
			 * resets state for the next call. */
			havePending = 0;
		}
		if ((0 == num_read) && /* Reached end-of-file */
                    (0 < total_read) &&	/* Got something, maybe from pending */
                    (0 == DELIMITER) &&	/* Not expecting special delimiter */
                    ('\n' != buf [total_read-1])) { /* Default delimiter not present */
			/* Add the default delimiter, so the last line of the
			 * file (terminated with EOF instead of newline) isn't
			 * quietly dropped. */
			buf [total_read] = '\n';
			++total_read;
		}
	}
#if	AGREP_POINTER
	else return 0;	/* should not call this function if buffer is a pointer to a user-specified region! */
#else	/*AGREP_POINTER*/
	else {	/* simulate a file */
		total_read = (record_size > (agrep_inlen - agrep_inpointer)) ? (agrep_inlen - agrep_inpointer) : record_size;
		memcpy(buf, agrep_inbuffer + agrep_inpointer, total_read);
		agrep_inpointer += total_read;
		/* printf("agrep_inpointer %d total_read %d\n", agrep_inpointer, total_read);*/
	}
#endif	/*AGREP_POINTER*/
	if (glimpse_clientdied) return 0;
	return(total_read);
}

/*
 * In these functions no allocs/copying is done when
 * fd == -1, i.e., agrep is called to search within memory.
 */

void
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

void
free_buf(fd, buf)
	int fd;
	char *buf;
{
#if	AGREP_POINTER
	if (fd != -1)
#endif	/*AGREP_POINTER*/
		free(buf);
}
