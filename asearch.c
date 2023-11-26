/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
#include "agrep.h"

extern unsigned Init1, Init[], Mask[], endposition, D_endpos, AND, NO_ERR_MASK;
extern int DELIMITER, FILENAMEONLY, INVERSE;
extern CHAR CurrentFileName[];
extern int I, num_of_matched, prev_num_of_matched, TRUNCATE;

extern int CurrentByteOffset;
extern int errno;
extern CHAR *agrep_inbuffer;
extern int  agrep_inlen;
extern int  agrep_initialfd;
extern int  EXITONERROR;
extern int  agrep_inpointer;

extern FILE *agrep_finalfp;
extern CHAR *agrep_outbuffer;
extern int agrep_outlen;
extern int agrep_outpointer;

extern int NEW_FILE, POST_FILTER;

extern int LIMITOUTPUT, LIMITPERFILE;

int  output();            /* agrep.c */
int  asearch0();          /* asearch.c */
int  fill_buf();          /* bitap.c */

int
asearch(old_D_pat, text, D)
CHAR old_D_pat[]; 
int text; 
register unsigned D;
{
	register unsigned i, c, r1, r2, CMask, r_NO_ERR, r_Init1; 
	register unsigned A0, B0, A1, B1, endpos;
	unsigned A2, B2, A3, B3, A4, B4;
	unsigned A[MaxError+1], B[MaxError+1];
	unsigned D_Mask;
	int end;
	int D_length, FIRSTROUND, ResidueSize, lasti, l, k, j=0;
	int printout_end;
	CHAR *buffer;
	/* CHAR *tempbuf = NULL; */	/* used only when text == -1 */

	if (I == 0) Init1 = (unsigned)037777777777;
	if(D > 4) {
		return asearch0(old_D_pat, text, D); 
	}

	D_length = strlen(old_D_pat);
	D_Mask = D_endpos;
	for ( i=1; i<D_length; i++) D_Mask = (D_Mask<<1) | D_Mask;
	D_Mask = ~D_Mask;

	r_Init1 = Init1; /* put Init1 in register */
	r_NO_ERR = NO_ERR_MASK; /* put NO_ERR_MASK in register */
	endpos = D_endpos;    
	FIRSTROUND = ON;
	A0 = B0 = A1 = B1 = A2 = B2 = A3 = B3 = A4 = B4 = Init[0];
	for(k=0; k<=D; k++) A[k] = B[k] = Init[0];

#if	AGREP_POINTER
	if (text != -1) {
#endif	/*AGREP_POINTER*/
		lasti = Max_record;
		alloc_buf(text, &buffer, Max_record+BlockSize+1);
		buffer[Max_record-1] = '\n';

		while ((l = fill_buf(text, buffer + Max_record, BlockSize)) > 0)
		{
			i = Max_record;
			end = Max_record + l ;
			if (FIRSTROUND) { 
				i = Max_record - 1;
				if(DELIMITER) {
					for(k=0; k<D_length; k++) {
						if(old_D_pat[k] != buffer[Max_record+k]) break;
					}
					if(k>=D_length) j--;
				}
				FIRSTROUND = OFF; 
			}
			if (l < BlockSize) {	/* copy pattern and '\0' at end of buffer */
				strncpy(buffer+end, old_D_pat, D_length);
				buffer[end+D_length] = '\0';
				end = end + D_length; 
			}

			/* ASEARCH_PROCESS: the while-loop below */
			while (i < end )
			{
				c = buffer[i];
				CMask = Mask[c];
				r1 = r_Init1 & B0;
				A0 = ((B0 >>1 ) & CMask) | r1;
				r1 = r_Init1 & B1;
				r2 =  B0 | (((A0 | B0) >> 1) & r_NO_ERR); 
				A1 = ((B1 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 1) goto Nextcharfile;
				r1 = r_Init1 & B2;
				r2 =  B1 | (((A1 | B1) >> 1) & r_NO_ERR); 
				A2 = ((B2 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 2) goto Nextcharfile;
				r1 = r_Init1 & B3;
				r2 =  B2 | (((A2 | B2) >> 1) & r_NO_ERR); 
				A3 = ((B3 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 3) goto Nextcharfile;
				r1 = r_Init1 & B4;
				r2 =  B3 | (((A3 | B3) >> 1) & r_NO_ERR); 
				A4 = ((B4 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 4) goto Nextcharfile;
Nextcharfile:
				i=i+1;
				CurrentByteOffset ++;
				if(A0 & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = A0;
					if ( D == 1) r1 = A1;
					if ( D == 2) r1 = A2;
					if ( D == 3) r1 = A3;
					if ( D == 4) r1 = A4;
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0;  
						}
						printout_end = i - D_length - 1 ; 
						if ((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length; /* point to starting position of D_pat */
					TRUNCATE = OFF;
					for(k=0; k<= D; k++) {
						B[k] = Init[0];
					}
					r1 = B[0] & Init1;
					A[0] = (((B[0]>>1) & CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = Init1 & B[k];
						r2 = B[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						A[k] = (((B[k]>>1)&CMask) | r1 | r2) ;
					}
					A0 = A[0]; 
					B0 = B[0]; 
					A1 = A[1]; 
					B1 = B[1]; 
					A2 = A[2]; 
					B2 = B[2];
					A3 = A[3]; 
					B3 = B[3]; 
					A4 = A[4]; 
					B4 = B[4];
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i];
				CMask = Mask[c];
				r1 = r_Init1 & A0;
				B0 = ((A0 >> 1 ) & CMask) | r1;
				/* printf("Mask = %o, B0 = %on", CMask, B0); */
				r1 = r_Init1 & A1;
				r2 =  A0 | (((A0 | B0) >> 1) & r_NO_ERR); 
				B1 = ((A1 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 1) goto Nextchar1file;
				r1 = r_Init1 & A2;
				r2 =  A1 | (((A1 | B1) >> 1) & r_NO_ERR); 
				B2 = ((A2 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 2) goto Nextchar1file;
				r1 = r_Init1 & A3;
				r2 =  A2 | (((A2 | B2) >> 1) & r_NO_ERR); 
				B3 = ((A3 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 3) goto Nextchar1file;
				r1 = r_Init1 & A4;
				r2 =  A3 | (((A3 | B3) >> 1) & r_NO_ERR); 
				B4 = ((A4 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 4) goto Nextchar1file;
Nextchar1file:
				i=i+1;
				CurrentByteOffset ++;
				if(B0 & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = B0;
					if ( D == 1) r1 = B1;
					if ( D == 2) r1 = B2;
					if ( D == 3) r1 = B3;
					if ( D == 4) r1 = B4;
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
					{ 
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;
							free_buf(text, buffer);
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0; 
						}
						printout_end = i - D_length - 1 ; 
						if((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length ;
					TRUNCATE = OFF;
					for(k=0; k<= D; k++) {
						A[k] = Init[0];
					}
					r1 = A[0] & Init1; 
					B[0] = (((A[0]>>1)&CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = Init1 & A[k];
						r2 = A[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						B[k] = (((A[k]>>1)&CMask) | r1 | r2) ;
					}
					A0 = A[0]; 
					B0 = B[0]; 
					A1 = A[1]; 
					B1 = B[1]; 
					A2 = A[2]; 
					B2 = B[2];
					A3 = A[3]; 
					B3 = B[3]; 
					A4 = A[4]; 
					B4 = B[4];
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
			}

			if(l < BlockSize) {
				lasti = Max_record ;
			}
			else {
				ResidueSize = Max_record + l - lasti;
				if(ResidueSize > Max_record) {
					ResidueSize = Max_record;
					TRUNCATE = ON;         
				}
				strncpy(buffer+Max_record-ResidueSize, buffer+lasti, ResidueSize);
				lasti = Max_record - ResidueSize;
				if(lasti == 0)     lasti = 1; 
			}
		}
		free_buf(text, buffer);
		return 0;
#if	AGREP_POINTER
	}
	else {
		lasti = 1;
		/* if (DELIMITER) tempbuf = (CHAR*)malloc(D_length + 1); */
		buffer = (CHAR *)agrep_inbuffer;
		l = agrep_inlen;
		end = l;
		/* buffer[end-1] = '\n'; */ /* at end of the text. */
		/* buffer[0] = '\n'; */  /* in front of the  text. */
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

			/* An exact copy of the above ASEARCH_PROCESS: the while-loop below */
			while (i < end )
			{
				c = buffer[i];
				CMask = Mask[c];
				r1 = r_Init1 & B0;
				A0 = ((B0 >>1 ) & CMask) | r1;
				r1 = r_Init1 & B1;
				r2 =  B0 | (((A0 | B0) >> 1) & r_NO_ERR); 
				A1 = ((B1 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 1) goto Nextcharmem;
				r1 = r_Init1 & B2;
				r2 =  B1 | (((A1 | B1) >> 1) & r_NO_ERR); 
				A2 = ((B2 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 2) goto Nextcharmem;
				r1 = r_Init1 & B3;
				r2 =  B2 | (((A2 | B2) >> 1) & r_NO_ERR); 
				A3 = ((B3 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 3) goto Nextcharmem;
				r1 = r_Init1 & B4;
				r2 =  B3 | (((A3 | B3) >> 1) & r_NO_ERR); 
				A4 = ((B4 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 4) goto Nextcharmem;
Nextcharmem: 
				i=i+1;
				CurrentByteOffset ++;
				if(A0 & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = A0;
					if ( D == 1) r1 = A1;
					if ( D == 2) r1 = A2;
					if ( D == 3) r1 = A3;
					if ( D == 4) r1 = A4;
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0;  
						}
						printout_end = i - D_length - 1 ; 
						if ((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length; /* point to starting position of D_pat */
					TRUNCATE = OFF;
					for(k=0; k<= D; k++) {
						B[k] = Init[0];
					}
					r1 = B[0] & Init1;
					A[0] = (((B[0]>>1) & CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = Init1 & B[k];
						r2 = B[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						A[k] = (((B[k]>>1)&CMask) | r1 | r2) ;
					}
					A0 = A[0]; 
					B0 = B[0]; 
					A1 = A[1]; 
					B1 = B[1]; 
					A2 = A[2]; 
					B2 = B[2];
					A3 = A[3]; 
					B3 = B[3]; 
					A4 = A[4]; 
					B4 = B[4];
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i];
				CMask = Mask[c];
				r1 = r_Init1 & A0;
				B0 = ((A0 >> 1 ) & CMask) | r1;
				/* printf("Mask = %o, B0 = %on", CMask, B0); */
				r1 = r_Init1 & A1;
				r2 =  A0 | (((A0 | B0) >> 1) & r_NO_ERR); 
				B1 = ((A1 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 1) goto Nextchar1mem;
				r1 = r_Init1 & A2;
				r2 =  A1 | (((A1 | B1) >> 1) & r_NO_ERR); 
				B2 = ((A2 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 2) goto Nextchar1mem;
				r1 = r_Init1 & A3;
				r2 =  A2 | (((A2 | B2) >> 1) & r_NO_ERR); 
				B3 = ((A3 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 3) goto Nextchar1mem;
				r1 = r_Init1 & A4;
				r2 =  A3 | (((A3 | B3) >> 1) & r_NO_ERR); 
				B4 = ((A4 >>1 ) & CMask) | r2 | r1 ;  
				if(D == 4) goto Nextchar1mem;
Nextchar1mem: 
				i=i+1;
				CurrentByteOffset ++;
				if(B0 & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = B0;
					if ( D == 1) r1 = B1;
					if ( D == 2) r1 = B2;
					if ( D == 3) r1 = B3;
					if ( D == 4) r1 = B4;
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
					{ 
						if(FILENAMEONLY && (NEW_FILE || !POST_FILTER)) {
							num_of_matched++;
							free_buf(text, buffer);
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0; 
						}
						printout_end = i - D_length - 1 ; 
						if((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length ;
					TRUNCATE = OFF;
					for(k=0; k<= D; k++) {
						A[k] = Init[0];
					}
					r1 = A[0] & Init1; 
					B[0] = (((A[0]>>1)&CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = Init1 & A[k];
						r2 = A[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						B[k] = (((A[k]>>1)&CMask) | r1 | r2) ;
					}
					A0 = A[0]; 
					B0 = B[0]; 
					A1 = A[1]; 
					B1 = B[1]; 
					A2 = A[2]; 
					B2 = B[2];
					A3 = A[3]; 
					B3 = B[3]; 
					A4 = A[4]; 
					B4 = B[4];
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
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

int
asearch0(old_D_pat, text, D)
CHAR old_D_pat[]; 
int text; 
register unsigned D;
{
	register unsigned i, c, r1, r2, CMask, r_NO_ERR, r_Init1,  end, endpos; 
	unsigned A[MaxError+2], B[MaxError+2];
	unsigned D_Mask;
	int D_length, FIRSTROUND, ResidueSize, lasti, l, k, j=0;
	int printout_end;
	CHAR *buffer;
	/* CHAR *tempbuf = NULL;*/	/* used only when text == -1 */

	D_length = strlen(old_D_pat);
	D_Mask = D_endpos;
	for ( i=1; i<D_length; i++) D_Mask = (D_Mask<<1) | D_Mask;
	D_Mask = ~D_Mask;

	r_Init1 = Init1; /* put Init1 in register */
	r_NO_ERR = NO_ERR_MASK; /* put NO_ERR_MASK in register */
	endpos = D_endpos;    
	FIRSTROUND = ON;
	for(k=0; k<=D; k++) A[k] = B[k] = Init[0];

#if	AGREP_POINTER
	if (text != -1) {
#endif	/*AGREP_POINTER*/
		lasti = Max_record;
		alloc_buf(text, &buffer, BlockSize+Max_record+1);
		buffer[Max_record-1] = '\n';
		while ((l = fill_buf(text, buffer + Max_record, BlockSize)) > 0)
		{
			i = Max_record; 
			end = Max_record + l ;
			if (FIRSTROUND) { 
				i = Max_record - 1;
				FIRSTROUND = OFF; 
			}
			if (l < BlockSize) {
				strncpy(buffer+end, old_D_pat, D_length);
				buffer[end+D_length] = '\0';
				end = end + D_length; 
			}

			/* ASEARCH0_PROCESS: the while-loop below */
			while (i < end )
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1 = B[0] & r_Init1;
				A[0] = ((((B[0] >> 1)) & CMask) | r1 ) ;
				for(k=1; k<=D; k++) {
					r1 = r_Init1 & B[k];
					r2 = B[k-1] | (((A[k-1]|B[k-1])>>1) & r_NO_ERR);
					A[k] = ((B[k] >> 1) & CMask) | r2 | r1;
				}
				if(A[0] & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = A[D];
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0;  
						}
						printout_end = i - D_length - 1;           
						if((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length; /* point to starting position of D_pat */
					for(k=0; k<= D; k++) {
						B[k] = Init[0];
					}
					r1 = B[0] & r_Init1;
					A[0] = (((B[0]>>1) & CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = Init1 & B[k];
						r2 = B[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						A[k] = (((B[k]>>1)&CMask) | r1 | r2) ;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1   = r_Init1 & A[0];
				B[0] = ((A[0] >> 1 ) & CMask) | r1;
				for(k=1; k<=D; k++) {
					r1 = r_Init1 & A[k];
					r2 = A[k-1] | (((A[k-1]|B[k-1])>>1) & r_NO_ERR);
					B[k] = ((A[k] >> 1) & CMask) | r2 | r1;
				}
				if(B[0] & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = B[D];
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0; 
						}
						printout_end = i - D_length -1 ; 
						if((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length ;
					for(k=0; k<= D; k++) {
						A[k] = Init[0];
					}
					r1 = A[0] & r_Init1; 
					B[0] = (((A[0]>>1)&CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = r_Init1 & A[k];
						r2 = A[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						B[k] = (((A[k]>>1)&CMask) | r1 | r2) ;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
			}

			if(l < BlockSize) {
				lasti = Max_record;
			}
			else {
				ResidueSize = Max_record + l - lasti;
				if(ResidueSize > Max_record) {
					ResidueSize = Max_record;
					TRUNCATE = ON;         
				}
				strncpy(buffer+Max_record-ResidueSize, buffer+lasti, ResidueSize);
				lasti = Max_record - ResidueSize;
				if(lasti == 0)     lasti = 1; 
			}
		}
		free_buf(text, buffer);
		return 0;
#if	AGREP_POINTER
	}
	else {
		lasti = 1;
		/* if (DELIMITER) tempbuf = (CHAR*)malloc(D_length + 1); */
		buffer = (CHAR *)agrep_inbuffer;
		l = agrep_inlen;
		end = l;
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

			/* An exact copy of the above ASEARCH0_PROCESS: the while-loop below */
			while (i < end )
			{
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1 = B[0] & r_Init1;
				A[0] = ((((B[0] >> 1)) & CMask) | r1 ) ;
				for(k=1; k<=D; k++) {
					r1 = r_Init1 & B[k];
					r2 = B[k-1] | (((A[k-1]|B[k-1])>>1) & r_NO_ERR);
					A[k] = ((B[k] >> 1) & CMask) | r2 | r1;
				}
				if(A[0] & endpos) {
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					j++;  
					r1 = A[D];
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0;  
						}
						printout_end = i - D_length - 1;           
						if((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length; /* point to starting position of D_pat */
					for(k=0; k<= D; k++) {
						B[k] = Init[0];
					}
					r1 = B[0] & r_Init1;
					A[0] = (((B[0]>>1) & CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = Init1 & B[k];
						r2 = B[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						A[k] = (((B[k]>>1)&CMask) | r1 | r2) ;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
				c = buffer[i++];
				CurrentByteOffset ++;
				CMask = Mask[c];
				r1   = r_Init1 & A[0];
				B[0] = ((A[0] >> 1 ) & CMask) | r1;
				for(k=1; k<=D; k++) {
					r1 = r_Init1 & A[k];
					r2 = A[k-1] | (((A[k-1]|B[k-1])>>1) & r_NO_ERR);
					B[k] = ((A[k] >> 1) & CMask) | r2 | r1;
				}
				if(B[0] & endpos) {
					j++;  
					if (DELIMITER) CurrentByteOffset -= D_length;
					else CurrentByteOffset -= 1;
					r1 = B[D];
					if(((AND == 1) && ((r1 & endposition) == endposition)) || ((AND == 0) && (r1 & endposition)) ^ INVERSE )
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
									if (text == -1) {
										memcpy(buffer+end-D_length, tempbuf, D_length+1);
									}
									*/
									free_buf(text, buffer);
									return -1;
								}
								else agrep_outbuffer[agrep_outpointer+outindex++] = '\n';
								agrep_outpointer += outindex;
							}
							/*
							if (text == -1) {
								memcpy(buffer+end-D_length, tempbuf, D_length+1);
							}
							*/
							free_buf(text, buffer);
							NEW_FILE = OFF;
							return 0; 
						}
						printout_end = i - D_length -1 ; 
						if((text != -1) && !(lasti >= Max_record + l - 1)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						else if ((text == -1) && !(lasti >= l)) {
							if (-1 == output(buffer, lasti, printout_end, j)) {free_buf(text, buffer); return -1;}
						}
						if (((LIMITOUTPUT > 0) && (LIMITOUTPUT <= num_of_matched)) ||
						    ((LIMITPERFILE > 0) && (LIMITPERFILE <= num_of_matched - prev_num_of_matched))) {
							free_buf(text, buffer);
							return 0;	/* done */
						}
					}
					lasti = i - D_length ;
					for(k=0; k<= D; k++) {
						A[k] = Init[0];
					}
					r1 = A[0] & r_Init1; 
					B[0] = (((A[0]>>1)&CMask) | r1) & D_Mask;
					for(k=1; k<= D; k++) {
						r1 = r_Init1 & A[k];
						r2 = A[k-1] | (((A[k-1] | B[k-1])>>1)&r_NO_ERR);
						B[k] = (((A[k]>>1)&CMask) | r1 | r2) ;
					}
					if (DELIMITER) CurrentByteOffset += 1*D_length;
					else CurrentByteOffset += 1*1;
				}
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
