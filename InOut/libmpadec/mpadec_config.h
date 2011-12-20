
/* Hardware architecture */
//#define ARCH_ALPHA
//#define ARCH_PPC
//#define ARCH_SPARC
#define ARCH_X86
//#define ARCH_AMD64
//#define ARCH_IA64

#ifdef WIN32
#define HAVE_IO_H
#define HAVE_CONIO_H
#undef OSS
#else
#define HAVE_INTTYPES_H
#endif

#define FLOAT MYFLT

#include "sysdep.h"

/*#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
typedef signed char      int8_t;
typedef unsigned char    uint8_t;
typedef signed short     int16_t;
typedef unsigned short   uint16_t;
#if defined(__BORLANDC__) || defined(_MSC_VER)
typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;
#elif defined(__GNUC__)
typedef signed long long   int64_t;
typedef unsigned long long uint64_t;
#endif
#if defined(ARCH_AMD64) || defined(ARCH_IA64) || defined(ARCH_ALPHA)
typedef signed int       int32_t;
typedef unsigned int     uint32_t;
typedef int64_t  intptr_t;
typedef uint64_t uintptr_t;
#else
typedef signed long      int32_t;
typedef unsigned long    uint32_t;
typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;
#endif
#endif
*/

#undef PACKED
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

#include <string.h>
#include <memory.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif

#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE  1

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef WIN32
#define strcasecmp stricmp
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

