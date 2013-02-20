#
#	agrep
#
#	Makefile for the LINUX version of AGREP
#
#	Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved.
#

CC		= gcc -ansi -O3

# ---------------------------------------------------------------------
# Define HAVE_DIRENT_H to be 1 when you don't have <sys/dir.h>
# else define it to be 0 (in this case, one of the other 3 flags
# may need to be defined to be 1).
# ---------------------------------------------------------------------

HAVE_DIRENT_H   = 1
HAVE_SYS_DIR_H	= 0
HAVE_SYS_NDIR_H	= 0
HAVE_NDIR_H	= 0

# ---------------------------------------------------------------------
# Define UTIME to be 1 if you have the utime() routine on your system.
# Else define it to be 0.
# ---------------------------------------------------------------------

UTIME = 1

# ---------------------------------------------------------------------
# Define codepage_SET to be 1 if you want to use the international
# 8bit character set. Else define it to be 0.
# ---------------------------------------------------------------------
# This switch has not been introduced by me ! [TG] 05.10.96

ISO_CHAR_SET	= 1

OPTIMIZEFLAGS	= -O3
DEFINEFLAGS	= -DHAVE_DIRENT_H=$(HAVE_DIRENT_H)     \
		  -DHAVE_SYS_DIR_H=$(HAVE_SYS_DIR_H)   \
		  -DHAVE_SYS_NDIR_H=$(HAVE_SYS_NDIR_H) \
		  -DHAVE_NDIR_H=$(HAVE_NDIR_H)         \
		  -DUTIME=$(UTIME)                     \
		  -DISO_CHAR_SET=$(ISO_CHAR_SET)       \
		  -DS_IFLNK=-1                         \
		  -Dlstat=stat
SUBDIRCFLAGS	= -c $(DEFINEFLAGS) $(OPTIMIZEFLAGS)
MYDEFINEFLAGS	= -DMEASURE_TIMES=0  \
		  -DAGREP_POINTER=1  \
		  -DDOTCOMPRESSED=0
CFLAGS		= $(MYDEFINEFLAGS) $(SUBDIRCFLAGS)
OTHERLIBS	=

PROG	      = agrep

HDRS	      =	agrep.h checkfil.h re.h defs.h config.h codepage.h version.h

OBJS	      =	follow.o	\
		asearch.o	\
		asearch1.o	\
		agrep.o		\
		bitap.o		\
		checkfil.o	\
		compat.o	\
		dummyfil.o	\
		main.o		\
		maskgen.o	\
		parse.o		\
		checksg.o	\
		preproce.o	\
		delim.o		\
		asplit.o	\
		recursiv.o	\
		sgrep.o		\
		newmgrep.o	\
		utilitie.o	\
		codepage.o	\
		agrephlp.o

$(PROG):	$(OBJS) $(HDRS)
		  $(CC) -o $(PROG) $(OBJS)

clean:
		rm -f *.o
		rm -f $(PROG)

#	The header file config.h should be visible in the whole source code
#	Apparently, it is not at the moment.	[TG] 28.09.96

compat.o:	agrep.h defs.h config.h
asearch.o:	agrep.h defs.h config.h
asearch1.o:	agrep.h defs.h config.h
bitap.o:	agrep.h defs.h config.h codepage.h
checkfil.o:	agrep.h checkfil.h defs.h config.h
follow.o:	re.h agrep.h defs.h config.h
main.o:		agrep.h checkfil.h defs.h config.h
agrep.o:	agrep.h checkfil.h defs.h config.h version.h codepage.h
agrephlp.o:	version.h config.h
newmgrep.o:	agrep.h defs.h config.h codepage.h
maskgen.o:	agrep.h defs.h config.h codepage.h
next.o:		agrep.h defs.h config.h
parse.o:	re.h agrep.h defs.h config.h
preproce.o:	agrep.h defs.h config.h
checksg.o:	agrep.h checkfil.h defs.h config.h
delim.o:	agrep.h defs.h config.h
asplit.o:	agrep.h defs.h config.h
sgrep.o:	agrep.h defs.h config.h codepage.h
abm.o:		agrep.h defs.h config.h
utilitie.o:	re.h agrep.h defs.h config.h
dummyfil.o:	config.h
codepage.o:	codepage.h config.h agrep.h
