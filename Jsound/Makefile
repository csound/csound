SHELL = /bin/sh
CC = cc
CFLAGS = -c -g

SRCS = jsnd.yacc.tab.c lex.yy.c symbtab.c main.c semantics.c generate.c
OBJS = jsnd.yacc.tab.o lex.yy.o symbtab.o main.o semantics.o generate.o


jsnd:	$(OBJS)
	$(CC) -o jsnd $(OBJS) -lm

jsnd.yacc.tab.c:	jsnd.yacc
	bison -d -t jsnd.yacc -o jsnd.yacc.tab.c

lex.yy.c:	jsnd5.lex
	flex jsnd5.lex
#	flex -d jsnd.lex

lex.yy.o:	lex.yy.c
	$(CC) $(CFLAGS) $*.c

.c.o:
	$(CC) $(CFLAGS) $*.c

depend:
	$(CC) -M $(SRCS) > deps
	cat Makefile deps > makefile

####################################################
