#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Unless you are adding new library dependencies or new platforms to the Csound build,
# you should not need to modify this file.
#
# Csound always builds with libsndfile.
#
# Csound builds with libportaudio, if available; 
# otherwise there is no realtime audio.
#
# Csound builds with libfltk, if available; 
# otherwise there is no graphing of function tables, and no widget opcodes.
#
# Configuration choices determine the use of libportaudio and libfltk, 
# optional output soundfile formats, the use of Tcl/Tk, and whether
# to build CsoundVST or other frontends in addition to console Csound.

AC_PREREQ(2.57)
AC_INIT([Csound], [5.0])

# Information on the package
AC_CONFIG_SRCDIR([Top/argdecode.c])
AM_CONFIG_HEADER([H/config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CXX
dnl Add warning when using g++
if test "X$GXX" = Xyes ; then
  CXXFLAGS="$CXXFLAGS -Wall"
fi
AC_PROG_CC
dnl Add warning when using gcc
if test "X$GCC" = Xyes ; then
  CFLAGS="$CFLAGS -O2 -g -Wall"
fi
AM_PROG_CC_STDC

# Checks for libraries.

LDFLAGS="$LDFLAGS -L/usr/local/lib"

AC_CHECK_LIB([dl], [dlsym])
# AC_CHECK_LIB([midi], [midi_set_owner])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([sndfile], [sf_open])
AC_CHECK_PROG(HAVE_PYTHON, [python], [yes], [no]) 
AC_CHECK_PROG(HAVE_SWIG, [swig], [yes], [no]) 

# Checks for header files.

AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([OS.h arpa/inet.h fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h sgtty.h bsd/sgtty.h stddef.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h sdif.h portaudio.h sndfile.h portmidi.h mmsystem.h boost/numeric/ublas/blas.hpp python2.3/Python.h FL/Fl.H])

# Checks for typedefs, structures, and compiler characteristics.

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE
AC_C_BIGENDIAN

# Checks for library functions.

# AC_FUNC_CLOSEDIR_VOID
# AC_FUNC_ERROR_AT_LINE
# AC_FUNC_FORK
# AC_PROG_GCC_TRADITIONAL
# AC_FUNC_MALLOC
# AC_FUNC_MEMCMP
# AC_FUNC_REALLOC
# AC_FUNC_SELECT_ARGTYPES
# AC_FUNC_SETVBUF_REVERSED
# AC_TYPE_SIGNAL
# AC_FUNC_STAT
# AC_FUNC_STRTOD
# AC_FUNC_VPRINTF
# AC_CHECK_FUNCS([atexit dup2 gettimeofday memmove memset modf putenv rint select socket strchr strdup strerror strrchr strstr strtol])
AC_CHECK_FUNC(popen, AC_DEFINE([PIPES], 1, [Enable use of UNIX-style pipes]))

AC_DEFINE([CSOUND_WITH_API], 1, [Define when building Csound with the Csound API library])

AC_ARG_ENABLE([double],
	AC_HELP_STRING([--enable-double],
				   [enables the use of double precision floating point internally (default=no)]),
	[USE_DOUBLE=$enableval],
	[USE_DOUBLE=no])

if test "x$USE_DOUBLE" = xyes; then
  AC_DEFINE([USE_DOUBLE], 1, [Define when using double precision floating point])
fi

AC_ARG_WITH([portaudio],
    AC_HELP_STRING([--with-portaudio],
                   [enables the use of the PortAudio library for real-time audio (default=yes)]),
	[WITH_PORTAUDIO=$withval],
    [WITH_PORTAUDIO=yes])

AC_ARG_WITH(fltk,
	AC_HELP_STRING([--with-fltk],
				   [enables the use of the fltk windowing toolkit (http://www.fltk.org/) for graphing and widget opcodes (default=yes)]),
	[WITH_FLTK=$withval],
	[WITH_FLTK=yes])

AC_ARG_WITH(tcltk,
	AC_HELP_STRING([--with-tcltk],
				   [enables the use of Tcl/Tk ( default=yes)]),
	[WITH_TCLTK=$withval],
	[WITH_TCLTK=yes])

AC_ARG_ENABLE(sf-ircam,
	[AC_HELP_STRING([--enable-sf-ircam],
		[enable reading/writing IRCAM-format sound files, with 1024-byte header (default=yes)])],
	[USE_IRCAM=$enableval],
	[USE_IRCAM=yes])

AC_ARG_ENABLE(sf-sun41,
	AC_HELP_STRING([--enable-sf-sun41],
		[enable reading/writing SUN4.1-format soundfiles, with their own header (default=no)]),
	[USE_SUN41=$enableval],
	[USE_SUN41=no])

AC_ARG_ENABLE(sf-NeXT,
	AC_HELP_STRING([--enable-sf-NeXT],
		[enable reading/writing SUN4.1-format soundfiles, with their own header (default=no)]),
	[USE_NeXT=$enableval],
	[USE_NeXT=no])

AC_ARG_ENABLE(csoundvst,
	AC_HELP_STRING([--enable-csoundvst],
		[enable building CsoundVST, on Windows or Linux (default=no)]),
	[ENABLE_CSOUNDVST=$enableval],
	[ENABLE_CSOUNDVST=no])

#	Compiler options used for all compilation.

CPPFLAGS="$CPPFLAGS -DBETA -I$srcdir/H -I/usr/local/include -I/usr/local/boost -I/usr/include/Python2.3"

if test "x$GCC" = xyes; then
  case `$CC -dumpversion 2>/dev/null` in
    [[12]].*) 
      ALIGN_FLAGS="-malign-loops=4 -malign-jumps=4";;
    *) 
      ALIGN_FLAGS="-falign-loops=4 -falign-jumps=4";;
  esac
fi

SOUNDFILE_LDFLAGS="-L/usr/local/lib"
SOUNDFILE_LIBS="-lsndfile"

#	Now we adapt compiler options to the platform.

case $host in 
    *irix*) 
		SGI="yes"
	    AC_DEFINE([SGI], 1, [Build for SGI platform])
        AUDIO_LIBS="-lportaudio -lasound"
	    ;;
	*sun*)
		sun="yes"
	    AC_DEFINE([sun], 1, [Build for SunOS platform])
        AUDIO_LIBS="-lportaudio -lasound"
	    ;;
    *-*-msdos* | *-*-go32* | *-*-mingw32* | *-*-cygwin* | *-*-windows*)
		WIN32="yes"
	    AC_DEFINE([WIN32], 1, [Build for Win32 platform])
	    CFLAGS="$CFLAGS -D_WIN32"
	    CXXFLAGS="$CXXFLAGS -D_WIN32"
        AUDIO_LDFLAGS="-mconsole -L/usr/local/lib"
        AUDIO_LIBS="-lportaudio"
	    ;;
    *linux*)
		LINUX="yes"
        AC_DEFINE([LINUX], 1, [Build for Linux platform])
        AUDIO_LDFLAGS="-L/usr/local/lib"
        AUDIO_LIBS="-lportaudio -lasound"
	    if test "$WORDS_BIG_ENDIAN"; then
			AC_DEFINE([LINUX_BE], 1, [Build for Linux big-endian platform])
	    fi
	    ;;
    *dec*)
		DEC="yes"
        AC_DEFINE([DEC], 1, [Build for DEC platform])
        AUDIO_LIBS="-lportaudio -lasound"
	    ;;
	*hp*)
		HP="yes"
        AC_DEFINE([HP], 1, [Build for HP platform])
        AUDIO_LIBS="-lportaudio -lasound"
	    ;;
	darwin* | rhapsody*)
		CFLAGS="$CFLAGS -fpascal-strings -I/Developer/Headers/FlatCarbon"
		LDFLAGS="$LDFLAGS -framework CoreAudio"
		;;
     *)
      AC_MSG_WARN([Unable to automatically configure for your system])
      ;;
esac

if test "x$WITH_PORTAUDIO" = xyes; then
	RTAUDIO=yes
	if test "x$WIN32" = xyes; then
		AUDIO_LIBADD="$AUDIO_LIBS -lwinmm"
	else
		AUDIO_LIBADD="$AUDIO_LIBS"
	fi
	AC_DEFINE([RTAUDIO], 1, [Define to enable PortAudio for real-time audio])
else
	RTAUDIO=no
	if test "x$WIN32" = xyes; then
		AUDIO_LIBADD="-lwinmm"
	else
		AUDIO_LIBADD=""
	fi
fi

if test "x$WITH_FLTK" = xyes; then
	FLTK=yes
	AC_PATH_PROG(FLTK_CONFIG,fltk-config)
	AC_LANG_CPLUSPLUS
	AC_LANG_C
	WINLDFLAGS=`$FLTK_CONFIG --ldflags` 
	WINLDFLAGS="$WINLDFLAGS -lpthread -lstdc++"
	WINCXXFLAGS=`$FLTK_CONFIG --cxxflags`
	AC_DEFINE([USE_FLTK], 1, [Define to use FLTK])
	AC_DEFINE([WINDOWS], 1, [Define to use FLTK to graph function tables])
	AC_DEFINE([USE_FLTK_WIDGETS], 1, [Define to use FLTK widget opcodes])
fi

AUDIO_FORMATS=""
if test "$USE_IRCAM" = yes; then
   AC_DEFINE([IRCAM], 1, [enable IRCAM soundfile format])
   echo "Using ircam"
   AUDIO_FORMATS="$AUDIO_FORMATS IRCAM"
fi
if test "$USE_SUN41" = yes; then
   AC_DEFINE([SUN41], 1, [enable Sun 4.1 soundfile format])
   AUDIO_FORMATS="$AUDIO_FORMATS SUN41"
fi
if test "$USE_NeXT" = yes; then
   AC_DEFINE([NeXT], 1, [enable NeXT soundfile format])
   AUDIO_FORMATS="$AUDIO_FORMATS NeXT"
fi

if test "$WITH_TCLTK" = yes; then
   AC_CHECK_PROG(TCLTK,wish)
   AC_DEFINE([TCLTK], 1, [enable Tcl/Tk support])
fi

#	All build options for the Csound API library 
#	and also the convenience library for utility programs.

LIBCSOUND_CPPFLAGS="$CPPFLAGS"
AC_SUBST(LIBCSOUND_CPPFLAGS)
LIBCSOUND_CFLAGS="$ALIGN_FLAGS $CFLAGS -DBUILDING_LIBCSOUND"
AC_SUBST(LIBCSOUND_CFLAGS)
LIBCSOUND_CXXFLAGS="$WINCXXFLAGS $CXXFLAGS -DBUILDING_LIBCSOUND"
AC_SUBST(LIBCSOUND_CXXFLAGS)
LIBCSOUND_LDFLAGS="-shared -module -no-undefined $SOUNDFILE_LDFLAGS $AUDIO_LDFLAGS"
AC_SUBST(LIBCSOUND_LDFLAGS)
LIBCSOUND_LIBADD="$WINLDFLAGS $SOUNDFILE_LIBS $AUDIO_LIBADD -lm"
AC_SUBST(LIBCSOUND_LIBADD)

#	All build options for the command-line front end to Csound.

CSOUND_CPPFLAGS="$CPPFLAGS"
AC_SUBST(CSOUND_CPPFLAGS)
CSOUND_CFLAGS="$ALIGN_FLAGS $CFLAGS"
AC_SUBST(CSOUND_CFLAGS)
CSOUND_CXXFLAGS="$WINCXXFLAGS $CXXFLAGS"
AC_SUBST(CSOUND_CXXFLAGS)
CSOUND_LDFLAGS="-module -no-undefined -L.libs $SOUNDFILE_LDFLAGS $AUDIO_LDFLAGS"
AC_SUBST(CSOUND_LDFLAGS)
CSOUND_LDADD=".libs/libcsound.a $WINLDFLAGS $SOUNDFILE_LIBS $AUDIO_LIBADD -lm"
AC_SUBST(CSOUND_LDADD)

#	All build options for all Csound plugins.

PLUGIN_CPPFLAGS="$CPPFLAGS"
AC_SUBST(PLUGIN_CPPFLAGS)
PLUGIN_CFLAGS="$ALIGN_FLAGS $CFLAGS -DBUILDING_LIBCSOUND"
AC_SUBST(PLUGIN_CFLAGS)
PLUGIN_LDFLAGS="-shared -module -no-undefined"
AC_SUBST(PLUGIN_LDFLAGS)
PLUGIN_LIBADD="-lm"
AC_SUBST(PLUGIN_LIBADD)

#	All build options for all Csound utility programs.

UTILITY_CPPFLAGS="$CSOUND_CPPFLAGS"
AC_SUBST(UTILITY_CPPFLAGS)
UTILITY_CFLAGS="$ALIGN_FLAGS $CFLAGS"
AC_SUBST(UTILITY_CFLAGS)
UTILITY_LDFLAGS="-DCSOUND_WITH_API -module -no-undefined -L.libs $SOUNDFILE_LDFLAGS $AUDIO_LDFLAGS"
AC_SUBST(UTILITY_LDFLAGS)
UTILITY_LDADD="$WINLDFLAGS $SOUNDFILE_LIBS $AUDIO_LIBADD -lm"
AC_SUBST(UTILITY_LDADD)

#	Translate AC_DEFINE macros to Automake conditionals,
#	which are used in Makefile.am 
#	to select sources based on platform and configuration options.

echo
echo "CONFIGURATION SUMMARY"
echo "====================="
echo

echo "Host is" $host

if test "x$USE_DOUBLE" = xyes; then
	echo "Double samples are selected (USE_DOUBLE is defined)."
else
	echo "Float samples are selected (USE_DOUBLE is not defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_DOUBLE], [test "x$USE_DOUBLE" = xyes])

if test "x$RTAUDIO" = xyes; then
	echo "Real-time audio is selected (RTAUDIO is defined)."
else
	echo "Real-time audio is not selected (RTAUDIO is not defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_RTAUDIO], [test "x$RTAUDIO" = xyes])

if test "x$FLTK" = xyes; then
	echo "FLTK is selected (USE_FLTK is defined)."
else
	echo "FLTK is not selected (USE_FLTK is not defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_FLTK], [test "x$FLTK" = xyes])

if test "x$WITH_TCLTK" = xyes; then
	echo "Tcl/Tk GUI is selected (TCLTK is defined)."
else
	echo "Tcl/Tk GUI is not selected (TCLTK is not defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_TCLTK], [test "x$WITH_TCLTK" = xyes])

if test "x$HAVE_PYTHON" = xyes; then
	echo "Python is available (HAVE_PYTHON is substituted)."
else
	echo "Python is not available (HAVE_PYTHON is not substituted)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_PYTHON], [test "x$HAVE_PYTHON" = xyes])

if test "x$HAVE_SWIG" = xyes; then
	echo "SWIG is available (HAVE_SWIG is substituted)."
else
	echo "SWIG is not available (HAVE_SWIG is not substituted)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_SWIG], [test "x$HAVE_SWIG" = xyes])

if test "x$WIN32" = xyes; then
	echo "Building on Windows (WIN32 is substituted)."
else
	echo "Not building on Windows (WIN32 is not substituted)."
fi
AM_CONDITIONAL([CONDITIONAL_IS_WIN32], [test "x$WIN32" = xyes])

if test "x$ENABLE_CSOUNDVST" = xyes; then
	echo "Building CsoundVST (ENABLE_CSOUNDVST is enabled)."
else
	echo "Not building CsoundVST (ENABLE_CSOUNDVST is disabled)."
fi
AM_CONDITIONAL([CONDITIONAL_ENABLE_CSOUNDVST], [test "x$ENABLE_CSOUNDVST" = xyes])

echo "Audio Formats suported are...  WAV  AIFF $AUDIO_FORMATS"
echo

AC_OUTPUT([Makefile csound.spec])

