#								scale Makef
# macros passed from csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

T = ../..

INC = ../..

OBJS =	$T/sndinfo.$OBJEXT$ $T/filopen.$OBJEXT$ $T/sfheader.$OBJEXT$ \
	$T/soundin.$OBJEXT$ $T/memalloc.$OBJEXT$ $T/wave.$OBJEXT$ \
	$T/aiff.$OBJEXT$ $T/aifc.$OBJEXT$ $T/ieee80.$OBJEXT$ \
	$T/ulaw.$OBJEXT$ $T/getstring.$OBJEXT$

envext:		envext.$OBJEXT$ $(OBJS)
		gcc $(CFLAGS) envext.$OBJEXT$ $(OBJS) -lm -o envext

envext.$OBJEXT$:	envext.c ../../cs.h ../../ustub.h ../../soundio.h
		gcc -I ../../H $(CFLAGS) -I$(INC) -c envext.c

install:	envext
		-strip envext
		rm -f $(DEST)/envext
		cp envext $(DEST)

clean:		rmbak
		-rm envext

rmbak:
		-rm *.bak *~
