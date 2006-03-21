LIBS = -I ../../H -lfltk -L /usr/X11R6/lib -lX11 ../../libcsound.a -lsndfile -lpthread
CXX = c++

wins5:	main.cxx winsound.cxx winsound.h
	${CXX} -o wins5 -g main.cxx winsound.cxx ${LIBS}

winsound.cxx: winsound.fl
	fluid -c winsound.fl

winsound.h: winsound.fl
	fluid -c winsound.fl

