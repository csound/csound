#								PVOC makef
# macros defined in toplevel csound/makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

T = ../..
INC = ../../H

OBJS =  pvc_main.o $T/pvanal.o $T/pvxanal.o \
	$T/pvfileio.o $T/filopen.o \
        $T/sfheader.o $T/mxfft.o \
	$T/soundin.o $T/ulaw.o $T/aiff.o \
	$T/wave.o $T/ieee80.o $T/memalloc.o \
	$T/pvoc.o $T/fft.o $T/dsputil.o \
	$T/window.o $T/winFLTK.o $T/winascii.o \
	$T/winEPS.o $T/aifc.o $T/getstring.o \
	$T/FL_graph.o

pvocs:	pvanal

pvanal:	$(OBJS)
	gcc $(CFLAGS) -o pvanal $(OBJS) $(LIBS)

pvc_main.o:	pvc_main.c $(INC)/ustub.h $(INC)/cs.h
	gcc -I $(INC) $(CFLAGS) -c pvc_main.c

install: pvanal
	-strip pvanal
	-rm -f $(DEST)/pvanal
	cp pvanal $(DEST)

clean:
	-rm -f pvanal pvc_main.o

rmbak:
	-rm *.bak *~
