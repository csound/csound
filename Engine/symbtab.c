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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

#ifndef PARSER_DEBUG
#define PARSER_DEBUG (0)
#endif

// FIXME - this is global...
CS_HASH_TABLE* symbtab;

#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag

ORCTOKEN *add_token(CSOUND *csound, char *s, int type);
//static ORCTOKEN *add_token_p(CSOUND *csound, char *s, int type, int val);
extern int csound_orcget_lineno(void*);
extern int find_opcode_num(CSOUND* csound, char* opname,
                           char* outArgsFound, char* inArgsFound);

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
    OENTRY *temp;
    int len = 0;

    symbtab = cs_hash_table_create(csound);
    /* Now we need to populate with basic words */
    /* Add token types for opcodes to symbtab.  If a polymorphic opcode
     * definition is found (dsblksiz >= 0xfffb), look for implementations
     * of that opcode to correctly mark the type of opcode it is (T_OPCODE,
     * T_OPCODE0, or T_OPCODE00)
     */

    for (ep = (OENTRY*) csound->opcodlst; ep < (OENTRY*) csound->oplstend; ep++) {
        if (ep->dsblksiz >= 0xfffb) {
          char * polyName;
          len = strlen(ep->opname) + 1;
          polyName = mcalloc(csound, len + 1);
          sprintf(polyName, "%s.", ep->opname);

          for (temp = (OENTRY*) csound->opcodlst;
               temp < (OENTRY*) csound->oplstend; temp++) {
            if (ep != temp && strncmp(polyName, temp->opname, len) == 0) {
              add_token(csound, ep->opname, get_opcode_type(temp));
            }
          }

          mfree(csound, polyName);

        }
        else {
//            csound->Message(csound, "Found Regular Opcode %s\n",ep->opname);
          add_token(csound, ep->opname,get_opcode_type(ep));
        }


    }

}

ORCTOKEN *add_token(CSOUND *csound, char *s, int type)
{
    //printf("Hash value for %s: %i\n", s, h);

    ORCTOKEN *a = cs_hash_table_get(csound, symbtab, s);

    ORCTOKEN *ans;
    if (a!=NULL) {
      if (type == a->type) return a;
      if (type!= T_FUNCTION || a->type!=T_OPCODE)
        csound->Warning(csound,
                        Str("Type confusion for %s (%d,%d), replacing\n"),
                        s, type, a->type);
      a->type = type;
      return a;
    }
    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);
    ans->type = type;

    cs_hash_table_put(csound, symbtab, s, ans);

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
      if (UNLIKELY(strchr("aijkftKOVPopS[]0", s[len]) == NULL)) {
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
    int type = T_IDENT;
    ORCTOKEN *a;
    ORCTOKEN *ans;

    if (PARSER_DEBUG)
      csound->Message(csound, "Looking up token for: %s\n", s);

    if (udoflag == 0) {
      if (isUDOAnsList(s)) {
        ans = new_token(csound, UDO_ANS_TOKEN);
        ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        return ans;
      }
    }

    if (udoflag == 1) {
      printf("Found UDO Arg List\n");
      if (isUDOArgList(s)) {
        ans = new_token(csound, UDO_ARGS_TOKEN);
        ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        return ans;
      }
    }

    a = cs_hash_table_get(csound, symbtab, s);

    if (a != NULL) {
      ans = (ORCTOKEN*)mmalloc(csound, sizeof(ORCTOKEN));
      memcpy(ans, a, sizeof(ORCTOKEN));
      ans->next = NULL;
      ans->lexeme = (char *)mmalloc(csound, strlen(a->lexeme) + 1);
      strcpy(ans->lexeme, a->lexeme);
      return ans;
    }

    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);

    if (udoflag == -2 || namedInstrFlag == 1) {
        return ans;
    }

    ans->type = type;

    return ans;
}


/**
 *
  This function takes in the arguments from useropinfo in OENTRY and parses
  them, filling the OENTRY input and output types and creating
  the argument lists for xinset/xouset in insert.c
  argument pointerlists, stored in useropinfo->in_ndx_list and
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
    char    *types, *otypes;
    int     i, i_incnt, iv_incnt, iv_outcnt, a_incnt, k_incnt,
            i_outcnt, a_outcnt, k_outcnt, err;
    int     S_incnt, S_outcnt, f_outcnt, f_incnt, kv_incnt, kv_outcnt;
    int16   *a_inlist, *k_inlist, *i_inlist, *a_outlist, *k_outlist, *i_outlist;
    int16   *S_inlist, *S_outlist, *f_inlist, *f_outlist, *kv_inlist,
            *kv_outlist, *iv_inlist, *iv_outlist;

    /* count the number of arguments, and check types */
    i = i_incnt = S_incnt = a_incnt = k_incnt = f_incnt = f_outcnt =
        i_outcnt = S_outcnt = a_outcnt = k_outcnt = kv_incnt =
        kv_outcnt = iv_outcnt = iv_incnt = err = 0;
    types = inm->intypes; otypes = opc->intypes;
    opc->dsblksiz = (uint16) sizeof(UOPCODE);
    if (!strcmp(types, "0"))
      types++;                  /* no input args */
    while (*types) {
      switch (*types) {
      case 'a':
        a_incnt++; *otypes++ = *types;
        break;
      case 'O':
        k_incnt++; *otypes++ = 'O'; break;
      case 'P':
         k_incnt++;*otypes++ = 'P'; break;
      case 'V':
         k_incnt++;*otypes++ = 'V'; break;
      case 'K':
        i_incnt++;              /* also updated at i-time */
      case 'k':
        k_incnt++; *otypes++ = 'k';
        break;
      case 'f':
        f_incnt++; *otypes++ = *types;
        break;
      case '[':
        types++;
        if(*types=='i') iv_incnt++;
         else kv_incnt++;
          *otypes++ = *(types-1);
          *otypes++ = *(types);*otypes++ = *(types+1);
        types++;
        break;
      case 'i':
      case 'o':
      case 'p':
      case 'j':
        i_incnt++; *otypes++ = *types;
       break;
      case 'S':
        S_incnt++; *otypes++ = *types;
        break;
      default:
        synterr(csound, Str("invalid input type for opcode %s"), inm->name);
        err++; i--;
      }
      i++; types++;
      if (UNLIKELY(i > OPCODENUMOUTS_MAX)) {
        synterr(csound, Str("too many input args for opcode %s"), inm->name);
        csound->LongJmp(csound, 1);
      }
    }
    *otypes++ = 'o'; *otypes = '\0';    /* optional arg for local ksmps */
    inm->inchns = i;                    /* total number of input chnls */
    inm->perf_incnt = a_incnt + k_incnt + f_incnt + kv_incnt;
    opc->dsblksiz += (uint16) (sizeof(MYFLT*) * i);
    /* same for outputs */
    i = 0;
    types = inm->outtypes; otypes = opc->outypes;
    if (!strcmp(types, "0"))
      types++;                  /* no output args */
    while (*types) {
      if (UNLIKELY(i >= OPCODENUMOUTS_MAX)) {
        synterr(csound, Str("too many output args for opcode %s"), inm->name);
        csound->LongJmp(csound, 1);
      }
      switch (*types) {
      case 'a':
        a_outcnt++; *otypes++ = *types;
        break;
      case 'K':
        i_outcnt++;             /* also updated at i-time */
      case 'k':
        k_outcnt++; *otypes++ = 'k';
        break;
      case 'f':
        f_outcnt++; *otypes++ = *types;
        break;
      case '[':
        types++;
        if(*types == 'i') iv_outcnt++;
        else kv_outcnt++;
        *otypes++ = *(types-1);
          *otypes++ = *(types); *otypes++ = *(types+1);
        types++;
        break;
      case 'i':
        i_outcnt++; *otypes++ = *types;
        break;
      case 'S':
        S_outcnt++; *otypes++ = *types;
        break;
      default:
        synterr(csound, Str("invalid output type for opcode %s"), inm->name);
        err++; i--;
      }
      i++; types++;
    }
    *otypes = '\0';
    inm->outchns = i;                   /* total number of output chnls */
    inm->perf_outcnt = a_outcnt + k_outcnt + f_outcnt + kv_outcnt;

    opc->dsblksiz += (uint16) (sizeof(MYFLT*) * i);
    opc->dsblksiz = ((opc->dsblksiz + (uint16) 15)
                     & (~((uint16) 15)));   /* align (needed ?) */
    /* now build index lists for the various types of arguments */
    i = i_incnt + S_incnt + inm->perf_incnt + iv_incnt +
        i_outcnt + S_outcnt + inm->perf_outcnt + iv_outcnt;
    i_inlist = inm->in_ndx_list = (int16*) mmalloc(csound,
                                                   sizeof(int16) * (i + 16));
    S_inlist = i_inlist + i_incnt + 1;
    iv_inlist =  S_inlist + S_incnt + 1;
    a_inlist = iv_inlist + iv_incnt + 1;
    k_inlist = a_inlist + a_incnt + 1;
    f_inlist = k_inlist + k_incnt + 1;
    kv_inlist = f_inlist + f_incnt + 1;
    i = 0; types = inm->intypes;
    while (*types) {

      switch (*types++) {

        case 'a': *a_inlist++ = i; break;
        case 'O':
        case 'P':
        case 'V':
        case 'k': *k_inlist++ = i;

        break;
        case 'f': *f_inlist++ = i; break;
        case '[':
          if(*types=='i') *iv_inlist++ = i;
          else *kv_inlist++ = i;
          types+=2;
          break;
        case 'K': *k_inlist++ = i;      /* also updated at i-time */
        case 'i':
        case 'o':
        case 'p':
        case 'j': *i_inlist++ = i; break;
        case 'S': *S_inlist++ = i; break;
      }
      i++;
    }

    /* put delimiters */
    *i_inlist = *S_inlist = *iv_inlist = *a_inlist = *k_inlist =
      *f_inlist = *kv_inlist = -1;

    i_outlist = inm->out_ndx_list = kv_inlist + 1;
    S_outlist = i_outlist + i_outcnt + 1;
    iv_outlist =  S_outlist + S_outcnt + 1;
    a_outlist = iv_outlist + iv_outcnt + 1;
    k_outlist = a_outlist + a_outcnt + 1;
    f_outlist = k_outlist + k_outcnt + 1;
    kv_outlist = f_outlist + f_outcnt + 1;
    i = 0; types = inm->outtypes;
    while (*types) {
      switch (*types++) {
        case 'a': *a_outlist++ = i; break;
        case 'k': *k_outlist++ = i; break;
        case 'f': *f_outlist++ = i; break;
        case '[':
          if(*types=='i') *iv_outlist++ = i;
          else *kv_outlist++ = i;
          types+=2; break;
        case 'K': *k_outlist++ = i;     /* also updated at i-time */
        case 'i': *i_outlist++ = i; break;
        case 'S': *S_outlist++ = i; break;
      }
      i++;
    }

    *i_outlist = *S_outlist = *iv_outlist = *a_outlist = *k_outlist =
      *f_outlist = *kv_outlist = -1;  /* put delimiters */
    return err;
}


/** Adds a UDO definition as an T_OPCODE or T_OPCODE0 type to the symbol table
 * used at parse time.  An OENTRY is also added at this time so that at
 * verification time the opcode can be looked up to get its signature.
 */
int add_udo_definition(CSOUND *csound, char *opname,
        char *outtypes, char *intypes) {

    OENTRY    tmpEntry, *opc, *newopc;
    int32      newopnum;
    OPCODINFO *inm;


    /* IV - Oct 31 2002 */
    if (UNLIKELY(!check_instr_name(opname))) {
        synterr(csound, Str("invalid name for opcode"));
        return -1;
    }

    /* IV - Oct 31 2002: check if opcode is already defined */
    newopnum = find_opcode_num(csound, opname, outtypes, intypes);

    if (newopnum) {
        /* IV - Oct 31 2002: redefine old opcode if possible */
      if (UNLIKELY(newopnum < SETEND || !strcmp(opname, "subinstr"))) {
          synterr(csound, Str("cannot redefine %s"), opname);
          return -2;
        }

        csound->Message(csound,
                        Str("WARNING: redefined opcode: %s\n"), opname);
    }

    /* IV - Oct 31 2002 */
    /* store the name in a linked list (note: must use mcalloc) */
    inm = (OPCODINFO *) mcalloc(csound, sizeof(OPCODINFO));
    inm->name = (char*)mmalloc(csound, 1+strlen(opname));
    strcpy(inm->name, opname);
    inm->intypes = intypes;
    inm->outtypes = outtypes;

    inm->prv = csound->opcodeInfo;
    csound->opcodeInfo = inm;

    /* IV - Oct 31 2002: */
    /* create a fake opcode so we can call it as such */
    opc = &csound->opcodlst[USEROPCODE];
    memcpy(&tmpEntry, opc, sizeof(OENTRY));
    tmpEntry.opname = (char*)mmalloc(csound, 1+strlen(opname));
    strcpy(tmpEntry.opname, opname);
    csound->AppendOpcodes(csound, &tmpEntry, 1);

    if (!newopnum) {
        newopnum = (int32) ((OENTRY*) csound->oplstend
                           - (OENTRY*) csound->opcodlst) - 1L;
    }

    newopc = &(csound->opcodlst[newopnum]);
    newopc->useropinfo = (void*) inm; /* ptr to opcode parameters */

    /* check in/out types and copy to the opcode's */
    /* IV - Sep 8 2002: opcodes have an optional arg for ksmps */
    newopc->outypes = mmalloc(csound, strlen(outtypes) + 1
                                      + strlen(intypes) + 2);
    newopc->intypes = &(newopc->outypes[strlen(outtypes) + 1]);

    if (UNLIKELY(parse_opcode_args(csound, newopc) != 0))
      return -3;

    if (strcmp(outtypes, "0")==0) {
        add_token(csound, opname, T_OPCODE0);
    } else {
        add_token(csound, opname, T_OPCODE);
    }

    return 0;
}

void synterr(CSOUND *csound, const char *s, ...)
{
  va_list args;

  csound->MessageS(csound, CSOUNDMSG_ERROR, Str("error:  "));
  va_start(args, s);
  csound->MessageV(csound, CSOUNDMSG_ERROR, s, args);
  va_end(args);

  /* FIXME - Removed temporarily for debugging
   * This function may not be necessary at all in the end if some of this is
   * done in the parser
   */
  csound->synterrcnt++;
}
