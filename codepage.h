/**********************************************************
*  Headerfile for converting characters from upper case   *
*  to lower case for ISO code pages.                      *
*---------------------------------------------------------*

[new]	25.09.96	alpha/digit
[chg]	22.09.96	metasymbol [TG]
[chg]	21.08.96	changed from subroutine to struct [TG]
	
	Tom Gries <gries@ibm.net>
[ini]	Mike Thomas <rmthomas@sciolus.cistron.nl>

*******************************************************/

#define	CODEPGNR	256	/* array index of codepage identification number */
#define CPSIZE		257	/* 0..255 characters, one codepage identifier */
#define CODEPAGES	3	/* number of implemented codepages: 437, 850, ISO-8859-1 */

struct CODEPAGE_struct
  {
  
  unsigned char	lower_1;	/* output 1:	lowercase characters
  					- still accented
					- still "umlauted"		*/
					
  unsigned char	lower_2;	/* output 2:	lowercase characters
  					- mapped to the closest
					  ASCII lowercase character	*/
					  
  unsigned char lower_3;	/* output 3:	type indicator:
  
  					for the control characters <  ' ':
					
					- preserve the code;
					  for example, CR remains CR
					
  					for characters >= ' ':
					
					- 'a':	all letters are mapped to 'a'
					- '1':	all digits or what looks like: '1'
					- '#':	otherwise	*/
					  
  int metasymb;			/* output 3:
    					- >0 : character is used as metasymbol
					- metasymb[UL850[c].metasymb]=c;
    					- is not allowed in search pattern
					- AGREP.C shows a warning message */
  };
