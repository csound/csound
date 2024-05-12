
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csoundCore.h"
#include "tok.h"
#include "csound_orc.h"
#include "insert.h"
#include "namedins.h"
#include "interlocks.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"

#ifndef PARSER_DEBUG
#define PARSER_DEBUG (0)
#endif

#if defined(_WIN32) || defined(_WIN64)
# define strtok_r strtok_s
#endif

#define namedInstrFlag csound->parserNamedInstrFlag

extern int csound_orcget_lineno(void*);

/* from csound_orc_compile.c */
extern char** splitArgs(CSOUND* csound, char* argString);

ORCTOKEN *lookup_token(CSOUND *csound, char *s, void *yyscanner)
{
    IGN(yyscanner);
    int type = T_IDENT;
    ORCTOKEN *ans;

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Looking up token for: %s\n", s);

    ans = new_token(csound, T_IDENT);

    if (strchr(s, ':') != NULL) {
        char* th;
        char* baseName = strtok_r(s, ":", &th);
        char* annotation = strtok_r(NULL, ":", &th);
        ans->lexeme = cs_strdup(csound, baseName);
        ans->optype = cs_strdup(csound, annotation);
        type = T_TYPED_IDENT;
    } else {
        ans->lexeme = cs_strdup(csound, s);
    }

//    if (udoflag == -2 || namedInstrFlag == 1) {
    if (namedInstrFlag == 1) {
        return ans;
    }

    ans->type = type;

    return ans;
}

static char* map_udo_in_arg_type(char* in) {
    if(strlen(in) == 1) {
      if (strchr("ijop", *in) != NULL) {
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

static void map_args(char* args) {
    while (*args != '\0') {
      if (*args == ':') {
        while(*args != 0 && *args != ';') {
          args++;
        }
      } else if (*args == 'K'){
        *args = 'k';
      }
      args++;
    }
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

static int parse_opcode_args(CSOUND *csound, OENTRY *opc)
{
    OPCODINFO   *inm = (OPCODINFO*) opc->useropinfo;
    char** in_args;
    char** out_args;
    char typeSpecifier[256];
    char tempName[20];
    int i = 0, err = 0;
    int n=0;

    ARRAY_VAR_INIT varInit;

    typeSpecifier[1] = '\0';

    // The following handles adding of extra 'o' type for optional
    // ksmps arg for all UDO's
    // this feature is removed from 7.0
    /* 
       char intypes[256];
      if (*inm->intypes == '0') {
        intypes[0] = 'o';
        intypes[1] = '\0';
    } else {
        snprintf(intypes, 256, "%so", inm->intypes);
	} 
	in_args = splitArgs(csound, intypes);*/
    in_args = splitArgs(csound, inm->intypes);
    out_args = splitArgs(csound, inm->outtypes);

    if (UNLIKELY(in_args == NULL)) {
      /* synterr(csound,
              Str("invalid input argument types found for opcode %s: %s\n"),
              inm->name, intypes); */
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
          int dimensions = 0;
          while (*in_arg == '[') {
            dimensions += 1;
            in_arg += 1;
          }

          end = in_arg;
          while(*end != ']') {
            end++;
          }
          memcpy(typeSpecifier, in_arg, end - in_arg);

          typeSpecifier[(end - in_arg)] = 0;
// printf("Dimensions: %d SubArgType: %s\n", dimensions, typeSpecifier);
          CS_TYPE* type =
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
          char *c = map_udo_in_arg_type(in_arg);
          //                printf("found arg type %s -> %c\n", in_arg, c);

          CS_TYPE* type =
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
    
//    inm->inchns = i + 1; /* Add one for optional local ksmps */
//    inm->inchns = i - 1;
    inm->inchns = i;     // this feature is removed from 7.0

    i = 0;
    if (*out_args[0] != '0') {
      while(out_args[i] != NULL) {
        char* out_arg = out_args[i];
        char* end;
        snprintf(tempName, 20, "out%d", i);

        if (*out_arg == '[') {
          int dimensions = 0;
          while (*out_arg == '[') {
            dimensions += 1;
            out_arg += 1;
          }

          end = out_arg;
          while(*end != ']') {
            end++;
          }
          memcpy(typeSpecifier, out_arg, end - out_arg);

          typeSpecifier[(end - out_arg) + 1] = 0;
          //printf("Dimensions: %d SubArgType: %s\n", dimensions, typeSpecifier);
          CS_TYPE* type =
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
          //                printf("found arg type %s -> %c\n", out_arg, c);
          CS_TYPE* type =
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

    // this feature is removed from 7.0
    // opc->intypes = cs_strdup(csound, intypes);
    opc->intypes = cs_strdup(csound, (inm->intypes[0] == '0') ? "" :
                                                                 inm->intypes);
    opc->outypes = cs_strdup(csound, (inm->outtypes[0] == '0') ? "" :
                                                                 inm->outtypes);

    map_args(opc->intypes);
    map_args(opc->outypes);

//    /* count the number of arguments, and check types */
//      default:
//        synterr(csound, Str("invalid input type for opcode %s"), inm->name);
//        err++; i--;
//      }
//      i++; types++;
//      if (UNLIKELY(i > OPCODENUMOUTS_MAX)) {
//        synterr(csound, Str("too many input args for opcode %s"), inm->name);
//        csound->LongJmp(csound, 1);
//      }
//    }
//      default:
//        synterr(csound, Str("invalid output type for opcode %s"), inm->name);
//        err++; i--;
//      }
//      i++; types++;
//    }
//

early_exit:
    if(in_args != NULL) {
      while(in_args[n] != NULL)  {
        // printf("delete %p\n", argsFound[n]);
        csound->Free(csound, in_args[n]);
        n++;
      }
      csound->Free(csound, in_args);
    }
    if (out_args != NULL) {
      n = 0;
      while(out_args[n] != NULL)  {
        // printf("delete %p\n", argsFound[n]);
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


/** Adds a UDO definition as an T_OPCODE or T_OPCODE0 type to the symbol table
 * used at parse time.  An OENTRY is also added at this time so that at
 * verification time the opcode can be looked up to get its signature.
 */
int add_udo_definition(CSOUND *csound, char *opname,
                       char *outtypes, char *intypes,
                       int flags) {

    OENTRY    tmpEntry, *opc, *newopc;
    OPCODINFO *inm;
    int len;

    if (UNLIKELY(!check_instr_name(opname))) {
      synterr(csound, Str("invalid name for opcode"));
      return -1;
    }

    len = strlen(intypes);
    if (len == 1 && *intypes == '0') {
      opc = find_opcode_exact(csound, opname, outtypes, "o");
    } else {
      // this feature is removed from 7.0
      /*char* adjusted_intypes = csound->Malloc(csound, sizeof(char) * (len + 2));
      sprintf(adjusted_intypes, "%so", intypes);
      opc = find_opcode_exact(csound, opname, outtypes, adjusted_intypes); */
      opc = find_opcode_exact(csound, opname, outtypes, intypes);
      //csound->Free(csound, adjusted_intypes);
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
    inm->intypes = intypes;
    inm->outtypes = outtypes;
    inm->in_arg_pool = csoundCreateVarPool(csound);
    inm->out_arg_pool = csoundCreateVarPool(csound);

    inm->prv = csound->opcodeInfo;
    csound->opcodeInfo = inm;

    if (opc != NULL) {
      /* printf("**** Redefining case: %s %s %s\n", */
      /*        inm->name, inm->outtypes, inm->intypes); */
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
      /* IV - Sep 8 2002: opcodes have an optional arg for ksmps */
      newopc->outypes = csound->Malloc(csound, strlen(outtypes) + 1
                                       + strlen(intypes) + 2);
      newopc->intypes = &(newopc->outypes[strlen(outtypes) + 1]);
      newopc->flags = flags | newopc->flags;
    }

    //printf("****Calling parse_opcode_args\n");
    if (UNLIKELY(parse_opcode_args(csound, newopc) != 0))
      return -3;

    return 0;
}

void synterr(CSOUND *csound, const char *s, ...)
{
    va_list args;
    va_start(args, s);
    csoundErrMsgV(csound, Str("error:  "), s, args);
    va_end(args);


    /* FIXME - Removed temporarily for debugging
     * This function may not be necessary at all in the end if some of this is
     * done in the parser
     */
    csound->synterrcnt++;
}
