SHELL = /bin/sh
CC = cc
CFLAGS = -c -g -D__BUILDING_LIBCSOUND -DUSE_DOUBLE

SRCS = csound_orcparse.c csound_orclex.c symbtab.c main.c semantics.c generate.c
OBJS = csound_orcparse.o csound_orclex.o symbtab.o main.o semantics.o generate.o

all:	jsnd

jsnd:	$(OBJS)
	$(CC) -o jsnd $(OBJS) -lm

csound_orcparse.c:	jsnd5.y
	bison -d -p csound_orc -v -t  -o csound_orcparse.c jsnd5.y
	#bison -d -v -t jsnd5.y

csound_orclex.c:	jsnd5.lex
	flex -d -ocsound_orclex.c -Pcsound_orc  jsnd5.lex 
	#flex -d jsnd5.lex
#	flex -d jsnd.lex

y.tab.o:	y.tab.c
	$(CC) $(CFLAGS) $*.c

lex.yy.o:	lex.yy.c
	$(CC) $(CFLAGS) $*.c

.c.o:	
	$(CC) $(CFLAGS) $*.c

depend:
	$(CC) -M $(SRCS) > deps
	cat Makefile deps > makefile

clean:
	-rm *.o
	-rm csound_orcparse.c
	-rm csound_orcparse.h
	-rm csound_orclex.c
	-rm jsnd

####################################################
