#								SNDINFO makef
# macros defined in toplevel csound/makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

T = ../..
INC = ../../H

OBJS =  main.o $T/sndinfo.o $T/filopen.o \
	$T/sfheader.o $T/soundin.o $T/ulaw.o \
	$T/aiff.o $T/wave.o $T/ieee80.o \
	$T/memalloc.o $T/aifc.o $T/getstring.o

sndinfos:	sndinfo

sndinfo:	$(OBJS)
		gcc $(CFLAGS) -o sndinfo $(OBJS) -lm

main.o:		main.c $(INC)/ustub.h
		gcc -I $(INC) $(CFLAGS) -c main.c

install: 	sndinfo
		-strip sndinfo
		-rm -f $(DEST)/sndinfo
		cp sndinfo $(DEST)

clean:
		-rm -f sndinfo main.o

rmbak:
		-rm *.bak *~
