LIBOSCDIR = ..
LIBOSC = ${LIBOSCDIR}/libOSC.a
CFLAGS=  -O2 -I$(LIBOSCDIR)

DUMPOBJS=dumpOSC.o


dumpOSC: ${DUMPOBJS}
	${CC} -o $@ ${DUMPOBJS}

dumpUDP: dumpUDP.o
	${CC} -o dumpUDP dumpUDP.o

${LIBOSC}:
	echo "You need to go to " ${LIBOSCDIR} " and do a make."

clean:
	rm -f dumpOSC *.o
