SRC = beats.tab.c lex.yy.c main.c
OBJ = beats.tab.o lex.yy.o main.o

all:	csbeats

csbeats:	$(OBJ)
	gcc -g -o csbeats $(OBJ) -lm

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
	rm -rf main.o beats.tab.o beats.tab.c beats.tab.h lex.yy.o lex.yy.c \
	beats makefile

dist:
	zip beats.zip  beats.l  beats.y main.c Makefile test.xs

makefile: Makefile $(SRC)
	gcc -M $(SRC) | cat Makefile - > makefile
