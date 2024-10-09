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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
   MYFLT *out;
   MYFLT *in1, *in2;
} OPCODE;

static int32_t op_init(CSOUND *csound, OPCODE *p)
{
// Intialization code goes here
    return OK;
}

static int32_t op_k(CSOUND *csound, OPCODE *p)
{
// code called at k-rate goes here
    return OK;
}

// You can use these functions if you need to prepare and cleanup things on
// loading/unloading the library, but they can be absent if you don't need them

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
    OENTRY  *ep = (OENTRY *) &(localops[0]);
    int32_t     err = 0;
    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname, ep->dsblksiz, ep->thread,
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

static OENTRY localops[] =
{
  { "opcode",   sizeof(OPCODE),  0, 3, "i",    "ii", (SUBR)op_init, (SUBR)op_k }}
};

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

  /* VL commenting this out so ALL uses of Str(x)
     call LocalizeString() [which might be a stub]
     This would allows us to keep an eye on
     -Wformat-security warnings
  */
//#ifndef GNU_GETTEXT
//#define Str(x)  (x)
//#else
#define Str(x)  (csound->LocalizeString(x))
//#endif

PUBLIC  int64_t  csound_opcode_init(CSOUND *, OENTRY **);
PUBLIC  NGFENS  *csound_fgen_init(CSOUND *);

PUBLIC  int32_t     csoundModuleCreate(CSOUND *);
PUBLIC  int32_t     csoundModuleInit(CSOUND *);
PUBLIC  int32_t     csoundModuleDestroy(CSOUND *);
PUBLIC  const char  *csoundModuleErrorCodeToString(int32_t);

PUBLIC  int32_t     csoundModuleInfo(void);

/** The LINKAGE macro sets up linking of opcode list*/

#define LINKAGE                                                         \
PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep)             \
{ (void) csound; *ep = localops; return (int64_t) sizeof(localops);  } \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT)); }

/** The LINKAGE_BUILTIN macro sets up linking of opcode list for builtin opcodes
 * which must have unique function names */

#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)                                           \
PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep)             \
{   (void) csound; *ep = name; return (int64_t) (sizeof(name));  }         \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT)); }

/** LINKAGE for f-table plugins */

#define FLINKAGE                                                        \
PUBLIC NGFENS *csound_fgen_init(CSOUND *csound)                         \
{   (void) csound; return localfgens;                               }   \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT)); }

#undef FLINKAGE_BUILTIN
#define FLINKAGE_BUILTIN(name)                                          \
PUBLIC NGFENS *csound_fgen_init(CSOUND *csound)                         \
{   (void) csound; return name;                                     }   \
PUBLIC int32_t csoundModuleInfo(void)                                       \
{ return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT)); }

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_CSDL_H */
