#							SORTEX makef
# macros defined in toplevel csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    =   -L/usr/X11R6/lib -lm -lpthread -lfltk -lX11 -lportaudio -lasound -ldl -lc 

T = ../..

SCSORT =  smain.o $T/scsort.o $T/sread.o \
	  $T/sort.o $T/twarp.o $T/swrite.o \
	  $T/memalloc.o MOD.o $T/getstring.o
EXTRACT = xmain.o $T/scxtract.o $T/sread.o \
	  $T/extract.o $T/twarp.o $T/swrite.o \
	  $T/memalloc.o MOD.o $T/getstring.o

PROGS = scsort extract

sortexs:	$(PROGS)

scsort: 	$(SCSORT)
		gcc $(CFLAGS) -o scsort $(SCSORT) $(LIBS)

extract: 	$(EXTRACT)
		gcc $(CFLAGS) -o extract $(EXTRACT) $(LIBS)

smain.o:	smain.c
		gcc -I ../../H $(CFLAGS) -c smain.c

xmain.o:	xmain.c
		gcc -I ../../H $(CFLAGS) -c xmain.c

MOD.o:   MOD.c
		gcc -I ../../H $(CFLAGS) -c MOD.c

install: 	$(PROGS)
		-strip $(PROGS)
		-rm -f $(DEST)/scsort $(DEST)/extract
		cp $(PROGS) $(DEST)

clean:
		-rm smain.o xmain.o $(PROGS)

rmbak:
		-rm *.bak *~
