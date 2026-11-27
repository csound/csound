/* Csound numeric types. See docs/numeric-types.md for the CS7 migration. */
#ifndef CSOUND_TYPES_H
#define CSOUND_TYPES_H

#if !defined(USE_DOUBLE) && !defined(USE_FLOAT)
#include "float-version.h"
#endif

#if defined(USE_DOUBLE) && defined(USE_FLOAT)
#error "USE_DOUBLE and USE_FLOAT cannot be enabled together"
#endif

/* Audio samples and opcode arguments; USE_DOUBLE keeps the CS7 default. */
#ifdef USE_DOUBLE
typedef double cs_float;
#else
typedef float cs_float;
#endif

/* Internal calculations that normally need double precision. */
#ifdef USE_FLOAT
typedef float cs_double;
#define CS_DOUBLE_SCAN "f"
#define cs_modf modff
#else
typedef double cs_double;
#define CS_DOUBLE_SCAN "lf"
#define cs_modf modf
#endif

/* Plugin metadata: bit 7 marks the ABI with a single-precision cs_double.
   Older loaders see an invalid sample size and reject this ABI too. */
#define CSOUND_MODULE_USE_FLOAT 0x80

/* Plugin metadata record, written by CSOUND_PLUGIN_METADATA (csdl.h) and
   read from the library file without loading it. Layout, all fields bytes:

     offset  0  marker, the 8 bytes of CSOUND_PLUGIN_METADATA_MARKER
     offset  8  format, CSOUND_PLUGIN_METADATA_FORMAT
     offset  9  3 reserved bytes, zero
     offset 12  payload length in bytes, 32-bit little-endian
     offset 16  payload: the version, NUL-terminated, then the text,
                NUL-terminated

   CSOUND_MODULE_VERSION_MAX is the room for the version including its NUL,
   CSOUND_PLUGIN_METADATA_MAX the most payload bytes a record can have. */
#define CSOUND_MODULE_VERSION_MAX           64
#define CSOUND_PLUGIN_METADATA_FORMAT       1
#define CSOUND_PLUGIN_METADATA_HEADER_SIZE  16
#define CSOUND_PLUGIN_METADATA_MAX          65536
#define CSOUND_PLUGIN_METADATA_MARKER \
  'C', 'S', 'P', 'L', 'M', 'E', 'T', 'A'

/* Deprecated in CS7. Keep old host and opcode source code valid. */
#ifndef __MYFLT_DEF
#define __MYFLT_DEF
/* Temporarily pause these warnings. Re-enable the attributes later. */
#if defined(SWIG)
typedef cs_float MYFLT;
#elif defined(_MSC_VER)
typedef /* __declspec(deprecated("MYFLT is deprecated in CS7; use cs_float")) */ cs_float MYFLT;
#elif defined(__GNUC__) || defined(__clang__)
typedef cs_float MYFLT /* __attribute__((deprecated("MYFLT is deprecated in CS7; use cs_float"))) */;
#else
typedef cs_float MYFLT;
#endif
#endif

#endif /* CSOUND_TYPES_H */
