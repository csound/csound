all:	beats

beats:	beats.tab.o lex.yy.o main.o
	gcc -g -o beats lex.yy.o beats.tab.o main.o -lm

lex.yy.c:	beats.l beats.tab.h
	flex beats.l

lex.yy.o:	lex.yy.c beats.tab.h
	gcc -g -c lex.yy.c

beats.tab.c:	beats.y
	bison -d beats.y

beats.tab.o:	beats.tab.c
	gcc -g -c beats.tab.c

main.o:	main.c
	gcc -g -c main.c

clean:
	rm main.o beats.tab.o beats.tab.c beats.tab.h lex.yy.o lex.yy.c beats

dist:
	zip beats.zip  beats.l  beats.y main.c Makefile test.xs
