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


// FIXME - this is global...
// VL moved to csound struct
//CS_HASH_TABLE* symbtab;

#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag

ORCTOKEN *add_token(CSOUND *csound, char *s, int type);
//static ORCTOKEN *add_token_p(CSOUND *csound, char *s, int type, int val);
extern int csound_orcget_lineno(void*);

/* from csound_orc_compile.c */
extern char** splitArgs(CSOUND* csound, char* argString);

int get_opcode_type(OENTRY *ep)
{
    int retVal = 0;

    if (ep->outypes == NULL || strlen(ep->outypes) == 0) {
      retVal = T_OPCODE0;
    }
    else {
      retVal = T_OPCODE;
    }
    return retVal;
}

void init_symbtab(CSOUND *csound)
{
    OENTRY *ep;
    CONS_CELL *top, *head, *items;

    char *shortName;


    if (csound->symbtab == NULL) {
      /* VL 27 02 2015 -- if symbtab exists, do not create it again
        to avoid memory consumption.
       */
      //printf("init symbtab\n");
      csound->symbtab = cs_hash_table_create(csound);
    /* Now we need to populate with basic words */
    /* Add token types for opcodes to symbtab.  If a polymorphic opcode
     * definition is found (dsblksiz >= 0xfffb), look for implementations
     * of that opcode to correctly mark the type of opcode it is (T_OPCODE,
     * T_OPCODE0, or T_OPCODE00)
     */

    top = head = cs_hash_table_values(csound, csound->opcodes);

    while (head != NULL) {
      items = head->value;
      while (items != NULL) {
        ep = items->value;

        if (ep->dsblksiz < 0xfffb) {
          shortName = get_opcode_short_name(csound, ep->opname);

          add_token(csound, shortName, get_opcode_type(ep));

          if (shortName != ep->opname) {
            csound->Free(csound, shortName);
          }
        }
        items = items->next;
      }
      head = head->next;
    }
    csound->Free(csound, top);
    }
}

void add_to_symbtab(CSOUND *csound, OENTRY *ep) {
  if (csound->symbtab != NULL) {
   char *shortName;
   if (ep->dsblksiz < 0xfffb) {
          shortName = get_opcode_short_name(csound, ep->opname);
          add_token(csound, shortName, get_opcode_type(ep));
          if (shortName != ep->opname) {
            csound->Free(csound, shortName);
          }
          csound->DebugMsg(csound, "opcode %s added to symbtab\n", ep->opname);
   }
  }
}

ORCTOKEN *add_token(CSOUND *csound, char *s, int type)
{
    //printf("Hash value for %s: %i\n", s, h);

    ORCTOKEN *a = cs_hash_table_get(csound, csound->symbtab, s);

    ORCTOKEN *ans;
    if (a!=NULL) {
      if (type == a->type) return a;
      if (UNLIKELY((type!=T_FUNCTION || a->type!=T_OPCODE)))
        csound->Warning(csound,
                        Str("Type confusion for %s (%d,%d), replacing\n"),
                        s, type, a->type);
      a->type = type;
      return a;
    }
    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)csound->Malloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);
    ans->type = type;

    cs_hash_table_put(csound, csound->symbtab, s, ans);

    return ans;
}

/* static ORCTOKEN *add_token_p(CSOUND *csound, char *s, int type, int val) */
/* { */
/*     ORCTOKEN *ans = add_token(csound, s, type); */
/*     ans->value = val; */
/*     return ans; */
/* } */

int isUDOArgList(char *s)
{
    int len = strlen(s) - 1;

    while (len >= 0) {
      if (UNLIKELY(strchr("aijkftKOJVPopS[]0", s[len]) == NULL)) {
        /* printf("Invalid char '%c' in '%s'", *p, s); */
        return 0;
      }
      len--;
    }
    return 1;
}

int isUDOAnsList(char *s)
{
    int len = strlen(s) - 1;

    while (len >= 0) {
      if (UNLIKELY(strchr("aikftSK[]0", s[len]) == NULL)) {
        return 0;
      }
      len--;
    }
    return 1;
}

ORCTOKEN *lookup_token(CSOUND *csound, char *s, void *yyscanner)
{
    IGN(yyscanner);
    int type = T_IDENT;
    ORCTOKEN *a;
    ORCTOKEN *ans;

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Looking up token for: %s\n", s);

    if (udoflag == 0) {
      if (isUDOAnsList(s)) {
        ans = new_token(csound, UDO_ANS_TOKEN);
        ans->lexeme = (char*)csound->Malloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        return ans;
      }
    }

    if (udoflag == 1) {
      if (UNLIKELY(csound->oparms->odebug)) printf("Found UDO Arg List\n");
      if (isUDOArgList(s)) {
        ans = new_token(csound, UDO_ARGS_TOKEN);
        ans->lexeme = (char*)csound->Malloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        return ans;
      }
    }

    a = cs_hash_table_get(csound, csound->symbtab, s);

    if (a != NULL) {
      ans = (ORCTOKEN*)csound->Malloc(csound, sizeof(ORCTOKEN));
      memcpy(ans, a, sizeof(ORCTOKEN));
      ans->next = NULL;
      ans->lexeme = (char *)csound->Malloc(csound, strlen(a->lexeme) + 1);
      strcpy(ans->lexeme, a->lexeme);
      return ans;
    }

    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)csound->Malloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);

    if (udoflag == -2 || namedInstrFlag == 1) {
        return ans;
    }

    ans->type = type;

    return ans;
}


//static int is_optional_udo_in_arg(char* argtype) {
//    return strchr("jOPVop", *argtype) != NULL;
//}

static char map_udo_in_arg_type(char in) {
    if (strchr("ijop", in) != NULL) {
        return 'i';
    } else if (strchr("kKOJPV", in) != NULL) {
        return 'k';
    }
    return in;
}

static char map_udo_out_arg_type(char in) {
    if (in == 'K') {
        return 'k';
    }
    return in;
}

static void map_args(char* args) {
    while (*args != '\0') {
      *args = map_udo_out_arg_type(*args);
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

*/

static int parse_opcode_args(CSOUND *csound, OENTRY *opc)
{
    OPCODINFO   *inm = (OPCODINFO*) opc->useropinfo;
    char** in_args;
    char** out_args;
    char intypes[256];
    char typeSpecifier[2];
    char tempName[20];
    int i = 0, err = 0;
    int n=0;

    ARRAY_VAR_INIT varInit;

    typeSpecifier[1] = '\0';

    // The following handles adding of extra 'o' type for optional
    // ksmps arg for all UDO's
    if (*inm->intypes == '0') {
        intypes[0] = 'o';
        intypes[1] = '\0';
    } else {
        snprintf(intypes, 256, "%so", inm->intypes);
    }
    in_args = splitArgs(csound, intypes);
    out_args = splitArgs(csound, inm->outtypes);

    if (UNLIKELY(in_args == NULL)) {
      synterr(csound,
              Str("invalid input argument types found for opcode %s: %s\n"),
              inm->name, intypes);
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
        snprintf(tempName, 20, "in%d", i);

        if (*in_arg == '[') {
          int dimensions = 0;
          while (*in_arg == '[') {
            dimensions += 1;
            in_arg += 1;
          }
          typeSpecifier[0] = *in_arg;
// printf("Dimensions: %d SubArgType: %s\n", dimensions, typeSpecifier);
          CS_TYPE* type =
            csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid input type for opcode %s\n"), in_arg);
            err++;
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
          char c = map_udo_in_arg_type(*in_arg);
          //                printf("found arg type %s -> %c\n", in_arg, c);

          typeSpecifier[0] = c;
          CS_TYPE* type =
            csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid input type for opcode %s\n"), in_arg);
            err++;
            continue;
          }

          CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,
                                                  type, tempName, NULL);
          csoundAddVariable(csound, inm->in_arg_pool, var);
        }
        i++;
      }
    }
//    inm->inchns = i + 1; /* Add one for optional local ksmps */
    inm->inchns = i - 1;

    i = 0;
    if (*out_args[0] != '0') {
      while(out_args[i] != NULL) {
        char* out_arg = out_args[i];
        snprintf(tempName, 20, "out%d", i);

        if (*out_arg == '[') {
          int dimensions = 0;
          while (*out_arg == '[') {
            dimensions += 1;
            out_arg += 1;
          }
          typeSpecifier[0] = *out_arg;
          //printf("Dimensions: %d SubArgType: %s\n", dimensions, typeSpecifier);
          CS_TYPE* type =
            csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid output type for opcode %s"), out_arg);
            err++;
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
          char c = map_udo_out_arg_type(*out_arg);
          //                printf("found arg type %s -> %c\n", out_arg, c);
          typeSpecifier[0] = c;
          CS_TYPE* type =
            csoundGetTypeWithVarTypeName(csound->typePool, typeSpecifier);

          if (UNLIKELY(type == NULL)) {
            synterr(csound, Str("invalid output type for opcode %s"), out_arg);
            err++;
            continue;
          }

          CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool, type,
                                                  tempName, NULL);
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


    opc->intypes = cs_strdup(csound, intypes);
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
        if (oentry->iopadr == ep->iopadr &&
            oentry->kopadr == ep->kopadr &&
            oentry->aopadr == ep->aopadr &&
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
        char *outtypes, char *intypes) {

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
      char* adjusted_intypes = csound->Malloc(csound, sizeof(char) * (len + 2));
      sprintf(adjusted_intypes, "%so", intypes);
      opc = find_opcode_exact(csound, opname, outtypes, adjusted_intypes);
      csound->Free(csound, adjusted_intypes);
    }

    /* check if opcode is already defined */
    if (UNLIKELY(opc != NULL)) {
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
      //printf("****New UDO: %s %s %s\n", inm->name, inm->outtypes, inm->intypes);
      /* IV - Oct 31 2002: */
      /* create a fake opcode so we can call it as such */
      opc = find_opcode(csound, "##userOpcode");
      memcpy(&tmpEntry, opc, sizeof(OENTRY));
      tmpEntry.opname = cs_strdup(csound, opname);

      csound->AppendOpcodes(csound, &tmpEntry, 1);
      newopc = csound_find_internal_oentry(csound, &tmpEntry);

      newopc->useropinfo = (void*) inm; /* ptr to opcode parameters */

      /* check in/out types and copy to the opcode's */
      /* IV - Sep 8 2002: opcodes have an optional arg for ksmps */
      newopc->outypes = csound->Malloc(csound, strlen(outtypes) + 1
                                       + strlen(intypes) + 2);
      newopc->intypes = &(newopc->outypes[strlen(outtypes) + 1]);

      if (strcmp(outtypes, "0")==0) {
        add_token(csound, opname, T_OPCODE0);
      } else {
        add_token(csound, opname, T_OPCODE);
      }

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
