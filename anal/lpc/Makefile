#								LPC makef
# macros defined in toplevel csound/makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    =   -L/usr/X11R6/lib -lm -lpthread -lfltk -lX11 -lportaudio -lasound -ldl -lc 

T = ../..
INC = ../../H

OBJS =  lpc_main.o $T/lpanal.o $T/lptrkfns.o \
	$T/filopen.o $T/sfheader.o $T/soundin.o \
	$T/ulaw.o $T/aiff.o $T/wave.o $T/ieee80.o \
        $T/memalloc.o $T/window.o $T/winFLTK.o \
	$T/winascii.o $T/winEPS.o $T/aifc.o \
	$T/getstring.o $T/FL_graph.o

lpcs:		lpanal

lpanal:		$(OBJS)
		gcc $(CFLAGS) -o lpanal $(OBJS) $(LIBS)

lpc_main.o:		lpc_main.c $(INC)/ustub.h $(INC)/cs.h
		gcc -I $(INC) $(CFLAGS) -c lpc_main.c

install:	lpanal
		-strip lpanal
		-rm -f $(DEST)/lpanal
		cp lpanal $(DEST)

clean:
		-rm -f lpanal lpc_main.o

rmbak:
		-rm *.bak *~
