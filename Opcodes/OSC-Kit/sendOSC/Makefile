LIBOSCDIR = ..
LIBOSC = -lOSC
CFLAGS=  -O2 -I$(LIBOSCDIR)

sendOSC: sendOSC.o htmsocket.o ${LIBOSC}
	${CC} -o sendOSC sendOSC.o htmsocket.o -L $(LIBOSCDIR) ${LIBOSC}

${LIBOSC}:
	echo "You need to go to " ${LIBOSCDIR} " and do a make."

clean:
	rm -f sendOSC *.o
