#								SNDINFO makef
# macros defined in toplevel csound/makefile
#	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    = -L/usr/local/lib -lfltk -L/usr/X11R6/lib -lm -lXext -lX11 -lm -lpthread -lstdc++  -L/usr/X11R6/lib -lm -lpthread -lpthread -lX11 -lportaudio -lasound -ldl -lc 

T = ../..

POBJS =  pv_export.o $T/pvoc.o
HOBJS =  het_export.o $T/memfiles.o $T/memalloc.o \
	$T/filopen.o
LOBJS =  lpc_export.o
HIOBJS = het_import.o
LIOBJS = lpc_import.o

all:	pv_export het_export lpc_export pv_import het_import lpc_import

pv_export:	$(POBJS)
		gcc $(CFLAGS) -o pv_export $(POBJS) -lm

pv_export.o:	pv_export.c
		gcc -I ../../H -c $(CFLAGS) pv_export.c

het_export:	$(HOBJS)
		gcc $(CFLAGS) -o het_export $(HOBJS) -lm

het_export.o:	het_export.c
		gcc -I ../../H -c $(CFLAGS) het_export.c

lpc_export:	$(LOBJS)
		gcc $(CFLAGS) -o lpc_export $(LOBJS) -lm

lpc_export.o:	lpc_export.c
		gcc -I ../../H -c $(CFLAGS) lpc_export.c

pv_import:	pv_import.o
		gcc $(CFLAGS) -o pv_import pv_import.o -lm

pv_import.o:	pv_import.c
		gcc -I ../../H -c $(CFLAGS) pv_import.c

het_import:	$(HIOBJS)
		gcc $(CFLAGS) -o het_import $(HIOBJS) -lm

het_import.o:	het_import.c
		gcc -c $(CFLAGS) -I ../../H het_import.c

lpc_import:	$(LIOBJS)
		gcc $(CFLAGS) -o lpc_import $(LIOBJS) -lm

lpc_import.o:	lpc_import.c
		gcc -c $(CFLAGS) -I ../../H lpc_import.c


install: 	pv_export pv_import
		-strip pv_export
		-rm -f $(DEST)/pv_export
		cp pv_export $(DEST)

clean:
		-rm -f pv_export pv_export.o

rmbak:
		-rm *.bak *~
