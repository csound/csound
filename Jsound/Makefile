SHELL = /bin/sh
CC = cc
CFLAGS = -c -g

SRCS = jsnd5.tab.c lex.yy.c symbtab.c main.c semantics.c generate.c
OBJS = jsnd5.tab.o lex.yy.o symbtab.o main.o semantics.o generate.o


jsnd:	$(OBJS)
	$(CC) -o jsnd $(OBJS) -lm

jsnd5.tab.c:	jsnd5.y
	bison -d -t jsnd5.y

lex.yy.c:	jsnd5.lex
	flex -d jsnd5.lex
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

####################################################
