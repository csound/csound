
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

    Array Type Format Contract:
    ---------------------------
    This function calls split_args() which takes EXTERNAL format input
    (from OENTRY.intypes/outypes) and returns INTERNAL format output.

    After split_args(), array types are in INTERNAL format:
      - "[k]" for 1D k-rate array
      - "[[a]" for 2D audio array
      - "[:MyType;]" for UDT array

    The code below (line ~130: if (*in_arg == '[')) expects this
    INTERNAL format where dimension brackets precede the type.
*/
static int32_t parse_udo_arg_list(CSOUND *csound, OPCODINFO *inm,
                                  char **args, bool isInput, int32_t *err)
{
    int32_t i = 0;
    char typeSpecifier[256];
    char tempName[20];
    ARRAY_VAR_INIT varInit;
    CS_VAR_POOL *pool = isInput ? inm->in_arg_pool : inm->out_arg_pool;
    const char *dir = isInput ? "input" : "output";

    if (*args[0] == '0') {
      return 0;
    }

    while (args[i] != NULL) {
      char *arg = args[i];
      snprintf(tempName, sizeof(tempName), "%s%d", isInput ? "in" : "out", i);

      if (*arg == '[') {
        int32_t dimensions = 0;
        char *end;
        while (*arg == '[') {
          dimensions += 1;
          arg += 1;
        }

        end = arg;
        while (*end != '\0' && *end != ']') {
          end++;
        }
        if (*end != ']') {
          synterr(csound,
                  Str("invalid %s array type for opcode %s: missing ']' in '%s'\n"),
                  dir, inm->name, args[i]);
          (*err)++;
          i++;
          continue;
        }
        size_t length = (size_t)(end - arg);
        if (length >= sizeof(typeSpecifier)) {
          length = sizeof(typeSpecifier) - 1;
        }
        memcpy(typeSpecifier, arg, length);
        typeSpecifier[length] = '\0';
        const CS_TYPE *type =
          csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

        if (UNLIKELY(type == NULL)) {
          synterr(csound, Str("invalid %s array type '%s' for opcode %s\n"),
                  dir, args[i], inm->name);
          (*err)++;
          i++;
          continue;
        }

        varInit.dimensions = dimensions;
        varInit.type = type;
        CS_VARIABLE *var = csoundCreateVariable(csound, csound->typePool,
                                                &CS_VAR_TYPE_ARRAY,
                                                tempName, &varInit);
        var->dimensions = dimensions;
        csoundAddVariable(csound, pool, var);
      } else {
        char *mapped;
        // Preserve :TypeName; UDT entries directly on both inputs and outputs
        if (arg[0] == ':' && strchr(arg, ';') != NULL) {
          mapped = arg;
        } else {
          mapped = isInput ? map_udo_in_arg_type(arg)
                           : map_udo_out_arg_type(arg);
        }

        const CS_TYPE *type =
          csoundGetTypeWithVarTypeName(csound->typePool, mapped);

        if (UNLIKELY(type == NULL)) {
          synterr(csound, Str("invalid %s type '%s' for opcode %s\n"),
                  dir, args[i], inm->name);
          (*err)++;
          i++;
          continue;
        }

        CS_VARIABLE *var = csoundCreateVariable(csound, csound->typePool,
                                                type, tempName, type);
        csoundAddVariable(csound, pool, var);
      }
      i++;
    }
    return i;
}

static int32_t parse_opcode_args(CSOUND *csound, OENTRY *opc,
                                 OPCODINFO *inm)
{
    if (inm == NULL) {
      synterr(csound, Str("invalid opcode info for opcode %s\n"),
              opc && opc->opname ? opc->opname : "(unknown)");
      return NOTOK;
    }
    char** in_args;
    char** out_args;
    int32_t err = 0;
    int32_t n = 0;

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

    inm->inchns = parse_udo_arg_list(csound, inm, in_args, true, &err);
    inm->outchns = parse_udo_arg_list(csound, inm, out_args, false, &err);

    opc->dsblksiz = (uint16) (sizeof(UOPCODE) +
                              sizeof(cs_float*) * (inm->inchns + inm->outchns));
    opc->dsblksiz = ((opc->dsblksiz + (uint16) 15)
                     & (~((uint16) 15)));   /* align (needed ?) */
    opc->intypes = csoundStrdup(csound, (inm->intypes[0] == '0') ? "" :
                                                                 inm->intypes);
    opc->outypes = csoundStrdup(csound, (inm->outtypes[0] == '0') ? "" :
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


static OENTRY* csound_find_internal_oentry(CSOUND* csound, OENTRY* oentry) {
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
    int32_t isRedefinition = 0;

    if (UNLIKELY(!check_instr_name(opname))) {
      synterr(csound, Str("invalid name for opcode\n"));
      return -1;
    }

    len = (int32_t) strlen(intypes);
    if (len == 1 && *intypes == '0') {
      /* For no-input UDOs, OENTRY intypes is stored as "" not "o" */
      opc = find_opcode_exact(csound, opname, outtypes, "");
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
        synterr(csound, Str("cannot redefine %s\n"), opname);
        return -2;
      }
      csound->Message(csound,
                      Str("WARNING: redefined opcode: %s\n"), opname);
      isRedefinition = 1;
    }
    /* IV - Oct 31 2002 */
    /* store the name in a linked list (note: must use csound->Calloc) */
    inm = (OPCODINFO *) csound->Calloc(csound, sizeof(OPCODINFO));
    inm->name = csoundStrdup(csound, opname);
    inm->newStyle = newStyle;
    inm->latest = inm;
    /* Duplicate strings to avoid dangling pointers to parser-allocated memory */
    inm->intypes = csoundStrdup(csound, intypes);
    inm->outtypes = csoundStrdup(csound, outtypes);
    inm->in_arg_pool = csoundCreateVarPool(csound);
    inm->out_arg_pool = csoundCreateVarPool(csound);

    /* Validate argument signatures before linking OPCODINFO or adding an OENTRY
       so a failed UDO definition never leaves a half-initialized opcode entry. */
    OENTRY parsedEntry = {0};
    parsedEntry.opname = (char *)opname;

    if (UNLIKELY(parse_opcode_args(csound, &parsedEntry, inm) != 0)) {
      CS_VARIABLE *var;
      csound->Free(csound, parsedEntry.intypes);
      csound->Free(csound, parsedEntry.outypes);
      csound->Free(csound, inm->name);
      csound->Free(csound, inm->intypes);
      csound->Free(csound, inm->outtypes);
      for (var = inm->in_arg_pool ? inm->in_arg_pool->head : NULL;
           var != NULL; var = var->next) {
        csound->Free(csound, var->varName);
      }
      for (var = inm->out_arg_pool ? inm->out_arg_pool->head : NULL;
           var != NULL; var = var->next) {
        csound->Free(csound, var->varName);
      }
      if (inm->in_arg_pool != NULL) csoundFreeVarPool(csound, inm->in_arg_pool);
      if (inm->out_arg_pool != NULL) csoundFreeVarPool(csound, inm->out_arg_pool);
      csound->Free(csound, inm);
      return -3;
    }

    inm->prv = csound->opcodeInfo;
    csound->opcodeInfo = inm;

    /* Always create a fresh OENTRY for this UDO definition.
       For redefinitions, we don't reuse the old OENTRY because running
       instances may still reference it. Instead, we create a new one
       and prepend it so lookups find the new definition first. */
    opc = find_opcode(csound, "##userOpcode");
    memcpy(&tmpEntry, opc, sizeof(OENTRY));
    tmpEntry.opname = csoundStrdup(csound, opname);

    if (isRedefinition) {
      /* Use prepend so redefined UDOs are found first in opcode lookup */
      csoundPrependOpcodes(csound, &tmpEntry, 1);
    } else {
      /* Use append to preserve polymorphic ordering for new definitions */
      csoundAppendOpcodes(csound, &tmpEntry, 1);
    }
    newopc = csound_find_internal_oentry(csound, &tmpEntry);
    inm->oentry = newopc;
    if (LIKELY(newopc != NULL)) {
      newopc->useropinfo = (void*) inm; /* ptr to opcode parameters */
      newopc->dsblksiz = parsedEntry.dsblksiz;
      newopc->intypes = parsedEntry.intypes;
      newopc->outypes = parsedEntry.outypes;
      newopc->flags = flags | newopc->flags;
      // add opcodeDef for this UDO
      add_opcode_def(csound, newopc);
    } else {
      csound->Free(csound, parsedEntry.intypes);
      csound->Free(csound, parsedEntry.outypes);
    }
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
    csoundErrMsgV(csound, "\nsyntax error, ", s, args);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    if(csoundGetDebug(csound)) {
    // Also echo semantic errors to the normal message channel to make them visible in verbose runs
     csound->Message(csound, "SEMERR: ");
     csoundMessageV(csound, 0, s, args_copy);
    }
    va_end(args_copy);
#endif
    va_end(args);
    csound->synterrcnt++;
}
