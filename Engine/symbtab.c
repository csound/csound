
/*
    symbtab.c:

    Copyright (C) 2006
    John ffitch, Steven Yi

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csoundCore.h"
#include "tok.h"
#include "csound_orc.h"
#include "udo.h"
#include "namedins.h"
#include "interlocks.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"
#include "csound_orc_compile.h"

#ifndef PARSER_DEBUG
#define PARSER_DEBUG (0)
#endif

#if defined(_WIN32) || defined(_WIN64)
# define strtok_r strtok_s
#endif

static char* map_udo_in_arg_type(char* in) {
    if(strlen(in) == 1) {
      if (strchr("ijopqvh", *in) != NULL) {
          return "i";
      } else if (strchr("kKOJPV", *in) != NULL) {
          return "k";
      }
    }
    return in;
}

static char* map_udo_out_arg_type(char* in) {
    if (strlen(in) == 1 && *in == 'K') {
        return "k";
    }
    return in;
}

/**
 *
  This function takes in the arguments from useropinfo in OENTRY and
  parses them, filling the OENTRY input and output types and creating
  the argument lists for xinset/xouset in insert.c argument
  pointerlists, stored in useropinfo->in_ndx_list and
  useropinfo->out_ndx_list.

  The argument lists are terminated by a -1 and are set in the
  following order:
  i-var args (i-time vars)
  S-var args (strings)
  i-arrays
  a-vars
  k-vars
  f-sigs
  arrays (a,k,f)

  This order is fixed and followed up in xinset/xoutset and
  useropcd1, useropcd2.

 Original code - IV Oct 12 2002
 modified by VL for Csound 6

    VL: 9.2.22 we are disabling the unused and confusing feature of
    a hidden local sampling rate parameter on 7.x

*/
static int32_t parse_opcode_args(CSOUND *csound, OENTRY *opc)
{
    OPCODINFO   *inm = (OPCODINFO*) opc->useropinfo;
    char** in_args;
    char** out_args;
    char typeSpecifier[256];
    char tempName[20];
    int32_t i = 0, err = 0;
    int32_t n=0;

    ARRAY_VAR_INIT varInit;

    typeSpecifier[1] = '\0';


    in_args = split_args(csound, inm->intypes);
    out_args = split_args(csound, inm->outtypes);

    if (UNLIKELY(in_args == NULL)) {
      synterr(csound,
              Str("invalid input argument types found for opcode %s: %s\n"),
              inm->name, inm->intypes);
      err++;
    }
    if (UNLIKELY(out_args == NULL)) {
      synterr(csound,
              Str("invalid output argument types found for opcode %s: %s\n"),
              inm->name, inm->outtypes);
      err++;
    }

    if (UNLIKELY(err > 0)) goto early_exit;

    if (*in_args[0] != '0') {
      while (in_args[i] != NULL) {
        char* in_arg = in_args[i];
        char* end;
        snprintf(tempName, 20, "in%d", i);

        if (*in_arg == '[') {
          int32_t dimensions = 0;
          while (*in_arg == '[') {
            dimensions += 1;
            in_arg += 1;
          }

          end = in_arg;
          while (*end != '\0' && *end != ']') {
            end++;
          }
          if (*end != ']') {
            synterr(csound, Str("invalid input array type for opcode %s: missing ']' in '%s'\n"), inm->name, in_args[i]);
            err++;
            i++;
            continue;
          }
          size_t length = (size_t)(end - in_arg);
          if (length >= sizeof(typeSpecifier)) {
            length = sizeof(typeSpecifier) - 1;
          }
          memcpy(typeSpecifier, in_arg, length);
          typeSpecifier[length] = '\0';
          CS_TYPE* type = (CS_TYPE *)
            csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid input type for opcode %s\n"), in_arg);
            err++;
            i++;
            continue;
          }

          varInit.dimensions = dimensions;
          varInit.type = type;
          CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,
                                                  (CS_TYPE*)&CS_VAR_TYPE_ARRAY,
                                                  tempName, &varInit);
          var->dimensions = dimensions;
          csoundAddVariable(csound, inm->in_arg_pool, var);
        } else {
          char *c;
          // Skip map_udo_in_arg_type for UDT (User Defined Type) entries
          // UDT entries are in the format :TypeName; and should be used directly
          if (in_arg[0] == ':' && strchr(in_arg, ';') != NULL) {
            c = in_arg;  // Use UDT type string directly
          } else {
            c = map_udo_in_arg_type(in_arg);  // Map single-char types
          }

          CS_TYPE* type = (CS_TYPE *)
            csoundGetTypeWithVarTypeName(csound->typePool, c);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid input type for opcode %s\n"), in_args[i]);
            err++;
            i++;
            continue;
          }

          CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,
                                                  type, tempName, type);
          csoundAddVariable(csound, inm->in_arg_pool, var);
        }
        i++;
      }
    }

    inm->inchns = i;
    i = 0;
    if (*out_args[0] != '0') {
      while(out_args[i] != NULL) {
        char* out_arg = out_args[i];
        char* end;
        snprintf(tempName, 20, "out%d", i);

        if (*out_arg == '[') {
          int32_t dimensions = 0;
          while (*out_arg == '[') {
            dimensions += 1;
            out_arg += 1;
          }

          end = out_arg;
          while (*end != '\0' && *end != ']') {
            end++;
          }
          if (*end != ']') {
            synterr(csound, Str("invalid output array type for opcode %s: missing ']' in '%s'\n"), inm->name, out_args[i]);
            err++;
            i++;
            continue;
          }
          size_t length = (size_t)(end - out_arg);
          if (length >= sizeof(typeSpecifier)) {
            length = sizeof(typeSpecifier) - 1;
          }
          memcpy(typeSpecifier, out_arg, length);
          typeSpecifier[length] = '\0';
          CS_TYPE* type = (CS_TYPE *)
            csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid output type for opcode %s"), out_args[i]);
            err++;
            i++;
            continue;
          }

          varInit.dimensions = dimensions;
          varInit.type = type;
          CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,
                                                  (CS_TYPE*)&CS_VAR_TYPE_ARRAY,
                                                  tempName, &varInit);
          var->dimensions = dimensions;
          csoundAddVariable(csound, inm->out_arg_pool, var);
        } else {
          char* c = map_udo_out_arg_type(out_arg);
          CS_TYPE* type = (CS_TYPE *)
            csoundGetTypeWithVarTypeName(csound->typePool, c);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid output type for opcode %s"), out_arg);
            err++;
            i++;
            continue;
          }

          CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool, type,
                                                  tempName, type);
          csoundAddVariable(csound, inm->out_arg_pool, var);
        }
        i++;
      }
    }

    inm->outchns = i;
    opc->dsblksiz = (uint16) (sizeof(UOPCODE) +
                              sizeof(MYFLT*) * (inm->inchns + inm->outchns));
    opc->dsblksiz = ((opc->dsblksiz + (uint16) 15)
                     & (~((uint16) 15)));   /* align (needed ?) */
    opc->intypes = cs_strdup(csound, (inm->intypes[0] == '0') ? "" :
                                                                 inm->intypes);
    opc->outypes = cs_strdup(csound, (inm->outtypes[0] == '0') ? "" :
                                                                 inm->outtypes);
    // Keep the internal format for type matching - do NOT apply map_args here

early_exit:
    if(in_args != NULL) {
      while(in_args[n] != NULL)  {
        csound->Free(csound, in_args[n]);
        n++;
      }
      csound->Free(csound, in_args);
    }
    if (out_args != NULL) {
      n = 0;
      while(out_args[n] != NULL)  {
        csound->Free(csound, out_args[n]);
        n++;
      }
      csound->Free(csound, out_args);
    }
    return err;
}


OENTRY* csound_find_internal_oentry(CSOUND* csound, OENTRY* oentry) {
    CONS_CELL *items;
    char *shortName;
    OENTRY *ep, *retVal = NULL;

    if (oentry == NULL) {
        return NULL;
    }
    shortName = get_opcode_short_name(csound, oentry->opname);

    items = cs_hash_table_get(csound, csound->opcodes, shortName);

    while (items != NULL) {
        ep = items->value;
        if (oentry->init == ep->init &&
            oentry->perf == ep->perf &&
            oentry->deinit == ep->deinit &&
            strcmp(oentry->opname, ep->opname) == 0 &&
            strcmp(oentry->outypes, ep->outypes) == 0 &&
            strcmp(oentry->intypes, ep->intypes) == 0) {
            retVal = ep;
            break;
        }
        items = items->next;
    }

    if (shortName != oentry->opname) {
        csound->Free(csound, shortName);
    }

    return retVal;
}

void add_opcode_def(CSOUND *csound, OENTRY *ep);

/** Adds a UDO definition as an T_OPCODE or T_OPCODE0 type to the symbol table
 * used at parse time.  An OENTRY is also added at this time so that at
 * verification time the opcode can be looked up to get its signature.
 */
int32_t add_udo_definition(CSOUND *csound, bool newStyle, char *opname,
                       char *outtypes, char *intypes,
                       int32_t flags) {

    OENTRY    tmpEntry, *opc, *newopc;
    OPCODINFO *inm;
    int32_t len;

    if (UNLIKELY(!check_instr_name(opname))) {
      synterr(csound, Str("invalid name for opcode"));
      return -1;
    }

    len = (int32_t) strlen(intypes);
    if (len == 1 && *intypes == '0') {
      opc = find_opcode_exact(csound, opname, outtypes, "o");
    } else {
      opc = find_opcode_exact(csound, opname, outtypes, intypes);
    }

    /* check if opcode is already defined */
    if (UNLIKELY(opc != NULL)) {

      // check if the opcode is already declared
      if (opc->flags & UNDEFINED) {
        opc->flags = 0x0000;
        return 0;
      }

      /* IV - Oct 31 2002: redefine old opcode if possible */
      if (UNLIKELY(!strcmp(opname, "instr") ||
                   !strcmp(opname, "endin") ||
                   !strcmp(opname, "opcode") ||
                   !strcmp(opname, "endop") ||
                   !strcmp(opname, "$label") ||
                   !strcmp(opname, "pset") ||
                   !strcmp(opname, "xin") ||
                   !strcmp(opname, "xout") ||
                   !strcmp(opname, "subinstr"))) {
        synterr(csound, Str("cannot redefine %s"), opname);
        return -2;
      }
      csound->Message(csound,
                      Str("WARNING: redefined opcode: %s\n"), opname);
    }
    /* IV - Oct 31 2002 */
    /* store the name in a linked list (note: must use csound->Calloc) */
    inm = (OPCODINFO *) csound->Calloc(csound, sizeof(OPCODINFO));
    inm->name = cs_strdup(csound, opname);
    inm->newStyle = newStyle;
    inm->intypes = intypes;
    inm->outtypes = outtypes;
    inm->in_arg_pool = csoundCreateVarPool(csound);
    inm->out_arg_pool = csoundCreateVarPool(csound);

    inm->prv = csound->opcodeInfo;
    csound->opcodeInfo = inm;

    if (opc != NULL) {
      opc->useropinfo = inm;
      newopc = opc;
    } else {
      /* IV - Oct 31 2002: */
      /* create a fake opcode so we can call it as such */
      opc = find_opcode(csound, "##userOpcode");
      memcpy(&tmpEntry, opc, sizeof(OENTRY));
      tmpEntry.opname = cs_strdup(csound, opname);

      csoundAppendOpcodes(csound, &tmpEntry, 1);
      newopc = csound_find_internal_oentry(csound, &tmpEntry);

      newopc->useropinfo = (void*) inm; /* ptr to opcode parameters */

      /* check in/out types and copy to the opcode's */
      newopc->outypes = csound->Malloc(csound, strlen(outtypes) + 1
                                       + strlen(intypes) + 1);
      newopc->intypes = &(newopc->outypes[strlen(outtypes) + 1]);
      newopc->flags = flags | newopc->flags;
    }

    if (UNLIKELY(parse_opcode_args(csound, newopc) != 0))
      return -3;

    // add opcodeDef for this UDO
    add_opcode_def(csound, newopc);
    return 0;
}

void synterr(CSOUND *csound, const char *s, ...)
{
    va_list args;
    va_start(args, s);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    va_list args_copy;
    va_copy(args_copy, args);
#endif
    csoundErrMsgV(csound, Str("error: "), s, args);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    // Also echo semantic errors to the normal message channel to make them visible in verbose runs
    char buf[1024];
    vsnprintf(buf, sizeof(buf), s, args_copy);
    va_end(args_copy);
    csound->Message(csound, "SEMERR: %s\n", buf);
#endif
    va_end(args);
    csound->synterrcnt++;
}
