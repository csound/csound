#								SCOT makef
# macros defined in toplevel csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    =   -L/usr/X11R6/lib -lm -lpthread -lfltk -lX11 -lportaudio -lasound -ldl -lc 

T = ../..

OBJS =  scot_main.o $T/scot.o $T/getstring.o

scots:  scot

scot:	$(OBJS)
	gcc -o scot $(OBJS)

scot_main.o:		scot_main.c
	gcc -I ../../H $(CFLAGS) -c scot_main.c

install:	scot
	-strip scot
	-rm -f $(DEST)/scot
	cp scot $(DEST)

clean:
	-rm scot *.o

rmbak:
	-rm *.bak *~
