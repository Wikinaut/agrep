CFLAGS	      = -O

PROG	      = agrep
HDRS	      =	agrep.h checkfile.h re.h
OBJS	      =	\
		asearch.o	\
		asearch1.o	\
		bitap.o		\
		checkfile.o	\
		compat.o	\
		follow.o	\
		main.o		\
		maskgen.o	\
		parse.o		\
		preprocess.o	\
		sgrep.o		\
		mgrep.o		\
		utilities.o

$(PROG):	$(OBJS)
		$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
		-rm -f $(OBJS) core a.out

asearch.o:	agrep.h
asearch1.o:	agrep.h
bitap.o:	agrep.h
checkfile.o:	checkfile.h
follow.o:	re.h
main.o:		agrep.h checkfile.h
maskgen.o:	agrep.h
next.o:		agrep.h
parse.o:	re.h
preprocess.o:	agrep.h
sgrep.o:	agrep.h
abm.o:		agrep.h
utilities.o:	re.h
