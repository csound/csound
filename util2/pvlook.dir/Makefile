#								scale Makef
# macros passed from csound/Makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    =   -L/usr/X11R6/lib -lm -lpthread -lfltk -lX11 -lportaudio -lasound -ldl -lc 

T = ../..

INC = ../../H

all:	pvlook

pvlook:		pvl_main.c $T/pvlook.o
		gcc -I $(INC) $(CFLAGS) pvl_main.c $T/Top/pvlook.c -lm -o pvlook

install:	pvlook
		-strip pvlook
		rm -f $(DEST)/pvlook
		cp pvlook $(DEST)

clean:		rmbak
		-rm pvlook
		-rm *.o

rmbak:
		-rm *.bak *~
