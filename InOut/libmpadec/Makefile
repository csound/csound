
CC=gcc
CFLAGS=-c -O3 -I.. -I../include -fPIC

OBJS=layer1.o layer2.o layer3.o synth.o tables.o mpadec.o mp3dec.o

all:	libmpadec.a libmpadec.so

libmpadec.a: $(OBJS)
	ar cr libmpadec.a $(OBJS)
	ranlib libmpadec.a

libmpadec.so: $(OBJS)
	$(CC) $(OBJS) -shared -o libmpadec.so -lm

%.o: %.c
	$(CC) $(CFLAGS) $*.c

clean:
	-@rm *.o
	-@rm *.a
	-@rm *.so
