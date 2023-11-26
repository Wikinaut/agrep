/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/*	[chg]	22.09.96	UL850[].lower_1 -> UL850[].lower	[TG]
	[chg]	21.08.96	now using ISO_CHAR.H and 
				table look-up for upper-lower conversion. [TG]
*/

#include "agrep.h"
#include "codepage.h"

/* TG 29.04.04 */
#include <errno.h>

extern unsigned char LUT[256];
extern int CODEPAGE;
extern struct CODEPAGE_struct CP[CODEPAGES][CPSIZE];

extern unsigned D_endpos, endposition, Init1, wildmask;
extern Mask[], Bit[], Init[], NO_ERR_MASK;
extern int AND, REGEX, NOUPPER, D_length;
extern unsigned char Progname[];
extern int agrep_initialfd;
extern int EXITONERROR;
extern int errno;

int
maskgen(Pattern, D)

unsigned char *Pattern; 
int D;
{
	struct term { 
		int flag; 
		unsigned char class[WORD];
	} 
	position[WORD+10];
	unsigned char c, pp;

	int i, j, k, l, M, OR=0, EVEN = 0, base, No_error;

#ifdef	DEBUG
	fprintf(stderr, "maskgen: len=%d, pat=%s, D=%d\n", strlen(Pattern), Pattern, D);
#endif
	for(i=0; i<WORD; i++) position[i].class[0] = '\0';
	for(i=0; i<WORD; i++) position[i].flag = 0;
	wildmask = NO_ERR_MASK = endposition = 0;
	No_error = 0;
	if ((M = strlen(Pattern)) <= 0) return 0;

	/* the metasymbols are not transposed in that table */
	
	if(NOUPPER) {
#if (defined(__EMX__) && defined(ISO_CHAR_SET))
		for(i=0; i<M; i++) Pattern[i] = LUT[Pattern[i]];
#else
		for(i=0; i<M; i++) if(isalpha((int)Pattern[i])) 
			if (isupper((int)Pattern[i])) Pattern[i] = tolower((int)Pattern[i]);
#endif
	}
	
#ifdef DEBUG
	for (i=0;i<256;i++) if (CP[CODEPAGE][i].metasymb > 0) printf("%d %c = metasymb[%d]\n",CP[CODEPAGE][i].upper,CP[CODEPAGE][i].upper,CP[CODEPAGE][i].metasymb);
	for(i=0; i<M; i++) printf("%d ", Pattern[i]);
	printf("\n");
	for(i=0; i<M; i++) printf("%c", Pattern[i]);
	printf("\n");
#endif
	for (i=0, j=1; i< M; i++)
	{
		pp=Pattern[i];
		
		if (pp==WILDCD) {
			if(REGEX) {
				position[j].class[0] = '.';
				position[j].class[1] = '.';
				position[j++].class[2] = '\0'; 
			}
			wildmask = wildmask | Bit[j-1];
		}
		else if (pp==LANGLE) { 
			No_error = ON; 
			EVEN++;
		}
		else if (pp==RANGLE) { 
			No_error = OFF; 
			EVEN--;
			if(EVEN < 0) {
				fprintf(stderr, "%s: unmatched '<', '>' (use \\<, \\> to search for <, >)\n", Progname);
				if (!EXITONERROR) {
					errno = AGREP_ERROR;
					return -1;
				}
				else exit(2);
			}
		}
		else if (pp==LRANGE) {
			if (No_error == ON) NO_ERR_MASK = NO_ERR_MASK | Bit[j];
			i=i+1; 
			if (Pattern[i] == NOTSYM) { 
				position[j].flag = Compl; 
				i++; 
			}
			k=0;
			while ((Pattern[i] != RRANGE) && i < M)
			{ 
				if(Pattern[i] == HYPHEN) 
				{ 
					position[j].class[k-1] = Pattern[i+1];
					i=i+2; 
				}
				else { 
					position[j].class[k] = position[j].class[k+1] = Pattern[i];
					k = k+2; 
					i++;
				}
			}
			if(i == M) {
				fprintf(stderr, "%s: unmatched '[', ']' (use \\[, \\] to search for [, ])\n", Progname);
				if (!EXITONERROR) {
					errno = AGREP_ERROR;
					return -1;
				}
				else exit(2);
			}
			position[j].class[k] = '\0';
			j++; 
		}
		else if (pp==RRANGE) {
			fprintf(stderr, "%s: unmatched '[', ']' (use \\[, \\] to search for [, ])\n", Progname);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
		else if (pp==ORPAT) {
			if(REGEX == ON || AND == ON) {
				fprintf(stderr, "illegal pattern: cannot handle OR (',') and AND (';')/regular-expressions simultaneously\n");
				if (!EXITONERROR) {
					errno = AGREP_ERROR;
					return -1;
				}
				else exit(2);
			}
			OR = ON;
			position[j].flag = 2; 
			position[j].class[0] = '\0';
			endposition = endposition | Bit[j++]; 
		}
		else if (pp==ANDPAT) {
			position[j].flag = 2; 
			position[j].class[0] = '\0'; 
			if(j > D_length) AND = ON;
			if(OR || (REGEX == ON && j>D_length)) {
				fprintf(stderr, "illegal pattern: cannot handle AND (';') and OR (',')/regular-expressions simultaneously\n");
				if (!EXITONERROR) {
					errno = AGREP_ERROR;
					return -1;
				}
				else exit(2);
			}
			endposition = endposition | Bit[j++]; 
		}
			/*
				case ' '    : if (Pattern[i-1] == ORPAT || Pattern[i-1] == ANDPAT) break;
							  if(No_error == ON) NO_ERR_MASK = NO_ERR_MASK | Bit[j];
							  position[j].flag = 0;
							  position[j].class[0] = position[j].class[1] = Pattern[i];
							  position[j++].class[2] = '\0';  break;
			*/
		else if (pp=='\n') {
			NO_ERR_MASK = NO_ERR_MASK | Bit[j];
			position[j].class[0] = position[j].class[1] = '\n';
			position[j++].class[2] = '\0'; 
		}
		else if (pp==WORDB) {
			NO_ERR_MASK = NO_ERR_MASK | Bit[j];
			position[j].class[0] = 1;
			position[j].class[1] = 47;
			position[j].class[2] = 58;
			position[j].class[3] = 64;
			position[j].class[4] = 91;
			position[j].class[5] = 96;
			position[j].class[6] = 123;
			position[j].class[7] = 127;
			position[j++].class[8] = '\0';
		}
		else if (pp==NNLINE) {
			NO_ERR_MASK |= Bit[j];
			position[j].class[0] = position[j].class[1] = '\n';
			position[j].class[2] = position[j].class[3] = NNLINE;
			position[j++].class[4] = '\0';
		}
		else if ((pp != STAR) & (pp != ORSYM) & (pp != LPARENT) & (pp != RPARENT)) {
		 	if(No_error == ON) NO_ERR_MASK = NO_ERR_MASK | Bit[j];
			position[j].flag = 0;
			position[j].class[0] = position[j].class[1] = Pattern[i];
			position[j++].class[2] = '\0'; 
		};
		
		if(j > WORD) {
			fprintf(stderr, "%s: pattern too long (has > %d chars)\n", Progname, WORD);
			if (!EXITONERROR) {
				errno = AGREP_ERROR;
				return -1;
			}
			else exit(2);
		}
	}
	if (EVEN != 0) {
		fprintf(stderr, "%s: unmatched '<', '>' (use \\<, \\> to search for <, >)\n", Progname);
		if (!EXITONERROR) {
			errno = AGREP_ERROR;
			return -1;
		}
		else exit(2);
	}
	M = j - 1;
	base = WORD - M;
	wildmask = (wildmask >> base);
	endposition = (endposition >> base);
	NO_ERR_MASK = (NO_ERR_MASK >> 1) & (~Bit[1]);
	NO_ERR_MASK = ~NO_ERR_MASK >> (base-1);
	for (i=1; i<= WORD - M ; i++) Init[0] = Init[0] | Bit[i];
	Init[0] = Init[0] | endposition;
	
	/* not necessary for INit[i], i>0, */
	/* but at every beginning of the matching process append one
	   no-match character to initialize the error vectors */
	   
	endposition = ( endposition << 1 ) + 1;
	Init1 = (Init[0] | wildmask | endposition) ;
	D_endpos = ( endposition >> ( M - D_length ) ) << ( M - D_length);
	endposition = endposition ^ D_endpos;
#ifdef DEBUG
	printf("endposition: %o\n", endposition);
	printf("no_err_mask: %o\n", NO_ERR_MASK);
#endif
	for(c=0, i=0; i < MAXSYM; c++, i++)
	{
		for (k=1, l=0; k<=M ; k++, l=0)  {
			while (position[k].class[l] != '\0') {
				if ((position[k].class[l] == NOCARE) && ((c != '\n') || REGEX) ) 
				{
					Mask[c] = Mask[c] | Bit[base + k]; 
					break; 
				}
				if (c >= position[k].class[l] && c <= position[k].class[l+1])
				{  
					Mask[c] = Mask[c] | Bit[base + k]; 
					break; 
				}
				l = l + 2;  
			}
			if (position[k].flag == Compl) Mask[c] = Mask[c] ^ Bit[base+k];
		}
	}

	if(NOUPPER) for(i=0; i<MAXSYM; i++)

#if (defined(__EMX__) && defined(ISO_CHAR_SET))

		Mask[i] = Mask[LUT[i]];
#else
		if (isupper(i)) Mask[i] = Mask[tolower(i)];
#endif

	return(M);
}
