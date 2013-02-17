/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
#include "agrep.h"

extern unsigned Init1, Init[], Mask[], endposition, D_endpos;
extern unsigned NO_ERR_MASK;
extern int TRUNCATE, DELIMITER, AND, I, S, DD, INVERSE, FILENAMEONLY ;
extern char CurrentFileName[];
extern int num_of_matched, prev_num_of_matched;

extern int CurrentByteOffset;
extern CHAR *agrep_inbuffer;
extern int agrep_inlen;

extern FILE *agrep_finalfp;
extern CHAR *agrep_outbuffer;
extern int agrep_outlen;
extern int agrep_outpointer;

extern int NEW_FILE, POST_FILTER;

extern int LIMITOUTPUT, LIMITPERFILE;

#ifdef _WIN32
int  output();            /* agrep.c */
int  fill_buf();          /* bitap.c */
#endif

int
asearch1(old_D_pat, Text, D)
char old_D_pat[]; 
int Text; 
register unsigned D;
{
	register unsigned end, i, r1, r3, r4, r5, CMask, D_Mask, k, endpos; 
	register unsigned r_NO_ERR;
	unsigned A[MaxError*2+1], B[MaxError*2+1];
	int D_length, ResidueSize, lasti, num_read,  FIRSTROUND=1, j=0;
	CHAR *buffer;
	/* CHAR *tempbuf = NULL;*/	/* used only when Text == -1 */

	if(I == 0) Init1 = (unsigned)037777777777;
	if(DD > D) DD = D+1;
	if(I  > D) I  = D+1;
	if(S  > D) S  = D+1;
	D_length = strlen(old_D_pat);

	r_NO_ERR = NO_ERR_MASK;

	D_Mask = D_endpos;
	for(i=1; i<D_length; i++) D_Mask = (D_Mask << 1) | D_Mask;
	D_Mask = ~D_Mask;
	endpos = D_endpos;
	r3 = D+1; 
	r4 = D*2;  /* to make sure in register */
	for(k=0; k < D;   k++) A[k] = B[k] = 0;
	for(k=D; k <= r4; k++) A[k] = B[k] = Init[0];

#if	AGREP_POINTER
	if (Text != -1) {
#endif	/*AGREP_POINTER*/
		lasti = Max_record;
		alloc_buf(Text, &buffer, BlockSize+Max_record+1);
		buffer[Max_record-1] = '\n';

		while ((num_read = fill_buf(Text, buffer + Max_record, BlockSize)) > 0)
		{
			i=Max_record; 
			end = Max_record + num_read;
			if(FIRSTROUND) { 
				i = Max_record -1 ;
				if(DELIMITER) {
					for(k=0; k<D_length; k++) {
						if(old_D_pat[k] != buffer[Max_record+k]) break;
					}
					if(k>=D_length) j--;
				}
				FIRSTROUND = 0; 
			}
			if(num_read < BlockSize) {
				strncpy(buffer+Max_record+num_read, old_D_pat, D_length);
				end = end + D_length;
				buffer[end] = '\0';
			}

			/* ASEARCH1_PROCESS: the while-loop below */
			while (i < end)
			{
				CMask = Mask[buffer[i++]];
				CurrentByteOffset ++;
				r1 = Init1 & B[D];
				A[D] = ((B[D] >> 1) & CMask )  | r1;
				for(k = r3; k <= r4; k++)  /* r3 = D+1, r4 = 2*D */
				{ 
					r5 = B[k];
					r1 = Init1 & r5;
					A[k] = ((r5 >> 1) & CMask) | B[k-I] | (((A[k-DD] | B[k-S]) >>1) & r_NO_ERR) | r1 ; 
				}
				if(A[D] & endpos) {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((A[D*2] & endposition) == endposition)) || ((AND == 0) && (A[D*2] & endposition)) ^ INVERSE )
					{ 
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;
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
									/*
									if (Text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (Text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;       
						} 
						if((Text != -1) && !(lasti >= Max_record + num_read - 1)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						else if ((Text == -1) && !(lasti >= num_read)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length;
					TRUNCATE = OFF;
					for(k = D; k <= r4 ; k++) A[k] = B[k] = Init[0];
					r1 = Init1 & B[D];
					A[D] = (((B[D] >> 1) & CMask )  | r1) & D_Mask;
					for(k = r3; k <= r4; k++)  /* r3 = D+1, r4 = 2*D */
					{ 
						r5 = B[k];
						r1 = Init1 & r5;
						A[k] = ((r5 >> 1) & CMask) | B[k-I] | (((A[k-DD] | B[k-S]) >>1) & r_NO_ERR) | r1 ; 
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}  /* end if (A[D]&endpos) */
				CMask = Mask[buffer[i++]];
				CurrentByteOffset ++;
				r1 = A[D] & Init1;
				B[D] = ((A[D] >> 1) & CMask) | r1;
				for(k = r3; k <= r4; k++)
				{ 
					r1 = A[k] & Init1;
					B[k] = ((A[k] >> 1) & CMask) | A[k-I] | (((B[k-DD] | A[k-S]) >>1)&r_NO_ERR) | r1 ; 
				}
				if(B[D] & endpos)  {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((B[r4] & endposition) == endposition)) || ((AND == 0) && (B[r4] & endposition)) ^ INVERSE )
					{ 
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;
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
									/*
									if (Text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (Text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}
						if((Text != -1) && !(lasti >= Max_record + num_read - 1)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						else if ((Text == -1) && !(lasti >= num_read)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					} 
					lasti = i-D_length; 
					TRUNCATE = OFF;
					for(k=D; k <= r4; k++) A[k] = B[k] = Init[0];
					r1 = Init1 & A[D];
					B[D] = (((A[D] >> 1) & CMask )  | r1) & D_Mask;
					for(k = r3; k <= r4; k++)  /* r3 = D+1, r4 = 2*D */
					{ 
						r5 = A[k];
						r1 = Init1 & r5;
						B[k] = ((r5 >> 1) & CMask) | A[k-I] | (((B[k-DD] | A[k-S]) >>1) & r_NO_ERR) | r1 ; 
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}  /* end if (B[D]&endpos) */
			}

			ResidueSize = Max_record + num_read - lasti;
			if(ResidueSize > Max_record) {
				ResidueSize = Max_record;
				TRUNCATE = ON;   
			}
			strncpy(buffer+Max_record-ResidueSize, buffer+lasti, ResidueSize);
			lasti = Max_record - ResidueSize;
			if(lasti < 0) lasti = 1;
			if(num_read < BlockSize) lasti = Max_record;
		}
		free_buf(Text, buffer);
		return 0;
#if	AGREP_POINTER
	}
	else {
		lasti = 1;
		/* if (DELIMITER) tempbuf = (CHAR*)malloc(D_length + 1); */
		buffer = (CHAR *)agrep_inbuffer;
		num_read = agrep_inlen;
		end = num_read;
		/* buffer[end-1] = '\n';*/ /* at end of the text. */
		/* buffer[0] = '\n';*/  /* in front of the  text. */
		i = 0;

		if(DELIMITER) {
			for(k=0; k<D_length; k++) {
				if(old_D_pat[k] != buffer[k]) break;
			}
			if(k>=D_length) j--;
			/*
			memcpy(tempbuf, buffer+end, D_length+1);
			strncpy(buffer+end, old_D_pat, D_length);
			buffer[end+D_length] = '\0';
			end = end + D_length;
			*/
		}

			/* An exact copy of the above ASEARCH1_PROCESS: the while-loop below */
			while (i < end)
			{
				CMask = Mask[buffer[i++]];
				CurrentByteOffset ++;
				r1 = Init1 & B[D];
				A[D] = ((B[D] >> 1) & CMask )  | r1;
				for(k = r3; k <= r4; k++)  /* r3 = D+1, r4 = 2*D */
				{ 
					r5 = B[k];
					r1 = Init1 & r5;
					A[k] = ((r5 >> 1) & CMask) | B[k-I] | (((A[k-DD] | B[k-S]) >>1) & r_NO_ERR) | r1 ; 
				}
				if(A[D] & endpos) {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((A[D*2] & endposition) == endposition)) || ((AND == 0) && (A[D*2] & endposition)) ^ INVERSE )
					{ 
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;
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
									/*
									if (Text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (Text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;       
						} 
						if((Text != -1) && !(lasti >= Max_record + num_read - 1)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						else if ((Text == -1) && !(lasti >= num_read)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length;
					TRUNCATE = OFF;
					for(k = D; k <= r4 ; k++) A[k] = B[k] = Init[0];
					r1 = Init1 & B[D];
					A[D] = (((B[D] >> 1) & CMask )  | r1) & D_Mask;
					for(k = r3; k <= r4; k++)  /* r3 = D+1, r4 = 2*D */
					{ 
						r5 = B[k];
						r1 = Init1 & r5;
						A[k] = ((r5 >> 1) & CMask) | B[k-I] | (((A[k-DD] | B[k-S]) >>1) & r_NO_ERR) | r1 ; 
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}  /* end if (A[D]&endpos) */
				CMask = Mask[buffer[i++]];
				CurrentByteOffset ++;
				r1 = A[D] & Init1;
				B[D] = ((A[D] >> 1) & CMask) | r1;
				for(k = r3; k <= r4; k++)
				{ 
					r1 = A[k] & Init1;
					B[k] = ((A[k] >> 1) & CMask) | A[k-I] | (((B[k-DD] | A[k-S]) >>1)&r_NO_ERR) | r1 ; 
				}
				if(B[D] & endpos)  {  
					j++;
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					if(((AND == 1) && ((B[r4] & endposition) == endposition)) || ((AND == 0) && (B[r4] & endposition)) ^ INVERSE )
					{ 
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;
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
									/*
									if (Text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(Text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (Text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(Text, buffer);
							NEW_FILE = OFF;
							return 0;
						}
						if((Text != -1) && !(lasti >= Max_record + num_read - 1)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						else if ((Text == -1) && !(lasti >= num_read)) {
							if (-1 == output(buffer, lasti, i-D_length-1, j)) {free_buf(Text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(Text, buffer);
							return 0;	/* done */
						}
					} 
					lasti = i-D_length; 
					TRUNCATE = OFF;
					for(k=D; k <= r4; k++) A[k] = B[k] = Init[0];
					r1 = Init1 & A[D];
					B[D] = (((A[D] >> 1) & CMask )  | r1) & D_Mask;
					for(k = r3; k <= r4; k++)  /* r3 = D+1, r4 = 2*D */
					{ 
						r5 = A[k];
						r1 = Init1 & r5;
						B[k] = ((r5 >> 1) & CMask) | A[k-I] | (((B[k-DD] | A[k-S]) >>1) & r_NO_ERR) | r1 ; 
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}  /* end if (B[D]&endpos) */
			}

		/*
		if (DELIMITER) {
			memcpy(buffer+end, tempbuf, D_length+1);
			free(tempbuf);
		}
		*/
		return 0;
	}
#endif	/*AGREP_POINTER*/
}

