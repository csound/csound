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
#include "csound_orcparse.h"
#include "insert.h"
#include "namedins.h"
#include "interlocks.h"

#ifndef PARSER_DEBUG
#define PARSER_DEBUG (0)
#endif

ORCTOKEN** symbtab;

#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag

ORCTOKEN *add_token(CSOUND *csound, char *s, int type);
static ORCTOKEN *add_token_p(CSOUND *csound, char *s, int type, int val);

int get_opcode_type(OENTRY *ep)
{
    int retVal = 0;

//    if ((ep->outypes == NULL || strlen(ep->outypes) == 0) &&
//       (ep->intypes == NULL || strlen(ep->intypes) == 0)) {
//        retVal = T_OPCODE00;
//    } else

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

    symbtab = (ORCTOKEN**)mcalloc(csound, HASH_SIZE * sizeof(ORCTOKEN*));
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

//        if (strchr(ep->opname, '.') != NULL) {
//           csound->Message(csound,
//                   "Found PolyMorphic Opcode Definition %s\n",ep->opname);
//        }

        }
        else {
//            csound->Message(csound, "Found Regular Opcode %s\n",ep->opname);
          add_token(csound, ep->opname,get_opcode_type(ep));
        }


    }


    /* This adds all the T_FUNCTION tokens.  These should only be
     * looked up in context when parsing a expression list (not yet done)
     * and perhaps a more intelligent way needs to be done eventually to
     * look up opcodes which can serve as functions, as well as for allowing
     * multiple arguments to functions
     */
    add_token(csound, "int", T_FUNCTION);
    add_token(csound, "frac", T_FUNCTION);
    add_token(csound, "round", T_FUNCTION);
    add_token(csound, "floor", T_FUNCTION);
    add_token(csound, "ceil", T_FUNCTION);
    add_token(csound, "rnd", T_FUNCTION);
    add_token(csound, "birnd", T_FUNCTION);
    add_token(csound, "abs", T_FUNCTION);
    add_token(csound, "exp", T_FUNCTION);
    add_token(csound, "log", T_FUNCTION);
    add_token(csound, "sqrt", T_FUNCTION);
    add_token(csound, "sin", T_FUNCTION);
    add_token(csound, "cos", T_FUNCTION);
    add_token(csound, "tan", T_FUNCTION);
    add_token(csound, "sininv", T_FUNCTION);
    add_token(csound, "cosinv", T_FUNCTION);
    add_token(csound, "taninv", T_FUNCTION);
    add_token(csound, "log10", T_FUNCTION);
    add_token(csound, "sinh", T_FUNCTION);
    add_token(csound, "cosh", T_FUNCTION);
    add_token(csound, "tanh", T_FUNCTION);
    add_token(csound, "ampdb", T_FUNCTION);
    add_token(csound, "ampdbfs", T_FUNCTION);
    add_token(csound, "dbamp", T_FUNCTION);
    add_token(csound, "dbfsamp", T_FUNCTION);
    add_token_p(csound, "ftlen", T_FUNCTION, TR);
    add_token_p(csound, "ftsr", T_FUNCTION, TR);
    add_token_p(csound, "ftlptim", T_FUNCTION, TR);
    add_token_p(csound, "ftchnls", T_FUNCTION, TR);
    add_token(csound, "i", T_FUNCTION);
    add_token(csound, "k", T_FUNCTION);
    add_token(csound, "cpsoct", T_FUNCTION);
    add_token(csound, "octpch", T_FUNCTION);
    add_token(csound, "cpspch", T_FUNCTION);
    add_token(csound, "pchoct", T_FUNCTION);
    add_token(csound, "octcps", T_FUNCTION);
    add_token_p(csound, "nsamp", T_FUNCTION, TR);
    add_token(csound, "powoftwo", T_FUNCTION);
    add_token(csound, "logbtwo", T_FUNCTION);
    add_token(csound, "a", T_FUNCTION);
    add_token_p(csound, "tb0", T_FUNCTION, TR);
    add_token_p(csound, "tb1", T_FUNCTION, TR);
    add_token_p(csound, "tb2", T_FUNCTION, TR);
    add_token_p(csound, "tb3", T_FUNCTION, TR);
    add_token_p(csound, "tb4", T_FUNCTION, TR);
    add_token_p(csound, "tb5", T_FUNCTION, TR);
    add_token_p(csound, "tb6", T_FUNCTION, TR);
    add_token_p(csound, "tb7", T_FUNCTION, TR);
    add_token_p(csound, "tb8", T_FUNCTION, TR);
    add_token_p(csound, "tb9", T_FUNCTION, TR);
    add_token_p(csound, "tb10", T_FUNCTION, TR);
    add_token_p(csound, "tb11", T_FUNCTION, TR);
    add_token_p(csound, "tb12", T_FUNCTION, TR);
    add_token_p(csound, "tb13", T_FUNCTION, TR);
    add_token_p(csound, "tb14", T_FUNCTION, TR);
    add_token_p(csound, "tb15", T_FUNCTION, TR);
    add_token_p(csound, "urd", T_FUNCTION, TR);
    add_token(csound, "not", T_FUNCTION);
    add_token(csound, "cent", T_FUNCTION);
    add_token(csound, "octave", T_FUNCTION);
    add_token(csound, "semitone", T_FUNCTION);
    add_token(csound, "cpsmidinn", T_FUNCTION);
    add_token(csound, "octmidinn", T_FUNCTION);
    add_token(csound, "pchmidinn", T_FUNCTION);
}

static unsigned int hash(char *s)
{
    unsigned int h = 0;
    while (*s != '\0') {
      h = (h<<4) ^ *s++;
    }
    return (h%HASH_SIZE);
}

ORCTOKEN *add_token(CSOUND *csound, char *s, int type)
{
    unsigned int h = hash(s);

    //printf("Hash value for %s: %i\n", s, h);

    ORCTOKEN *a = symbtab[h];
    ORCTOKEN *ans;
    while (a!=NULL) {
      if (strcmp(a->lexeme, s)==0) {
        if (type == a->type) return a;
        if (type!= T_FUNCTION || a->type!=T_OPCODE) 
          csound->Warning(csound,
                          Str("Type confusion for %s (%d,%d), replacing\n"),
                          s, type, a->type);
        a->type = type;
        return a;
      }
      a = a->next;
    }
    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);
    ans->next = symbtab[h];
    ans->type = type;

    symbtab[h] = ans;
    return ans;
}

static ORCTOKEN *add_token_p(CSOUND *csound, char *s, int type, int val)
{
    ORCTOKEN *ans = add_token(csound, s, type);
    ans->value = val;
    return ans;
}
    
int isUDOArgList(char *s)
{
    int len = strlen(s) - 1;

    while (len >= 0) {
      if (UNLIKELY(strchr("aijkftKopS0", s[len]) == NULL)) {
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
      if (UNLIKELY(strchr("aikftSK0", s[len]) == NULL)) {
        return 0;
      }
      len--;
    }
    return 1;
}

ORCTOKEN *lookup_token(CSOUND *csound, char *s, void *yyscanner)
{
    unsigned int h = hash(s);
    int type = T_IDENT;
    ORCTOKEN *a = symbtab[h];
    ORCTOKEN *ans;

    if (PARSER_DEBUG)
      csound->Message(csound, "Looking up token for: %d : %s\n", h, s);

    if (udoflag == 0) {
      if (isUDOAnsList(s)) {
        ans = new_token(csound, T_UDO_ANS);
        ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        ans->next = symbtab[h];
        symbtab[h] = ans;
        //printf("Found UDO Answer List\n");
        return ans;
      }
    }

    if (udoflag == 1) {
      if (isUDOArgList(s)) {
        ans = new_token(csound, T_UDO_ARGS);
        ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        ans->next = symbtab[h];
        symbtab[h] = ans;
        //printf("Found UDO Arg List\n");
        return ans;
      }
    }

    while (a!=NULL) {
      if (strcmp(s, "reverb") == 0) {
        if (PARSER_DEBUG)
          csound->Message(csound, "Looking up token for: %d: %d: %s : %s\n",
                          hash("reverb"), hash("a4"), s, a->lexeme);
      }
      if (strcmp(a->lexeme, s)==0) {
        ans = (ORCTOKEN*)mmalloc(csound, sizeof(ORCTOKEN));
        memcpy(ans, a, sizeof(ORCTOKEN));
        ans->next = NULL;
        ans->lexeme = (char *)mmalloc(csound, strlen(a->lexeme) + 1);
        strcpy(ans->lexeme, a->lexeme);

        return ans;
      }
      a = a->next;
    }


    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);
    //ans->next = symbtab[h];

    /* if (PARSER_DEBUG) */
    /*   csound->Message(csound, "NamedInstrFlag: %d\n", namedInstrFlag); */

    if (udoflag == -2 || namedInstrFlag == 1) {
        return ans;
    }

    // NEED TO FIX: In case of looking for label for kgoto or other opcodes, need
    // to return T_IDENT instead of any sub-type
    // Currently fixed by definition of label non-terminal
    switch (s[0]) {
    case 'S': type = T_IDENT_S; break;
    case 'a': type = T_IDENT_A; break;
    case 'f': type = T_IDENT_F; break;
    case 'i': type = T_IDENT_I; break;
    case 'k': type = T_IDENT_K; break;
    case 'p': type = T_IDENT_P; break;
    case 't': type = T_IDENT_T; break;
    case 'w': type = T_IDENT_W; break;
    case'g':
      switch (s[1]) {
      case 'i': type = T_IDENT_GI; break;
      case 'k': type = T_IDENT_GK; break;
      case 'a': type = T_IDENT_GA; break;
      case 'f': type = T_IDENT_GF; break;
      case 'w': type = T_IDENT_GW; break;
      case 't': type = T_IDENT_GT; break;
      case 'S': type = T_IDENT_GS; break;
      default: 
        csound->Message(csound, Str("Unknown word type for %s on line %d\n"),
                        s, csound_orcget_lineno(yyscanner));
        exit(1);
      }
    default: /*
      printf("IDENT Token: %i : %i", ans->type, T_IDENT);
      printf("Unknown word type for %s on line %d\n", s, yyline);
      exit(1);
             */
      break;
    }
    ans->type = type;
    //symbtab[h] = ans;

    return ans;
}




/* UDO code below was from otran, broken out and modified for new parser by
 * SYY
 */
/* VL -- I have made the modifications below to allow for f-sigs & t-sigs and on line 224 and 238*/

/* IV - Oct 12 2002: new function to parse arguments of opcode definitions */
static int parse_opcode_args(CSOUND *csound, OENTRY *opc)
{
    OPCODINFO   *inm = (OPCODINFO*) opc->useropinfo;
    char    *types, *otypes;
    int     i, i_incnt, a_incnt, k_incnt, i_outcnt, a_outcnt, k_outcnt, err;
    int     S_incnt, S_outcnt, f_outcnt, f_incnt, t_incnt, t_outcnt;
    int16   *a_inlist, *k_inlist, *i_inlist, *a_outlist, *k_outlist, *i_outlist;
    int16   *S_inlist, *S_outlist, *f_inlist, *f_outlist, *t_inlist, *t_outlist;
  
    /* count the number of arguments, and check types */
    i = i_incnt = S_incnt = a_incnt = k_incnt = f_incnt = f_outcnt =
        i_outcnt = S_outcnt = a_outcnt = k_outcnt = t_incnt = t_outcnt = err = 0;
    types = inm->intypes; otypes = opc->intypes;
    opc->dsblksiz = (uint16) sizeof(UOPCODE);
    if (!strcmp(types, "0"))
      types++;                  /* no input args */
    while (*types) {
      switch (*types) {
      case 'a':
        a_incnt++; *otypes++ = *types;
        break;
      case 'K':
        i_incnt++;              /* also updated at i-time */
      case 'k':
        k_incnt++; *otypes++ = 'k';
        break;
      case 'f':
        f_incnt++; *otypes++ = *types;
        break;
      case 't':
        t_incnt++; *otypes++ = *types;
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
    inm->perf_incnt = a_incnt + k_incnt + f_incnt + t_incnt;
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
      case 't':
        t_outcnt++; *otypes++ = *types;
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
    inm->perf_outcnt = a_outcnt + k_outcnt + f_outcnt + t_outcnt;
    opc->dsblksiz += (uint16) (sizeof(MYFLT*) * i);
    opc->dsblksiz = ((opc->dsblksiz + (uint16) 15)
                     & (~((uint16) 15)));   /* align (needed ?) */
    /* now build index lists for the various types of arguments */
    i = i_incnt + S_incnt + inm->perf_incnt +
        i_outcnt + S_outcnt + inm->perf_outcnt;
    i_inlist = inm->in_ndx_list = (int16*) mmalloc(csound,
                                                   sizeof(int16) * (i + 14));
    S_inlist = i_inlist + i_incnt + 1;
    a_inlist = S_inlist + S_incnt + 1;
    k_inlist = a_inlist + a_incnt + 1;
    f_inlist = k_inlist + k_incnt + 1;
    t_inlist = f_inlist + f_incnt + 1;
    i = 0; types = inm->intypes;
    while (*types) {
      switch (*types++) {
        case 'a': *a_inlist++ = i; break;
        case 'k': *k_inlist++ = i; break;
        case 'f': *f_inlist++ = i; break;
        case 't': *t_inlist++ = i; break;
        case 'K': *k_inlist++ = i;      /* also updated at i-time */
        case 'i':
        case 'o':
        case 'p':
        case 'j': *i_inlist++ = i; break;
        case 'S': *S_inlist++ = i; break;
      }
      i++;
    }
      
    *i_inlist = *S_inlist = *a_inlist = *k_inlist = *f_inlist = *t_inlist = -1;     /* put delimiters */
    i_outlist = inm->out_ndx_list = t_inlist + 1;
    S_outlist = i_outlist + i_outcnt + 1;
    a_outlist = S_outlist + S_outcnt + 1;
    k_outlist = a_outlist + a_outcnt + 1;
    f_outlist = k_outlist + k_outcnt + 1;
    t_outlist = f_outlist + f_outcnt + 1;
    i = 0; types = inm->outtypes;
    while (*types) {
      switch (*types++) {
        case 'a': *a_outlist++ = i; break;
        case 'k': *k_outlist++ = i; break;
        case 'f': *f_outlist++ = i; break;
        case 't': *t_outlist++ = i; break;
        case 'K': *k_outlist++ = i;     /* also updated at i-time */
        case 'i': *i_outlist++ = i; break;
        case 'S': *S_outlist++ = i; break;
      }
      i++;
    }
    
    *i_outlist = *S_outlist = *a_outlist = *k_outlist = *f_outlist = *t_outlist = -1;  /* put delimiters */
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
    newopnum = find_opcode(csound, opname);

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
    opc = csound->opcodlst + find_opcode(csound, ".userOpcode");
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
