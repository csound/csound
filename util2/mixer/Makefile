#								mixer Makef
# macros passed from csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

OBJS =	$T/sndinfo.o $T/filopen.o $T/sfheader.o \
	$T/soundin.o $T/ulaw.o $T/aiff.o \
	$T/wave.o $T/ieee80.o $T/memalloc.o $T/aifc.o \
        $T/getstring.o

T = ../..

INC = ../../H

all:	extractor mixers

mixers:		mixer


mixer:		mixer.o $(OBJS)
		gcc $(CFLAGS) mixer.o $(OBJS) -lm -o mixer

extractor:	xtrct.o $(OBJS)
		gcc $(CFLAGS) xtrct.o $(OBJS) -lm -o extractor

mixer.o:	mixer.c $(INC)/ustub.h $(INC)/cs.h $(INC)/soundio.h
		gcc -c $(CFLAGS) -I$(INC) mixer.c

xtrct.o:	xtrct.c $(INC)/ustub.h $(INC)/cs.h $(INC)/soundio.h
		gcc -c $(CFLAGS) -I$(INC) xtrct.c

install:	mixer extractor
		-strip mixer
		rm -f $(DEST)/mixer
		cp mixer $(DEST)
		-strip extractor
		rm -f $(DEST)/extractor
		cp extractor $(DEST)

clean:		rmbak
		-rm mixer
		-rm extractor
		-rm *.o

rmbak:
		-rm *.bak *~
