#								ADSYN Makef
# macros defined in toplevel csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

T = ../..
INC = ../../H

OBJS =  het_main.o $T/hetro.o $T/filopen.o \
	$T/sfheader.o $T/soundin.o $T/ulaw.o \
	$T/aiff.o $T/ieee80.o $T/memalloc.o \
	$T/wave.o $T/window.o $T/winFLTK.o \
	$T/FL_graph.o $T/winEPS.o $T/aifc.o \
	$T/winascii.o $T/sdif.o $T/sdif-mem.o \
	$T/getstring.o

adsyns:	hetro

hetro:	$(OBJS)
	gcc $(CFLAGS) -o hetro $(OBJS) $(LIBS)

install: hetro
	-strip hetro
	-rm -f $(DEST)/hetro
	cp hetro $(DEST)

het_main.o: $(INC)/ustub.h $(INC)/cs.h het_main.c
	gcc -I $(INC) $(CFLAGS) -c het_main.c

clean:
	-rm -f hetro

rmbak:
	-rm *.bak *~
