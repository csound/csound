#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Unless you are adding new library dependencies or new platforms to the Csound build,
# you should not need to modify this file.

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
AC_PROG_RANLIB

# Checks for programs.
AC_PROG_CXX
dnl Add warning when using G++
if test "X$GXX" = Xyes ; then
  CXXFLAGS="$CXXFLAGS -Wall"
fi
AC_PROG_CC
dnl Add warning when using GGG
if test "X$GCC" = Xyes ; then
  CFLAGS="$CFLAGS -Wall"
fi
AM_PROG_CC_STDC

AC_PATH_X
AC_PATH_XTRA

AC_DEFINE([CSOUND_WITH_API], 1, [Define when building Csound with the Csound API library])

AC_ARG_ENABLE([double],
        AC_HELP_STRING([--enable-double],
                [enable the use of double precision floating point internally, default=no]),
        [USE_DOUBLE="$enableval"],
        [USE_DOUBLE=no])

if test "x$USE_DOUBLE" = xyes; then
  AC_DEFINE([USE_DOUBLE], 1, [Define when using double precision floating point])
fi

# Checks for libraries.

AC_CHECK_LIB([dl], [dlsym])
AC_CHECK_LIB([asound], [snd_pcm_hw_params_any])
# FIXME: Replace `main' with a function in `-lbsd':
#AC_CHECK_LIB([bsd], [main])
# Next line is for SGI library `-laudio'
AC_CHECK_LIB([audio], [ALsetparams])
# Next line is for SUN library `-laudio'
AC_CHECK_LIB([audio], [audioSetDevInfo])
# on HP, to include rt devaudio library
AC_CHECK_LIB([Alib], [AOpenAudio])
#  NeXT midi support
AC_CHECK_LIB([midi], [midi_set_owner])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([X11], [XOpenDisplay])
AC_CHECK_LIB([sndfile], [sf_open])

# Checks for header files.

AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([OS.h arpa/inet.h fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h sgtty.h bsd/sgtty.h stddef.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h sdif.h portaudio.h sndfile.h portmidi.h mmsystem.h boost/numeric/ublas/blas.hpp boost/random.hpp python2.3/Python.h FL/Fl.H])

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

#	Options to configure.

AC_ARG_ENABLE(windows,
	AC_HELP_STRING([--enable-windows],
		[enables graphical display of waveforms default=yes]),
    [WINDOWS="$enableval"],
	[WINDOWS='yes'])

AC_ARG_ENABLE(rtaudio,
	AC_HELP_STRING([--enable-rtaudio],
		[enables real-time audio default=yes]),
	[USE_RTAUDIO="$enableval"],
	[USE_RTAUDIO=yes])

AC_ARG_ENABLE(sf-ircam,
	[AC_HELP_STRING([--enable-sf-ircam],
		[enable reading/writing IRCAM-format sound files, with 1024-byte header (default=yes)])],
	[USE_IRCAM=$enableval],
	[USE_IRCAM=yes])

AC_ARG_ENABLE(sf-sun41,
	AC_HELP_STRING([--enable-sf-sun41],
		[enable reading/writing SUN4.1-format soundfiles, with their own header default=no]),
	[USE_SUN41=$enableval],
	[USE_SUN41=no])

AC_ARG_ENABLE(sf-NeXT,
	AC_HELP_STRING([--enable-sf-NeXT],
		[enable reading/writing SUN4.1-format soundfiles, with their own header default=no]),
	[USE_NeXT=$enableval],
	[USE_NeXT=no])

AC_ARG_WITH(fltk,
	AC_HELP_STRING([--with-fltk],
	    [enables the use of the fltk windowing toolkit (http://www.fltk.org/) (over the local window msystem) if fltk can be found default=yes]),
	[USE_FLTK=$withval],
	[USE_FLTK=yes])

AC_ARG_WITH(gui,
	AC_HELP_STRING([--with-gui],
	    [enables the use of the fltk rt-gui default=yes]),
	[USE_FLTK_WIDGETS=$withval],
	[USE_FLTK_WIDGETS=yes])

AC_ARG_WITH(tcltk,
	AC_HELP_STRING([--with-tcltk],
	    [enables the use of the Tcl/tk ( default=yes)]),
	[USE_TCLTK=$withval],
	[USE_TCLTK=yes])

AC_ARG_WITH(libsndfile,
	AC_HELP_STRING([--with-libsndfile],
	    [enables the use of libsndfile for soundfile access (default=yes)]),
	[USE_LIBSNDFILE=$withval],
	[USE_LIBSNDFILE=yes])

#AC_ARG_WITH(gcc3,
#	AC_HELP_STRING([--with-gcc3],
#		[use gcc3 style CFLAGS (default=yes)]),
#	[USE_GCC3_FLAGS=$withval],
#	[USE_GCC3_FLAGS=yes])

#	Compiler options used for all compilation.

CPPFLAGS="-DBETA -IH -I/usr/local/include -I/usr/local/boost"

CFLAGS="-O2 -g -Wall"

if test "x$GCC" = xyes; then
  case `$CC -dumpversion 2>/dev/null` in
    [[12]].*) 
      ALIGN_FLAGS="-malign-loops=4 -malign-jumps=4";;
    *) 
      ALIGN_FLAGS="-falign-loops=4 -falign-jumps=4";;
  esac
fi


#	Now we adapt compiler options to the platform.

case $host in 
    *irix*) 
		SGI="yes"
	    AC_DEFINE([SGI], 1, [Build for SGI platform])
        XCFLAGS=""
        AUDIO_LIBS="-laudio"
	    ;;

	*sun*)
		sun="yes"
	    AC_DEFINE([sun], 1, [Build for SunOS platform])
        XCFLAGS="-ffpa"
        AUDIO_LIBS="-laudio"
	    ;;

    *-*-msdos* | *-*-go32* | *-*-mingw32* | *-*-cygwin* | *-*-windows*)
		WIN32="yes"
	    AC_DEFINE([WIN32], 1, [Build for Win32 platform])
        AUDIO_LDFLAGS="-mconsole -L/usr/local/lib"
        AUDIO_LIBS="-lportaudio -lwinmm"
	    ;;

    *linux*)
		LINUX="yes"
        AC_DEFINE([LINUX], 1, [Build for Linux platform])
        AUDIO_LDFLAGS="-L/usr/local/lib"
        AUDIO_LIBS="-lportaudio -lasound"
	    if test "$WORDS_BIG_ENDIAN"; then
			AC_DEFINE([LINUX_BE], 1, [Build for Linux big-endian platform])
	    fi
        XCFLAGS="-Wall $ALIGN_FLAGS -ffast-math -fomit-frame-pointer -finline-functions -funroll-loops -DWITHx87"
	    ;;
    *dec*)
		DEC="yes"
        AC_DEFINE([DEC], 1, [Build for DEC platform])
        XCFLAGS=""
	    ;;
	*hp*)
		HP="yes"
        AC_DEFINE([HP], 1, [Build for HP platform])
        XCFLAGS="-Aa +e +x"
        AUDIO_LIBS="-Alib"
	    ;;
     *)
      AC_MSG_WARN([Unable to automatically configure for your system])
      ;;
esac

if test "$USE_RTAUDIO" = yes; then
	AC_DEFINE([RTAUDIO], 1, [Define to enable real-time audio])
fi

AUDIO_FORMATS=""
if test "$USE_IRCAM" = yes; then
   AC_DEFINE([IRCAM], 1, [Enable IRCAM soundfile format])
   echo "Using ircam"
   AUDIO_FORMATS="$AUDIO_FORMATS IRCAM"
fi

if test "$USE_SUN41" = yes; then
   AC_DEFINE([SUN41], 1, [Enable Sun 4.1 soundfile format])
   AUDIO_FORMATS="$AUDIO_FORMATS SUN41"
fi

if test "$USE_NeXT" = yes; then
   AC_DEFINE([NeXT], 1, [Enable NeXT soundfile format])
   AUDIO_FORMATS="$AUDIO_FORMATS NeXT"
fi

if test "$USE_TCLTK" = yes; then
   AC_CHECK_PROG(TCLTK,wish)
   AC_DEFINE([TCLTK], 1, [Enable Tcl/Tk support])
fi

if test "$USE_LIBSNDFILE" = yes; then
   AC_DEFINE([_SNDFILE_], 1, [Use libsndfile for soundfile access])
   SOUNDFILE_LDFLAGS="-L/usr/local/lib"
   SOUNDFILE_LIBS="-lsndfile"
fi

if test "x$WINDOWS" = xyes; then
   AC_DEFINE([WINDOWS], 1, [Enable graphing of function tables])
fi

if test "x$WINDOWS" = xyes; then
	if test "x$USE_FLTK" = xyes; then
		AC_DEFINE(USE_FLTK, 1, [Define to use FLTK for graphs])
        AC_HAVE_LIBRARY(fltk)
        AC_HAVE_LIBRARY(pthread)
    fi
fi

if test "x$USE_FLTK_WIDGETS" = xyes; then
	AC_PATH_PROG(FLTK_CONFIG,fltk-config)
	AC_LANG_CPLUSPLUS
	AC_LANG_C
	WINLDFLAGS=`$FLTK_CONFIG --ldflags` 
	WINLDFLAGS="$WINLDFLAGS -lpthread -lstdc++"
	WINCXXFLAGS=`$FLTK_CONFIG --cxxflags`
    AC_DEFINE([USE_FLTK_WIDGETS], 1, [Use GUI])
	AC_DEFINE(USE_FLTK, 1, [Define to build FLTK widget opcodes])
    AC_HAVE_LIBRARY(fltk)
    AC_HAVE_LIBRARY(pthread)
fi

#	All build options for the Csound API library 
#	and also the convenience library for utility programs.

LIBCSOUND_CPPFLAGS="$CPPFLAGS"
AC_SUBST(LIBCSOUND_CPPFLAGS)
LIBCSOUND_CFLAGS="$ALIGN_FLAGS $XCFLAGS $CFLAGS"
AC_SUBST(LIBCSOUND_CFLAGS)
LIBCSOUND_CXXFLAGS="$WINCXXFLAGS"
AC_SUBST(LIBCSOUND_CXXFLAGS)
LIBCSOUND_LDFLAGS="-shared -module -no-undefined $SOUNDFILE_LDFLAGS $AUDIO_LDFLAGS"
AC_SUBST(LIBCSOUND_LDFLAGS)
LIBCSOUND_LIBADD="$WINLDFLAGS $SOUNDFILE_LIBS $AUDIO_LIBS -lm"
AC_SUBST(LIBCSOUND_LIBADD)

#	All build options for the command-line front end to Csound.

CSOUND_CPPFLAGS="$CPPFLAGS"
AC_SUBST(CSOUND_CPPFLAGS)
CSOUND_CFLAGS="$ALIGN_FLAGS $XCFLAGS $CFLAGS"
AC_SUBST(CSOUND_CFLAGS)
CSOUND_CXXFLAGS="$WINCXXFLAGS"
AC_SUBST(CSOUND_CXXFLAGS)
CSOUND_LDFLAGS="-module -no-undefined -L.libs $SOUNDFILE_LDFLAGS $AUDIO_LDFLAGS"
AC_SUBST(CSOUND_LDFLAGS)
CSOUND_LDADD=".libs/libcsound.a $WINLDFLAGS $SOUNDFILE_LIBS $AUDIO_LIBS -lm"
AC_SUBST(CSOUND_LDADD)

#	All build options for all Csound plugins.

PLUGIN_CPPFLAGS="$CPPFLAGS"
AC_SUBST(PLUGIN_CPPFLAGS)
PLUGIN_CFLAGS="$ALIGN_FLAGS $XCFLAGS $CFLAGS"
AC_SUBST(PLUGIN_CFLAGS)
PLUGIN_LDFLAGS="-shared -module -no-undefined"
AC_SUBST(PLUGIN_LDFLAGS)
PLUGIN_LIBADD="-lm"
AC_SUBST(PLUGIN_LIBADD)

#	All build options for all Csound utility programs.

UTILITY_CPPFLAGS="$CSOUND_CPPFLAGS"
AC_SUBST(UTILITY_CPPFLAGS)
UTILITY_CFLAGS="$ALIGN_FLAGS $XCFLAGS $CFLAGS"
AC_SUBST(UTILITY_CFLAGS)
UTILITY_LDFLAGS="-DCSOUND_WITH_API -module -no-undefined -L.libs $SOUNDFILE_LDFLAGS $AUDIO_LDFLAGS"
AC_SUBST(UTILITY_LDFLAGS)
UTILITY_LDADD="$WINLDFLAGS $SOUNDFILE_LIBS $AUDIO_LIBS -lm"
AC_SUBST(UTILITY_LDADD)

#	Translate AC_DEFINE macros to Automake conditionals,
#	which are used in Makefile.am 
#	to select sources based on platform and configuration options.

echo
echo "CONFIGURATION SUMMARY"
echo "====================="
echo

echo "Host is" $host

if test "x$SGI" = xyes; then
	echo "Platform  is SGI (SGI is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_SGI], [test "x$SGI" = xyes])

if test x"$sun" = xyes; then
	echo "Platform is SunOS (sun is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_sun], [test "x$sun" = xyes])

if test "x$DEC" = xyes; then
	echo "Platform is DEC (DEC is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_DEC], [test "x$DEC" = xyes])

if test "x$HP" = xyes; then
	echo "Platform is HP (DEC is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_HP], [test "x$HP" = xyes])

if test "x$WIN32" = xyes; then
	echo "Platform is Microsoft Windows (WIN32 is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_WIN32], [test "x$WIN32" = xyes])

if test "x$LINUX" = xyes; then
	echo "Platform is Linux (LINUX is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_LINUX], [test "x$LINUX" = xyes])

if test "x$LINUX_BE" = xyes; then
	echo "Platform is Linux (big-endian) (LINUX_BE is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_LINUX_BE], [test "x$LINUX_BE = xyes"])

if test "x$WINDOWS" = xyes; then
	echo "Graphing of function tables is enabled (WINDOWS is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_WINDOWS], [test "x$WINDOWS" = xyes])

if test "x$USE_FLTK" = xyes; then
	echo "FLTK is available (USE_FLTK is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_FLTK], [test "x$USE_FLTK" = xyes])

if test "x$USE_FLTK_WIDGETS" = xyes; then
	echo "FLTK widgets are enabled (USE_FLTK_WIDGETS is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_FLTK_WIDGETS], [test "x$USE_FLTK_WIDGETS" = xyes])

if test "x$USE_TCLTK" = xyes; then
	echo "Tcl/Tk GUI is enabled (USE_TCLTK is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_TCLTK], [test "x$USE_TCLTK" = xyes])

if test "x$USE_X11" = xyes; then
	echo "FLTK is not available, but X11 is (USE_X11 is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_X11], [test "x$USE_X11" = xyes])

if test "x$USE_LIBSNDFILE" = xyes; then
	echo "lbsndfile is enabled (_SNDFILE_ is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_LIBSNDFILE], [test "x$USE_LIBSNDFILE" = xyes])

if test "x$USE_RTAUDIO" = xyes; then
	echo "Real-time audio is available (USE_RTAUDIO is defined)."
fi
AM_CONDITIONAL([CONDITIONAL_USE_RTAUDIO], [test "x$USE_RTAUDIO" = xyes])

echo "Audio Formats suported are...  WAV  AIFF $AUDIO_FORMATS"
echo

AC_OUTPUT([Makefile csound.spec])

