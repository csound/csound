#								dnoise Makef

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    =   -L/usr/X11R6/lib -lm -lpthread -lfltk -lX11 -lportaudio -lasound -ldl -lc 

CDEP =
COBS =	$(COBJS)
LIBS = -lm

SHELL = /bin/sh
T = ../..

INC = ../..

DSRC =	dnoise.c fft.c

DOBJS =	$T/filopen.o $T/sfheader.o \
	$T/soundin.o $T/getstring.o $T/ulaw.o \
	$T/wave.o $T/aiff.o $T/ieee80.o \
	$T/memalloc.o $T/aifc.o fft.o dnoise.o 

SSRC =	srconv.c

SOBJS =	$T/filopen.o $T/sfheader.o \
	$T/soundin.o $T/getstring.o $T/ulaw.o \
	$T/wave.o $T/aiff.o $T/ieee80.o \
	$T/memalloc.o $T/aifc.o srconv.o

PROGS   = dnoise srconv

all:		$(PROGS)

dnoise:		$(DOBJS) 
		gcc $(CFLAGS) -o dnoise $(DOBJS) $(LIBS)

srconv:		$(SOBJS) 
		gcc $(CFLAGS) -o srconv $(SOBJS) $(LIBS)

.c.o:
	gcc -I ../../H $(CFLAGS) -c  -I$(INC) $*.c

install:	$(PROGS)
	-strip $(PROGS)
	-rm $(DEST)/dnoise
	-rm $(DEST)/srconv
	cp $(PROGS) $(DEST)

clean:
	-rm $(COBS)

rmbak:
	-rm *.bak *~

depend:
	gcc -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 -M $(DEFINES) $(CSRCS) > deps
	cat Makefile deps > makefile

# DO NOT DELETE THIS LINE
