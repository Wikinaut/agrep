# Microsoft Developer Studio Generated NMAKE File, Based on Agrep.dsp
!IF "$(CFG)" == ""
CFG=Agrep - Win32 Release
!MESSAGE No configuration specified. Defaulting to Agrep - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Agrep - Win32 Release" && "$(CFG)" != "Agrep - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Agrep.mak" CFG="Agrep - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Agrep - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Agrep - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Agrep - Win32 Release"

OUTDIR=.
INTDIR=.
# Begin Custom Macros
OutDir=.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Agrep.exe"

!ELSE 

ALL : "$(OUTDIR)\Agrep.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\agrep.obj"
	-@erase "$(INTDIR)\agrephlp.obj"
	-@erase "$(INTDIR)\asearch.obj"
	-@erase "$(INTDIR)\asearch1.obj"
	-@erase "$(INTDIR)\asplit.obj"
	-@erase "$(INTDIR)\bitap.obj"
	-@erase "$(INTDIR)\checkfil.obj"
	-@erase "$(INTDIR)\checksg.obj"
	-@erase "$(INTDIR)\codepage.obj"
	-@erase "$(INTDIR)\compat.obj"
	-@erase "$(INTDIR)\delim.obj"
	-@erase "$(INTDIR)\dummyfil.obj"
	-@erase "$(INTDIR)\follow.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\maskgen.obj"
	-@erase "$(INTDIR)\newmgrep.obj"
	-@erase "$(INTDIR)\ntdirent.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\preproce.obj"
	-@erase "$(INTDIR)\recursiv.obj"
	-@erase "$(INTDIR)\sgrep.obj"
	-@erase "$(INTDIR)\utilitie.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\Agrep.exe"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "_MBCS" /D AGREP_POINTER=1 /D HAVE_DIRENT_H=1 /Fp"$(INTDIR)\Agrep.pch" /YX /FD\
 /c 
CPP_OBJS=.
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Agrep.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=setargv.obj /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\Agrep.pdb" /machine:I386 /out:"$(OUTDIR)\Agrep.exe" 
LINK32_OBJS= \
	"$(INTDIR)\agrep.obj" \
	"$(INTDIR)\agrephlp.obj" \
	"$(INTDIR)\asearch.obj" \
	"$(INTDIR)\asearch1.obj" \
	"$(INTDIR)\asplit.obj" \
	"$(INTDIR)\bitap.obj" \
	"$(INTDIR)\checkfil.obj" \
	"$(INTDIR)\checksg.obj" \
	"$(INTDIR)\codepage.obj" \
	"$(INTDIR)\compat.obj" \
	"$(INTDIR)\delim.obj" \
	"$(INTDIR)\dummyfil.obj" \
	"$(INTDIR)\follow.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\maskgen.obj" \
	"$(INTDIR)\newmgrep.obj" \
	"$(INTDIR)\ntdirent.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\preproce.obj" \
	"$(INTDIR)\recursiv.obj" \
	"$(INTDIR)\sgrep.obj" \
	"$(INTDIR)\utilitie.obj"

"$(OUTDIR)\Agrep.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

OUTDIR=.
INTDIR=.
# Begin Custom Macros
OutDir=.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Agrep.exe"

!ELSE 

ALL : "$(OUTDIR)\Agrep.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\agrep.obj"
	-@erase "$(INTDIR)\agrephlp.obj"
	-@erase "$(INTDIR)\asearch.obj"
	-@erase "$(INTDIR)\asearch1.obj"
	-@erase "$(INTDIR)\asplit.obj"
	-@erase "$(INTDIR)\bitap.obj"
	-@erase "$(INTDIR)\checkfil.obj"
	-@erase "$(INTDIR)\checksg.obj"
	-@erase "$(INTDIR)\codepage.obj"
	-@erase "$(INTDIR)\compat.obj"
	-@erase "$(INTDIR)\delim.obj"
	-@erase "$(INTDIR)\dummyfil.obj"
	-@erase "$(INTDIR)\follow.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\maskgen.obj"
	-@erase "$(INTDIR)\newmgrep.obj"
	-@erase "$(INTDIR)\ntdirent.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\preproce.obj"
	-@erase "$(INTDIR)\recursiv.obj"
	-@erase "$(INTDIR)\sgrep.obj"
	-@erase "$(INTDIR)\utilitie.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Agrep.exe"
	-@erase "$(OUTDIR)\Agrep.ilk"
	-@erase "$(OUTDIR)\Agrep.pdb"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE"\
 /D "_MBCS" /D AGREP_POINTER=1 /D HAVE_DIRENT_H=1 /Fp"$(INTDIR)\Agrep.pch" /YX\
 /FD /c 
CPP_OBJS=.
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Agrep.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=setargv.obj /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\Agrep.pdb" /debug /machine:I386 /out:"$(OUTDIR)\Agrep.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\agrep.obj" \
	"$(INTDIR)\agrephlp.obj" \
	"$(INTDIR)\asearch.obj" \
	"$(INTDIR)\asearch1.obj" \
	"$(INTDIR)\asplit.obj" \
	"$(INTDIR)\bitap.obj" \
	"$(INTDIR)\checkfil.obj" \
	"$(INTDIR)\checksg.obj" \
	"$(INTDIR)\codepage.obj" \
	"$(INTDIR)\compat.obj" \
	"$(INTDIR)\delim.obj" \
	"$(INTDIR)\dummyfil.obj" \
	"$(INTDIR)\follow.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\maskgen.obj" \
	"$(INTDIR)\newmgrep.obj" \
	"$(INTDIR)\ntdirent.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\preproce.obj" \
	"$(INTDIR)\recursiv.obj" \
	"$(INTDIR)\sgrep.obj" \
	"$(INTDIR)\utilitie.obj"

"$(OUTDIR)\Agrep.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "Agrep - Win32 Release" || "$(CFG)" == "Agrep - Win32 Debug"
SOURCE=.\agrep.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_AGREP=\
	".\agrep.h"\
	".\checkfil.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	".\version.h"\
	

"$(INTDIR)\agrep.obj" : $(SOURCE) $(DEP_CPP_AGREP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_AGREP=\
	".\agrep.h"\
	".\checkfil.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	".\version.h"\
	

"$(INTDIR)\agrep.obj" : $(SOURCE) $(DEP_CPP_AGREP) "$(INTDIR)"


!ENDIF 

SOURCE=.\agrephlp.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_AGREPH=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	".\version.h"\
	

"$(INTDIR)\agrephlp.obj" : $(SOURCE) $(DEP_CPP_AGREPH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_AGREPH=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	".\version.h"\
	

"$(INTDIR)\agrephlp.obj" : $(SOURCE) $(DEP_CPP_AGREPH) "$(INTDIR)"


!ENDIF 

SOURCE=.\asearch.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_ASEAR=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\asearch.obj" : $(SOURCE) $(DEP_CPP_ASEAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_ASEAR=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\asearch.obj" : $(SOURCE) $(DEP_CPP_ASEAR) "$(INTDIR)"


!ENDIF 

SOURCE=.\asearch1.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_ASEARC=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\asearch1.obj" : $(SOURCE) $(DEP_CPP_ASEARC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_ASEARC=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\asearch1.obj" : $(SOURCE) $(DEP_CPP_ASEARC) "$(INTDIR)"


!ENDIF 

SOURCE=.\asplit.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_ASPLI=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\putils.c"\
	".\re.h"\
	

"$(INTDIR)\asplit.obj" : $(SOURCE) $(DEP_CPP_ASPLI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_ASPLI=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\putils.c"\
	".\re.h"\
	

"$(INTDIR)\asplit.obj" : $(SOURCE) $(DEP_CPP_ASPLI) "$(INTDIR)"


!ENDIF 

SOURCE=.\bitap.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_BITAP=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\bitap.obj" : $(SOURCE) $(DEP_CPP_BITAP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_BITAP=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\bitap.obj" : $(SOURCE) $(DEP_CPP_BITAP) "$(INTDIR)"


!ENDIF 

SOURCE=.\checkfil.c
DEP_CPP_CHECK=\
	".\checkfil.h"\
	".\config.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\checkfil.obj" : $(SOURCE) $(DEP_CPP_CHECK) "$(INTDIR)"


SOURCE=.\checksg.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_CHECKS=\
	".\agrep.h"\
	".\checkfil.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\checksg.obj" : $(SOURCE) $(DEP_CPP_CHECKS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_CHECKS=\
	".\agrep.h"\
	".\checkfil.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\checksg.obj" : $(SOURCE) $(DEP_CPP_CHECKS) "$(INTDIR)"


!ENDIF 

SOURCE=.\codepage.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_CODEP=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\codepage.obj" : $(SOURCE) $(DEP_CPP_CODEP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_CODEP=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\codepage.obj" : $(SOURCE) $(DEP_CPP_CODEP) "$(INTDIR)"


!ENDIF 

SOURCE=.\compat.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_COMPA=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\compat.obj" : $(SOURCE) $(DEP_CPP_COMPA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_COMPA=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\compat.obj" : $(SOURCE) $(DEP_CPP_COMPA) "$(INTDIR)"


!ENDIF 

SOURCE=.\delim.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_DELIM=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\delim.obj" : $(SOURCE) $(DEP_CPP_DELIM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_DELIM=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\delim.obj" : $(SOURCE) $(DEP_CPP_DELIM) "$(INTDIR)"


!ENDIF 

SOURCE=.\dummyfil.c

"$(INTDIR)\dummyfil.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\follow.c
DEP_CPP_FOLLO=\
	".\re.h"\
	

"$(INTDIR)\follow.obj" : $(SOURCE) $(DEP_CPP_FOLLO) "$(INTDIR)"


SOURCE=.\main.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_MAIN_=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_MAIN_=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

SOURCE=.\maskgen.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_MASKG=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\maskgen.obj" : $(SOURCE) $(DEP_CPP_MASKG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_MASKG=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\maskgen.obj" : $(SOURCE) $(DEP_CPP_MASKG) "$(INTDIR)"


!ENDIF 

SOURCE=.\newmgrep.c
DEP_CPP_NEWMG=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\newmgrep.obj" : $(SOURCE) $(DEP_CPP_NEWMG) "$(INTDIR)"


SOURCE=.\ntdirent.c
DEP_CPP_NTDIR=\
	".\ntdirent.h"\
	

"$(INTDIR)\ntdirent.obj" : $(SOURCE) $(DEP_CPP_NTDIR) "$(INTDIR)"


SOURCE=.\parse.c
DEP_CPP_PARSE=\
	".\re.h"\
	

"$(INTDIR)\parse.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"


SOURCE=.\preproce.c

!IF  "$(CFG)" == "Agrep - Win32 Release"

DEP_CPP_PREPR=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\preproce.obj" : $(SOURCE) $(DEP_CPP_PREPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Agrep - Win32 Debug"

DEP_CPP_PREPR=\
	".\agrep.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	

"$(INTDIR)\preproce.obj" : $(SOURCE) $(DEP_CPP_PREPR) "$(INTDIR)"


!ENDIF 

SOURCE=.\recursiv.c
DEP_CPP_RECUR=\
	".\autoconf.h"\
	".\config.h"\
	".\ntdirent.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\recursiv.obj" : $(SOURCE) $(DEP_CPP_RECUR) "$(INTDIR)"


SOURCE=.\sgrep.c
DEP_CPP_SGREP=\
	".\agrep.h"\
	".\codepage.h"\
	".\config.h"\
	".\defs.h"\
	".\re.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	

"$(INTDIR)\sgrep.obj" : $(SOURCE) $(DEP_CPP_SGREP) "$(INTDIR)"


SOURCE=.\utilitie.c
DEP_CPP_UTILI=\
	".\re.h"\
	

"$(INTDIR)\utilitie.obj" : $(SOURCE) $(DEP_CPP_UTILI) "$(INTDIR)"



!ENDIF 

