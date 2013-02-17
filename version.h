/*
[chg] 3.411 TG 20130216 small changes, help file, version, preparing for Github, makefile makefile.os2, CHANGES
[fix] 3.40 TG 08.11.2004 option -By does not work together with -n. to allow this, I commented LINENUM out
[fix]	3.35	TG 11.12.97	in agrep(): -f now working again
				prepf() for multi-pattern was called
				before the codepage LUT was prepared
[fix]	3.34	TG 22.10.97	in mgrep(): check bounds before memcpy()
[chg]	3.33	TG 07.04.97	when no target filename(s) were given:
				AGREP displays an error message now
				instead of reading from stdin.
				Solves the following problem:
				When one uses "AGREP <needle> *" and there are
				no files in that subdirectory,
				the 3.32 has waited for stdin (=haystack, target).
[chg]				compiled with emx 0.9c
[new]				using EMX.EXE or RSX.EXE allows to run AGREP in DOS boxes
				under Windows or OS/2

[chg]   15.01.97 3.32   new links, helppage revised
[new]	16.12.96 3.31	new subswitch -i0
[chg]	16.10.96 3.30	constant unsigned _emx_env
[chg]	14.10.96 3.29	nicer help pages; not running under DOS/OS2 messages
[fix]		 3.28	bug fixed with metasymb[0]
[chg]	13.10.96 3.27 	improved navigation through the help pages
[new]	11.10.96 3.26	_read_kbd() while browsing through the help pages
			'q' or 'Q' quitts immediately
[fix]	10.10.96 3.25	sgrep.c: output problems at buffer end fixed
			(not at the end of file)
[fix]	08.10.96 3.23	codepage detection when compiled for DOS returns -1 now
[fix]		 3.22	-w bug repaired (in since 3.17)
[new]	07.10.96 3.21	verbose options -V[0123V]
[new]	06.10.96 3.20	multi-codepage support
[new]   05.10.96 3.19	using environment variable AGREPOPTS
[chg]   04.10.96 3.17	major bugs fixed in Boyer-Moore bm() in sgrep.c
[new]   23.09.96 3.11	option -i0
[chg]            3.10	in AGREP.H, ISO_CHAR.H, AGREP.C
				handling of meta symbols
[fix]   22.09.96 3.09	in BITAP.C (type CHAR)
[new]			in AGREP.C (Grand Total)
[chg]TG 16.09.96 3.08	un-commenting code defined in OUTPUT_OVERFLOW

*/

#define AGREP_VERSION	"3.411/TG"

#define AGREP_OS "LINUX"

#ifdef __DOS
#define AGREP_OS	"DOS"
#endif

#ifdef __OS2
#define AGREP_OS	"OS/2"
#endif

#ifdef __RSX
#define AGREP_OS	"VPMI"
#endif

#ifdef _WIN32
#define AGREP_OS	"WIN32"
#endif

#define AGREP_DATE	__DATE__
