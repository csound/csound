/*
    csdl.h:

    Copyright (C) 2002 John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef CSOUND_CSDL_H
#define CSOUND_CSDL_H
/**
* \file csdl.h
*
* \brief Declares the Csound plugin opcode interface.
* \author John P. ffitch, Michael Gogins, Matt Ingalls, John D. Ramsdell,
*         Istvan Varga, Victor Lazzarini.
*
* Plugin opcodes can extend the functionality of Csound, providing new
* functionality that is exposed as opcodes in the Csound language.
* Plugins need to include this header file only, as it will bring all necessary
* data structures to interact with Csound. It is not necessary for plugins
* to link to the libcsound library, as plugin opcodes will always receive a
* CSOUND* pointer (to the CSOUND_ struct) which contains all the API functions
* inside.
*
* This is the basic template for a plugin opcode. See the manual for further
* details on accepted types and function call rates. The use of the LINKAGE
* macro is highly recommended, rather than calling the functions directly.
*
* \code
#include "csdl.h"

typedef struct {
   OPDS h;
   cs_float *out;
   cs_float *in1, *in2;
} OPCODE;

static int32_t op_init(CSOUND *csound, OPCODE *p)
{
// Intialization code goes here
    return OK;
}

static int32_t op_perf(CSOUND *csound, OPCODE *p)
{
// code called at perf-time goes here
    return OK;
}

static int32_t op_deinit(CSOUND *csound, OPCODE *p)
{
// code called at deinit-time goes here
    return OK;
}

// add the opcode OENTRY:
static OENTRY localops[] =
{
  { "opcode",   sizeof(OPCODE),  0, "i",    "ii", (SUBR)op_init, (SUBR)op_k, (SUBR) op_deinit }}
};


// These are generic functions to use for any type of csound plugin module
// (opcodes, ftables, IO backends etc)
PUBLIC int32_t csoundModuleInfo(void)
{
    return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
    // opcode registration example
    OENTRY  *ep = (OENTRY *) &(localops[0]);
    int32_t     err = 0;
    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname, ep->dsblksiz, 0,
                                  ep->outypes, ep->intypes,
                                  (int32_t (*)(CSOUND *, void *)) ep->init,
                                  (int32_t (*)(CSOUND *, void *)) ep->perf,
                                  (int32_t (*)(CSOUND *, void *)) ep->deinit);
      ep++;
    }
    return err;
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound)
{
    // Called when the plugin opcode is unloaded, usually when Csound terminates.
    return 0;
}

// instead of using the above generic functions
// you can use this macro - for opcode modules only
LINKAGE(localops)

*
* \endcode
**/

#ifdef __BUILDING_LIBCSOUND
#undef __BUILDING_LIBCSOUND
#endif
#include "interlocks.h"
#include "csoundCore.h"


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__wasi__)
  #undef PUBLIC
  #define PUBLIC extern
#endif

/* Use the Str() macro for translations of strings */
#undef Str
#define Str(x)  (csound->LocalizeString(x))


PUBLIC  int64_t  csound_opcode_init(CSOUND *, OENTRY **);
PUBLIC  NGFENS  *csound_fgen_init(CSOUND *);

PUBLIC  int32_t     csoundModuleCreate(CSOUND *);
PUBLIC  int32_t     csoundModuleInit(CSOUND *);
PUBLIC  int32_t     csoundModuleDestroy(CSOUND *);
PUBLIC  const char  *csoundModuleErrorCodeToString(int32_t);

PUBLIC  int32_t     csoundModuleInfo(void);

/** Return this value from hand-written csoundModuleInfo functions too.
 * An unset precision flag retains the legacy ABI with 64-bit cs_double.
 */
#define CSOUND_MODULE_INFO \
  ((CS_VERSION << 16) | (CS_SUBVER << 8) | (int32_t) sizeof(cs_float) | \
   (sizeof(cs_double) == sizeof(float) ? CSOUND_MODULE_USE_FLOAT : 0))

/** The LINKAGE macro sets up linking of opcode list*/

#define LINKAGE                                                         \
PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep)             \
{ (void) csound; *ep = localops; return (int64_t) sizeof(localops);  } \
PUBLIC  int32_t csoundModuleInfo(void)                                       \
{ return CSOUND_MODULE_INFO; }

/** The LINKAGE_BUILTIN macro sets up linking of opcode list for builtin opcodes
 * which must have unique function names */

#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)                                           \
PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep)             \
{   (void) csound; *ep = name; return (int64_t) (sizeof(name));  }         \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return CSOUND_MODULE_INFO; } \
const OENTRY *name##_p = name; \
/* Give the const length external linkage in C++ too. */ \
extern const int32_t name##_len; \
const int32_t name##_len = (int32_t) (sizeof(name)/sizeof(OENTRY)); \

/** LINKAGE for f-table plugins */

#define FLINKAGE                                                        \
PUBLIC NGFENS *csound_fgen_init(CSOUND *csound)                         \
{   (void) csound; return localfgens;                               }   \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return CSOUND_MODULE_INFO; }

/** Give the plugin library a metadata record: its own version, which is
 * independent of the Csound version, and an optional free text. Csound and
 * other tools read the record from the library file, without loading the
 * plugin. Optional; plugins without it still load. Use it once per plugin
 * library, at file scope, followed by a semicolon:
 *
 * \code
 * CSOUND_PLUGIN_METADATA("1.4.2",
 *   "opcodes = ['myop', 'myfilter']\n");
 * \endcode
 *
 * Both arguments are string literals. The version is printable ASCII,
 * shorter than CSOUND_MODULE_VERSION_MAX bytes; Semantic Versioning is
 * recommended. Csound does not interpret the text. By convention it is TOML,
 * and `opcodes` lists the names of the opcodes the plugin registers. Use ""
 * for no text. Single-quoted TOML strings need no escaping in C.
 * The record layout is described in csound_types.h.
 * Do not use it in opcode sources built into libcsound.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  define CSOUND_PLUGIN_EXPORT_DATA __declspec(dllexport)
#  define CSOUND_PLUGIN_USED_DATA
#elif defined(__GNUC__) || defined(__clang__)
#  define CSOUND_PLUGIN_EXPORT_DATA __attribute__((visibility("default")))
#  define CSOUND_PLUGIN_USED_DATA   __attribute__((used))
#else
#  define CSOUND_PLUGIN_EXPORT_DATA
#  define CSOUND_PLUGIN_USED_DATA
#endif

/* The record type, sized for the literals: "" ver "" also makes sure each
   argument is a string literal. All members are bytes, so it has no
   padding. */
#define CSOUND_PLUGIN_METADATA_TYPE_(ver, txt)                          \
  struct csoundModuleMetadata_s {                                       \
    unsigned char marker[8];                                            \
    unsigned char format;                                               \
    unsigned char reserved[3];                                          \
    unsigned char length[4];                                            \
    char          version[sizeof("" ver "")];                           \
    char          text[sizeof("" txt "")];                              \
  }

#define CSOUND_PLUGIN_METADATA_LENGTH_(ver, txt)                        \
  (sizeof("" ver "") + sizeof("" txt ""))

/* An array of negative size stops the build when the literals do not fit. */
#define CSOUND_PLUGIN_METADATA_CHECK_(ver, txt)                         \
  typedef char csoundModuleMetadata_fits_[                              \
    (sizeof("" ver "") > 1 &&                                           \
     sizeof("" ver "") <= CSOUND_MODULE_VERSION_MAX &&                  \
     CSOUND_PLUGIN_METADATA_LENGTH_(ver, txt) <=                        \
       CSOUND_PLUGIN_METADATA_MAX) ? 1 : -1]

#define CSOUND_PLUGIN_METADATA_INIT_(ver, txt)                          \
  { { CSOUND_PLUGIN_METADATA_MARKER },                                  \
    CSOUND_PLUGIN_METADATA_FORMAT, { 0, 0, 0 },                         \
    { (unsigned char) (CSOUND_PLUGIN_METADATA_LENGTH_(ver, txt) & 0xFF),\
      (unsigned char) ((CSOUND_PLUGIN_METADATA_LENGTH_(ver, txt) >> 8)  \
                       & 0xFF),                                         \
      (unsigned char) ((CSOUND_PLUGIN_METADATA_LENGTH_(ver, txt) >> 16) \
                       & 0xFF),                                         \
      (unsigned char) ((CSOUND_PLUGIN_METADATA_LENGTH_(ver, txt) >> 24) \
                       & 0xFF) },                                       \
    ver, txt }

/* The declaration gives the const record external linkage in C++ and keeps
   -Wmissing-variable-declarations quiet in C. "used" goes on the definition
   only: compilers ignore it, with a warning, on a declaration. */
#ifdef __cplusplus
#  define CSOUND_PLUGIN_METADATA(ver, txt)                              \
     CSOUND_PLUGIN_METADATA_CHECK_(ver, txt);                           \
     CSOUND_PLUGIN_METADATA_TYPE_(ver, txt);                            \
     extern "C" CSOUND_PLUGIN_EXPORT_DATA                               \
     const struct csoundModuleMetadata_s csoundModuleMetadata;          \
     extern "C" CSOUND_PLUGIN_EXPORT_DATA CSOUND_PLUGIN_USED_DATA       \
     const struct csoundModuleMetadata_s csoundModuleMetadata =         \
       CSOUND_PLUGIN_METADATA_INIT_(ver, txt)
#else
#  define CSOUND_PLUGIN_METADATA(ver, txt)                              \
     CSOUND_PLUGIN_METADATA_CHECK_(ver, txt);                           \
     CSOUND_PLUGIN_METADATA_TYPE_(ver, txt);                            \
     extern CSOUND_PLUGIN_EXPORT_DATA                                   \
     const struct csoundModuleMetadata_s csoundModuleMetadata;          \
     CSOUND_PLUGIN_EXPORT_DATA CSOUND_PLUGIN_USED_DATA                  \
     const struct csoundModuleMetadata_s csoundModuleMetadata =         \
       CSOUND_PLUGIN_METADATA_INIT_(ver, txt)
#endif

#undef FLINKAGE_BUILTIN
#define FLINKAGE_BUILTIN(name)                                          \
PUBLIC NGFENS *csound_fgen_init(CSOUND *csound)                         \
{   (void) csound; return name;                                     }   \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return CSOUND_MODULE_INFO; }

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_CSDL_H */
