#								scale Makef
# macros passed from csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

OBJS =	$T/sndinfo.o $T/filopen.o $T/sfheader.o \
	$T/soundin.o $T/ulaw.o $T/aiff.o \
	$T/aifc.o $T/wave.o $T/ieee80.o \
        $T/memalloc.o $T/getstring.o

T = ../..

INC = ../../H

scale:	scale.o $(OBJS) 
	gcc $(CFLAGS) scale.o $(OBJS) -lm -o scale

scale.o:	scale.c $(INC)/ustub.h
	gcc $(CFLAGS) -c -I$(INC) scale.c

install:	scale
	-strip scale
	rm -f $(DEST)/scale
	cp scale $(DEST)

clean:		rmbak
	-rm scale *.o

rmbak:
	-rm *.bak *~
