#							CSCORE makef
# macros defined in toplevel csound/Makefile
# 	CFLAGS INCLUDE LIB DEST

INCLUDE	= ${prefix}/include
LIB	= ${prefix}/share
DEST	= ${exec_prefix}/bin
CFLAGS  = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(DEFINES)
LIBS    =   -L/usr/X11R6/lib -lm -lpthread -lfltk -lX11 -lportaudio -lasound -ldl -lc 

T = ../..

OBJS = $T/cscormai.o $T/cscorfns.o $T/rdscor.o \
	$T/memalloc.o $T/getstring.o $T/natben.o

I = $(INCLUDE)

includes:	$I/cscore.h

$I/cscore.h:	$T/cscore.h
		-mkdir -p $I
		cp $T/cscore.h $I

cscore.a:	$(OBJS)
		-rm cscore.a
		ar q cscore.a $(OBJS)
		-ranlib cscore.a

cscores:	cscore.a

$T/cscormai.o:	
		(cd ../..; make cscormai.o )

$T/rdscor.o:
		(cd ../..; make rdscor.o )

$T/memalloc.o:
		(cd ../..; make memalloc.o )


install:	includes cscore.a
		-rm $(LIB)/libcscore.a
		cp cscore.a $(LIB)/libcscore.a

clean:
		-rm cscore.a

rmbak:
		-rm *.bak *~


