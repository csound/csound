o = o
CC = gcc
DEFINES = -DBETA
CFLAGS = -g -O2 -Wall -malign-loops=4 -malign-jumps=4 -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87 $(RPM_OPT_FLAGS) $(DEFINES)
INCLUDE	= ${prefix}/include
P = ../Plugins

LIBRARIES = \
	babo.so		\
	bbcut.so	\
	biquad.so	\
	bowedbar.so 	\
	butter.so  	\
	clfilt.so  	\
	cross2.so	\
	dam.so		\
	filter.so	\
	flanger.so	\
	follow.so	\
	grain.so	\
	grain4.so	\
	hrtferX.so	\
	locsig.so	\
	lowpassr.so	\
	midiops2.so	\
	midiops3.so	\
	modal4.so	\
	nlfilt.so	\
	oscbnk.so	\
	phisem.so	\
	physmod.so	\
	pitch.so	\
	pluck.so	\
	repluck.so	\
	scansyn.so	\
	scansynx.so	\
	sfont.so	\
	sndwarp.so	\
	space.so 	\
	spat3d.so 	\
	spectra.so 	\
	ugens6.c	\
	ugens7.c	\
	ugens9.c	\
	ugensa.so	\
	uggab.so	\
	ugmoss.so	\
	ugrw1.so	\
	ugrw2.so	\
	ugsc.so		\
	vdelay.so	\
	wave-terrain.so


LSRC = \
	babo.c		\
	bbcut.c		\
	biquad.c	\
	bowedbar.c	\
	butter.c	\
	clfilt.c	\
	cross2.c	\
	dam.c		\
	filter.c	\
	flanger.c	\
	follow.c	\
	grain.c		\
	grain4.c	\
	hrtferX.c	\
	locsig.c	\
	lowpassr.c	\
	midiops2.c	\
	midiops3.c	\
	modal4.c	\
	oscbnk.c	\
	nlfilt.c 	\
	phisem.c	\
	physmod.c	\
	physutil.c	\
	pitch.c		\
	pluck.c		\
	repluck.c	\
	scansyn.c 	\
	scansynx.c 	\
	sfont.c		\
	sndwarp.c	\
	space.c 	\
	spat3d.c 	\
	spectra.c 	\
	ugens6.c	\
	ugens7.c	\
	ugens9.c	\
	ugensa.c	\
	uggab.c		\
	ugmoss.c	\
	ugrw1.c		\
	ugrw2.c		\
	ugsc.c		\
	vdelay.c	\
	wave-terrain.c


all:	$(LIBRARIES)

opcodes: all

PHYSOBJ = physmod.$(o) physutil.$(o) mandolin.$(o) singwave.$(o) \
	fm4op.$(o) moog1.$(o) shaker.$(o)

physmod.so:	$(PHYSOBJ)
	ld -E --shared $(PHYSOBJ) -o physmod.so
	-rm $P/physmod.so
	ln physmod.so $P

ugrw1.so:	ugrw1.$(o) fprint.$(o)
	ld -E --shared ugrw1.$(o) fprint.$(o) -o physmod.so
	-rm $P/ugrw1.so
	ln ugrw1.so $P

%.so: %.$(o)
	ld -E --shared $*.$(o) -o $*.so
	-rm $P/$*.so
	ln $*.so $P

%.$(o): %.c
	$(CC) -I .. -I ../H $(CFLAGS) -c $*.c -o $*.$(o)

TAGS:
	etags -a -o ../TAGS $(LSRC)

depend:
	gcc -M -I ../H $(CFLAGS) $(LSRC) > deps
	cat Makefile deps > makefile

# DO NOT DELETE THIS LINE
babo.o: babo.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/babo.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
bbcut.o: bbcut.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/bbcut.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
biquad.o: biquad.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/biquad.h
bowedbar.o: bowedbar.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/bowedbar.h \
  ../H/physutil.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/bowed.h
butter.o: butter.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/butter.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
clfilt.o: clfilt.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/clfilt.h
cross2.o: cross2.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ptrigtbl.h \
  ../H/fhtfun.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
dam.o: dam.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/dam.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
filter.o: filter.c /usr/include/stdlib.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h ../H/csdl.h \
  ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/filter.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
flanger.o: flanger.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/flanger.h
follow.o: follow.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/follow.h
grain.o: grain.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/grain.h
grain4.o: grain4.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/grain4.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
hrtferX.o: hrtferX.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/hrtferx.h ../H/3Dug.h
locsig.o: locsig.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/locsig.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
lowpassr.o: lowpassr.c /usr/include/math.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h /usr/include/bits/mathinline.h \
  ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/lowpassr.h
midiops2.o: midiops2.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h
midiops3.o: midiops3.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/midiops.h \
  ../H/midiops3.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
modal4.o: modal4.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/modal4.h \
  ../H/physutil.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/marimba.h ../H/vibraphn.h
oscbnk.o: oscbnk.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/fft.h ../H/oscbnk.h
nlfilt.o: nlfilt.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/nlfilt.h
phisem.o: phisem.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/phisem.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
physmod.o: physmod.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/clarinet.h \
  ../H/physutil.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/flute.h ../H/bowed.h ../H/brass.h \
  ../H/mandolin.h ../H/singwave.h ../H/moog1.h ../H/fm4op.h ../H/shaker.h
physutil.o: physutil.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/physutil.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
pitch.o: pitch.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h ../H/spectra.h ../H/pitch.h ../H/uggab.h \
  ../H/namedins.h /usr/include/sys/time.h /usr/include/sys/resource.h \
  /usr/include/bits/resource.h ../H/ugens2.h /usr/include/termios.h \
  /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
pluck.o: pluck.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/wavegde.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/pluck.h
repluck.o: repluck.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/repluck.h
scansyn.o: scansyn.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/scansyn.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
scansynx.o: scansynx.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/scansynx.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
sfont.o: sfont.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h /usr/include/bits/mathinline.h \
  /usr/include/ctype.h ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/sfenum.h \
  ../H/sfont.h ../H/sftype.h ../H/sf.h /usr/include/errno.h \
  /usr/include/bits/errno.h /usr/include/linux/errno.h \
  /usr/include/asm/errno.h
sndwarp.o: sndwarp.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/sndwarp.h
space.o: space.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/space.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
spat3d.o: spat3d.c /usr/include/stdlib.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ftgen.h \
  ../H/spat3d.h
spectra.o: spectra.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/spectra.h
ugens6.o: ugens6.c ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugens6.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
ugens7.o: ugens7.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugens7.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
ugens9.o: ugens9.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/fft.h ../H/dsputil.h \
  ../H/convolve.h ../H/ugens9.h ../H/soundio.h ../H/oload.h
ugensa.o: ugensa.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugensa.h \
  ../H/ugens7.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
uggab.o: uggab.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/uggab.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
ugmoss.o: ugmoss.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugmoss.h \
  ../H/aops.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
ugrw1.o: ugrw1.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugrw1.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h /usr/include/ctype.h ../H/fout.h
ugrw2.o: ugrw2.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugrw2.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
ugsc.o: ugsc.c /usr/include/math.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h /usr/include/bits/mathinline.h \
  ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h ../H/ugsc.h
vdelay.o: vdelay.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h ../H/vdelay.h ../H/revsets.h
wave-terrain.o: wave-terrain.c ../H/csdl.h ../H/cs.h ../H/config.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/stdio.h \
  /usr/include/setjmp.h /usr/include/bits/setjmp.h /usr/include/string.h \
  /usr/include/bits/string.h /usr/include/bits/string2.h ../H/sysdep.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/sys/file.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../H/cwindow.h ../H/opcode.h ../H/version.h \
  ../H/sort.h ../H/midiops2.h ../H/text.h ../H/prototyp.h \
  ../H/wave-terrain.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/include/bits/mathinline.h
