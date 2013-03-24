/*
  csound_orc_compile.c:
  (Based on otran.c)

  Copyright (C) 1991, 1997, 2003, 2006, 2012
  Barry Vercoe, John ffitch, Steven Yi, Victor Lazzarini

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

#include "csoundCore.h"
#include "parse_param.h"
#include "csound_orc.h"
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "oload.h"
#include "insert.h"
#include "pstream.h"
#include "typetabl.h"
#include "csound_standard_types.h"

static  ARG* createArg(CSOUND *csound, INSTRTXT* ip,
                       char *s, ENGINE_STATE *engineState);
static  void    insprep(CSOUND *, INSTRTXT *, ENGINE_STATE *engineState);
static  void    lgbuild(CSOUND *, INSTRTXT *, char *,
                        int inarg, ENGINE_STATE *engineState);
static  void    gblnamset(CSOUND *, char *, ENGINE_STATE *engineState);
static  void    lclnamset(CSOUND *, INSTRTXT* ip, char *);
int     pnum(char *s) ;
static  int     lgexist2(INSTRTXT*, const char *s, ENGINE_STATE *engineState);
static void     unquote_string(char *, const char *);
extern void     print_tree(CSOUND *, char *, TREE *);
extern void delete_tree(CSOUND *csound, TREE *l);
void close_instrument(CSOUND *csound, INSTRTXT * ip);
char argtyp2(char *s);
void debugPrintCsound(CSOUND* csound);

extern int find_opcode_num(CSOUND* csound, char* opname,
                           char* outArgsFound, char* inArgsFound);
extern int find_opcode_num_by_tree(CSOUND* csound, char* opname,
                                   TREE* left, TREE* right);
void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState);
int named_instr_alloc(CSOUND *csound, char *s, INSTRTXT *ip, int32 insno,
                      ENGINE_STATE *engineState);
int check_instr_name(char *s);

/*  removed ; from end of #define as it can mess things */
#define strsav_string(a) string_pool_save_string(csound,csound->stringSavePool,a)

/* NOTE: these assume that sizeof(MYFLT) is either 4 or 8 */
#define Wfloats (((int) sizeof(SPECDAT) + 7) / (int) sizeof(MYFLT))
#define Pfloats (((int) sizeof(PVSDAT) + 7) / (int) sizeof(MYFLT))

#ifdef FLOAT_COMPARE
#undef FLOAT_COMPARE
#endif
#ifdef USE_DOUBLE
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 1.0e-12)
#else
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 5.0e-7)
#endif




/* ------------------------------------------------------------------------ */

static int argCount(ARG* arg)
{
    int retVal = -1;
    if (arg != NULL) {
      retVal = 0;
      while (arg != NULL) {
        arg = arg->next;
        retVal++;
      }
    }
    return retVal;
}

/* get size of string in MYFLT units */
static inline int strlen_to_samples(const char *s)
{
    int n = (int) strlen(s);
    n = (n + (int) sizeof(MYFLT)) / (int) sizeof(MYFLT);
    return n;
}

/* convert string constant */
static void unquote_string(char *dst, const char *src)
{
    int i, j, n = (int) strlen(src) - 1;
    for (i = 1, j = 0; i < n; i++) {
      if (src[i] != '\\')
        dst[j++] = src[i];
      else {
        switch (src[++i]) {
        case 'a':   dst[j++] = '\a';  break;
        case 'b':   dst[j++] = '\b';  break;
        case 'f':   dst[j++] = '\f';  break;
        case 'n':   dst[j++] = '\n';  break;
        case 'r':   dst[j++] = '\r';  break;
        case 't':   dst[j++] = '\t';  break;
        case 'v':   dst[j++] = '\v';  break;
        case '"':   dst[j++] = '"';   break;
        case '\\':  dst[j++] = '\\';  break;
        default:
          if (src[i] >= '0' && src[i] <= '7') {
            int k = 0, l = (int) src[i] - '0';
            while (++k < 3 && src[i + 1] >= '0' && src[i + 1] <= '7')
              l = (l << 3) | ((int) src[++i] - '0');
            dst[j++] = (char) l;
          }
          else {
            dst[j++] = '\\'; i--;
          }
        }
      }
    }
    dst[j] = '\0';
}

int tree_arg_list_count(TREE * root)
{
    int count = 0;
    TREE * current = root;

    while (current != NULL) {
      current = current->next;
      count++;
    }
    return count;
}

/**
 * Returns last OPTXT of OPTXT chain optxt
 */
static OPTXT * last_optxt(OPTXT *optxt)
{
    OPTXT *current = optxt;

    while (current->nxtop != NULL) {
      current = current->nxtop;
    }
    return current;
}

/**
 * Append OPTXT op2 to end of OPTXT chain op1
 */
static inline void append_optxt(OPTXT *op1, OPTXT *op2)
{
    last_optxt(op1)->nxtop = op2;
}

/** Counts number of args in argString, taking into account array identifiers */
PUBLIC int argsRequired(char* argString)
{
    int retVal = 0;
    char* t = argString;

    if (t != NULL) {
      while (*t != '\0') {
        retVal++;
        if (*t == '[') {
          while (*t != ';' && *t != '\0') {
            t++;
          }
        }
        t++;
      }
    }
    return retVal;
}

/** Splits args in argString into char**, taking into account array identifiers */
PUBLIC char** splitArgs(CSOUND* csound, char* argString)
{
    int argCount = argsRequired(argString);
    char** args = mmalloc(csound, sizeof(char**) * (argCount + 1));
    char* t = argString;
    int i = 0;

    if (t != NULL) {
      while (*t != '\0' ) {
        char* part;

        if (*t == '[') {
          int len = 0;
          char* start = t;
          while (*t != ';') {
            t++;
            len++;
          }
          part = mmalloc(csound, sizeof(char) * (len + 2));
          strncpy(part, start, len);
          part[len] = ';';
          part[len + 1] = '\0';

        } else {
          part = mmalloc(csound, sizeof(char) * 2);
          part[0] = *t;
          part[1] = '\0';
        }
        args[i] = part;
        t++;
        i++;
      }
    }

    args[argCount] = NULL;

    return args;
}

void set_xincod(CSOUND *csound, TEXT *tp, OENTRY *ep)
{
    int n = tp->inlist->count;
    char *s;
    int nreqd = argsRequired(ep->intypes);
    char **types = splitArgs(csound, ep->intypes);
    //int lgprevdef = 0;
    char      tfound = '\0', treqd;

    if (n > nreqd) {                 /* IV - Oct 24 2002: end of new code */
      if ((treqd = *types[nreqd-1]) == 'n') {  /* indef args: */
        int incnt = -1;                       /* Should count args */
        if (!(incnt & 01))                    /* require odd */
          synterr(csound, Str("missing or extra arg"));
      }       /* IV - Sep 1 2002: added 'M' */
      else if (treqd != 'm' && treqd != 'z' && treqd != 'y' &&
               treqd != 'Z' && treqd != 'M' && treqd != 'N') /* else any no */
        synterr(csound, Str("too many input args\n"));
    }

    while (n--) {                     /* inargs:   */
      //int32    tfound_m, treqd_m = 0L;
      s = tp->inlist->arg[n];

      if (n >= nreqd) {               /* det type required */
        switch (*types[nreqd-1]) {
        case 'M':
        case 'N':
        case 'Z':
        case 'y':
        case 'z':   treqd = *types[nreqd-1]; break;
        default:    treqd = 'i';    /*   (indef in-type) */
        }
      }
      else treqd = *types[n];          /*       or given)   */
      if (treqd == 'l') {             /* if arg takes lbl  */
        csound->DebugMsg(csound, "treqd = l");
        //        lblrequest(csound, s);        /*      req a search */
        continue;                     /*      chk it later */
      }
      tfound = argtyp2(s);     /* else get arg type */
      /* IV - Oct 31 2002 */
//      tfound_m = STA(typemask_tabl)[(unsigned char) tfound];
//      lgprevdef = lgexist2(csound, s);
//      csound->DebugMsg(csound, "treqd %c, tfound_m %d lgprevdef %d\n",
//                       treqd, tfound_m, lgprevdef);
//      if (!(tfound_m & (ARGTYP_c|ARGTYP_p)) && !lgprevdef && *s != '"') {
//        synterr(csound,
//                Str("input arg '%s' used before defined (in opcode %s),"
//                    " line %d\n"),
//                s, ep->opname, line);
//      }
      if (tfound == 'a' && n < 31) /* JMC added for FOG */
                                   /* 4 for FOF, 8 for FOG; expanded to 15  */
        tp->xincod |= (1 << n);
      if (tfound == 'S' && n < 31)
        tp->xincod_str |= (1 << n);
      /* IV - Oct 31 2002: simplified code */
//      if (!(tfound_m & STA(typemask_tabl_in)[(unsigned char) treqd])) {
//        /* check for exceptional types */
//        switch (treqd) {
//        case 'Z':                             /* indef kakaka ... */
//          if (!(tfound_m & (n & 1 ? ARGTYP_a : ARGTYP_ipcrk)))
//            intyperr(csound, n, s, ep->opname, tfound, treqd, line);
//          break;
//        case 'x':
//          treqd_m = ARGTYP_ipcr;              /* also allows i-rate */
//        case 's':                             /* a- or k-rate */
//          treqd_m |= ARGTYP_a | ARGTYP_k;
//          printf("treqd_m=%d tfound_m=%d tfound=%c count=%d\n",
//                 treqd_m, tfound_m, tfound, tp->outlist->count);
//          if (tfound_m & treqd_m) {
//            if (tfound == 'a' && tp->outlist->count != 0) {
//              long outyp_m =                  /* ??? */
//                STA(typemask_tabl)[(unsigned char) argtyp2(csound,
//                                                    tp->outlist->arg[0])];
//              if (outyp_m & (ARGTYP_a | ARGTYP_w | ARGTYP_f)) break;
//            }
//            else
//              break;
//          }
//        default:
//          intyperr(csound, n, s, ep->opname, tfound, treqd, line);
//          break;
//        }
//      }//
    }
    mfree(csound, types);
    //csound->DebugMsg(csound, "xincod = %d", tp->xincod);
}


void set_xoutcod(CSOUND *csound, TEXT *tp, OENTRY *ep)
{
    int n = tp->outlist->count;
    char *s;
    char **types = splitArgs(csound, ep->outypes);
    //int nreqd = argsRequired(ep->outypes);
    char      tfound = '\0';//, treqd;

//    if (nreqd < 0)    /* for other opcodes */
//      nreqd = argsRequired(types = ep->outypes);
/* if ((n != nreqd) && */        /* IV - Oct 24 2002: end of new code */
/*          !(n > 0 && n < nreqd &&
            (types[n] == (char) 'm' || types[n] == (char) 'z' ||
             types[n] == (char) 'X' || types[n] == (char) 'N' ||
             types[n] == (char) 'F' || types[n] == (char) 'I'))) {
             synterr(csound, Str("illegal no of output args"));
             if (n > nreqd)
             n = nreqd;
             }*/


    while (n--) {                                     /* outargs:  */
      //long    tfound_m;       /* IV - Oct 31 2002 */
      s = tp->outlist->arg[n];
      //treqd = *types[n];
      tfound = argtyp2(s);                     /*  found    */
      /* IV - Oct 31 2002 */
//      tfound_m = STA(typemask_tabl)[(unsigned char) tfound];
      /* IV - Sep 1 2002: xoutcod is the same as xincod for input */
      if (tfound == 'a' && n < 31)
        tp->xoutcod |= (1 << n);
      if (tfound == 'S' && n < 31)
        tp->xoutcod_str |= (1 << n);
      //csound->Message(csound, "treqd %c, tfound %c \n", treqd, tfound);
      /* if (tfound_m & ARGTYP_w) */
      /*   if (STA(lgprevdef)) { */
      /*     synterr(csound, Str("output name previously used, " */
      /*                         "type '%c' must be uniquely defined, line %d"), */
      /*             tfound, line); */
      /*   } */
      /* IV - Oct 31 2002: simplified code */
//      if (!(tfound_m & STA(typemask_tabl_out)[(unsigned char) treqd])) {
//        synterr(csound, Str("output arg '%s' illegal type (for opcode %s),"
//                            " line %d\n"),
//                s, ep->opname, line);
//      }
    }
    mfree(csound, types);
}



/**
 * Create an Opcode (OPTXT) from the AST node given for a given engineState
 */
OPTXT *create_opcode(CSOUND *csound, TREE *root, INSTRTXT *ip,
                     ENGINE_STATE *engineState)
{
    TEXT *tp;
    TREE *inargs, *outargs;
    OPTXT *optxt, *retOptxt = NULL;
    char *arg;
    int opnum;
    int n, nreqd;;
    optxt = (OPTXT *) mcalloc(csound, (int32)sizeof(OPTXT));
    tp = &(optxt->t);

    switch(root->type) {
    case LABEL_TOKEN:
      /* TODO - Need to verify here or elsewhere that this label is not
         already defined */
      tp->opnum = LABEL;
      tp->opcod = strsav_string(root->value->lexeme);

      tp->outlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
      tp->outlist->count = 0;
      tp->inlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
      tp->inlist->count = 0;

      ip->mdepends |= csound->opcodlst[LABEL].flags;
      ip->opdstot += csound->opcodlst[LABEL].dsblksiz;

      break;
    case GOTO_TOKEN:
    case IGOTO_TOKEN:
    case KGOTO_TOKEN:
    case T_OPCODE:
    case T_OPCODE0:
    case '=':
      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound,
                        "create_opcode: Found node for opcode %s\n",
                        root->value->lexeme);

      nreqd = tree_arg_list_count(root->left);   /* outcount */
      /* replace opcode if needed */
      if (!strcmp(root->value->lexeme, "xin") &&
          nreqd > OPCODENUMOUTS_LOW) {
        if (nreqd > OPCODENUMOUTS_HIGH) {
          opnum = find_opcode_num(csound, "##xin256", "i", NULL);
        } else {
          opnum = find_opcode_num(csound, "##xin64", "i", NULL);
        }
      }
      else {
        opnum = find_opcode_num_by_tree(csound, root->value->lexeme,
                                        root->left, root->right);
      }

      /* INITIAL SETUP */
      tp->opnum = opnum;
      tp->opcod = strsav_string(csound->opcodlst[opnum].opname);
      ip->mdepends |= csound->opcodlst[opnum].flags;
      ip->opdstot += csound->opcodlst[opnum].dsblksiz;

      /* BUILD ARG LISTS */
      {
        int incount = tree_arg_list_count(root->right);
        int outcount = tree_arg_list_count(root->left);
        int argcount = 0;
        size_t m = sizeof(ARGLST) + (incount - 1) * sizeof(char*);
        tp->inlist = (ARGLST*) mrealloc(csound, tp->inlist, m);
        tp->inlist->count = incount;

        m = sizeof(ARGLST) + (outcount - 1) * sizeof(char*);
        tp->outlist = (ARGLST*) mrealloc(csound, tp->outlist, m);
        tp->outlist->count = outcount;


        for (inargs = root->right; inargs != NULL; inargs = inargs->next) {
          /* INARGS */
          arg = inargs->value->lexeme;
          tp->inlist->arg[argcount++] = strsav_string(arg);

          if ((n = pnum(arg)) >= 0) {
            if (n > ip->pmax)  ip->pmax = n;
          }
          /* VL 14/12/11 : calling lgbuild here seems to be problematic for
             undef arg checks */
          else {
            lgbuild(csound, ip, arg, 1, engineState);
          }
        }
      }
      /* VERIFY ARG LISTS MATCH OPCODE EXPECTED TYPES */
      {

        OENTRY *ep = csound->opcodlst + tp->opnum;
        int argcount = 0;
        for (outargs = root->left; outargs != NULL; outargs = outargs->next) {
          arg = outargs->value->lexeme;
          tp->outlist->arg[argcount++] = strsav_string(arg);
        }
        set_xincod(csound, tp, ep);

        /* OUTARGS */
        for (outargs = root->left; outargs != NULL; outargs = outargs->next) {

          arg = outargs->value->lexeme;

          if ((n = pnum(arg)) >= 0) {
            if (n > ip->pmax)  ip->pmax = n;
          }
          else {
            if (arg[0] == 'w' &&
                lgexist2(ip, arg, engineState) != 0) {
              synterr(csound, Str("output name previously used, "
                                  "type 'w' must be uniquely defined, line %d"),
                      root->line);
            }
            csound->DebugMsg(csound, "Arg: %s\n", arg);
            lgbuild(csound, ip, arg, 0, engineState);
          }

        }
        set_xoutcod(csound, tp, ep);

        if (root->right != NULL) {
          if (ep->intypes[0] != 'l') {     /* intype defined by 1st inarg */
            tp->intype = argtyp2( tp->inlist->arg[0]);
          }
          else  {
            tp->intype = 'l';          /*   (unless label)  */
          }
        }

        if (root->left != NULL) {      /* pftype defined by outarg */
          tp->pftype = argtyp2( root->left->value->lexeme);
        }
        else {                            /*    else by 1st inarg     */
          tp->pftype = tp->intype;
        }
      }
      break;
    default:
      csound->Message(csound,
                      Str("create_opcode: No rule to handle statement of "
                          "type %d\n"), root->type);
      if (PARSER_DEBUG) print_tree(csound, NULL, root);
    }

    if (retOptxt == NULL) {
      retOptxt = optxt;
    }
    else {
      append_optxt(retOptxt, optxt);
    }

    return retOptxt;
}

/**
 * Add a global variable and allocate memory
 * Globals, unlike locals, keep their memory space
 * in separate blocks, pointed by var->memBlock
 */
void addGlobalVariable(CSOUND *csound,
                       ENGINE_STATE *engineState,
                       CS_TYPE* type,
                       char *name,
                       void *typeArg)
{
    CS_VARIABLE *var = csoundCreateVariable(csound, csound->typePool,
                                            type, name, typeArg);
    csoundAddVariable(engineState->varPool, var);
    var->memBlock = (void *) mmalloc(csound, var->memBlockSize);
    if (var->initializeVariableMemory != NULL) {
      var->initializeVariableMemory(var, var->memBlock);
    }
}


/**
 * NB - instr0 to be created only once, in the first compilation
 *  and stored in csound->instr0
 * Create an Instrument (INSTRTXT) from the AST node given for use as
 * Instrument0. Called from csound_orc_compile.
 */
INSTRTXT *create_instrument0(CSOUND *csound, TREE *root,
                             ENGINE_STATE *engineState)
{
    INSTRTXT *ip;
    OPTXT *op;
    TREE *current;
    MYFLT sr= FL(-1.0), kr= FL(-1.0), ksmps= FL(-1.0),
          nchnls= DFLT_NCHNLS, inchnls = FL(0.0), _0dbfs= FL(-1.0);
    CS_TYPE* rType = (CS_TYPE*)&CS_VAR_TYPE_R;
    addGlobalVariable(csound, engineState, rType, "sr", NULL);
    addGlobalVariable(csound, engineState, rType, "kr", NULL);
    addGlobalVariable(csound, engineState, rType, "ksmps", NULL);
    addGlobalVariable(csound, engineState, rType, "nchnls", NULL);
    addGlobalVariable(csound, engineState, rType, "nchnls_i", NULL);
    addGlobalVariable(csound, engineState, rType, "0dbfs", NULL);
    addGlobalVariable(csound, engineState, rType, "$sr", NULL);
    addGlobalVariable(csound, engineState, rType, "$kr", NULL);
    addGlobalVariable(csound, engineState, rType, "$ksmps", NULL);
    /*
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "sr", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "kr", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "ksmps", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "nchnls", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "nchnls_i", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "0dbfs", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "$sr", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                                           rType, "$kr", NULL));
    csoundAddVariable(engineState->varPool,
                      csoundCreateVariable(csound, csound->typePool,
                      rType, "$ksmps", NULL));
    */
    myflt_pool_find_or_add(csound, engineState->constantsPool, 0);

    ip = (INSTRTXT *) mcalloc(csound, sizeof(INSTRTXT));
    ip->varPool = (CS_VAR_POOL*)mcalloc(csound, sizeof(CS_VAR_POOL));
    op = (OPTXT *)ip;

    current = root;

    /* initialize */

    ip->mdepends = 0;
    ip->opdstot = 0;


    ip->pmax = 3L;

    /* start chain */
    ip->t.opnum = INSTR;
    ip->t.opcod = strsav_string("instr"); /*  to hold global assigns */

    /* The following differs from otran and needs review.  otran keeps a
     * nulllist to point to for empty lists, while this is creating a new list
     * regardless
     */
    ip->t.outlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ip->t.outlist->count = 0;
    ip->t.inlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ip->t.inlist->count = 1;

    ip->t.inlist->arg[0] = strsav_string("0");


    while (current != NULL) {
      unsigned int uval;
      if (current->type != INSTR_TOKEN && current->type != UDO_TOKEN) {

        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "In INSTR 0: %s\n", current->value->lexeme);

        if (current->type == '='
            && strcmp(current->value->lexeme, "=.r") == 0) {

          //FIXME - perhaps should add check as it was in
          //constndx?  Not sure if necessary due to assumption
          //that tree will be verified
          MYFLT val = (MYFLT) strtod(current->right->value->lexeme, NULL);

          myflt_pool_find_or_add(csound, csound->engineState.constantsPool, val);

          /* modify otran defaults*/
          /* removed assignments to csound->tran_* */
          if (current->left->type == SRATE_TOKEN) {
            sr = val;
          }
          else if (current->left->type == KRATE_TOKEN) {
            kr = val;
          }
          else if (current->left->type == KSMPS_TOKEN) {
            uval = (val<=0 ? 1u : (unsigned int)val);
            ksmps = uval;
          }
          else if (current->left->type == NCHNLS_TOKEN) {
            uval = (val<=0 ? 1u : (unsigned int)val);
            nchnls = uval;
          }
          else if (current->left->type == NCHNLSI_TOKEN) {
            uval = (val<=0 ? 1u : (unsigned int)val);
            inchnls = uval;
            /* csound->Message(csound, "SETTING NCHNLS: %d\n",
               nchnls); */
          }
          else if (current->left->type == ZERODBFS_TOKEN) {
            _0dbfs = val;
            /* csound->Message(csound, "SETTING 0DBFS: %f\n",
               _0dbfs); */
          }

        }

        op->nxtop = create_opcode(csound, current, ip, engineState);
        op = last_optxt(op);

      }
      current = current->next;
    }

    /* Deal with defaults and consistency */
    if (ksmps == FL(-1.0)) {
      if (sr == FL(-1.0)) sr = DFLT_SR;
      if (kr == FL(-1.0)) kr = DFLT_KR;
      ksmps = (MYFLT) ((int) (sr/kr + FL(0.5)));
    }
    else if (kr == FL(-1.0)) {
      if (sr == FL(-1.0)) sr = DFLT_SR;
      kr = sr/ksmps;
    }
    else if (sr == FL(-1.0)) {
      sr = kr*ksmps;
    }
    /* That deals with missing values, however we do need ksmps to be integer */
    {
      CSOUND    *p = (CSOUND*) csound;
      char      err_msg[128];
      sprintf(err_msg, "sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:",
              sr, kr, ksmps);
      if (UNLIKELY(sr <= FL(0.0)))
        synterr(p, Str("%s invalid sample rate"), err_msg);
      if (UNLIKELY(kr <= FL(0.0)))
        synterr(p, Str("%s invalid control rate"), err_msg);
      else if (UNLIKELY(ksmps < FL(0.75) ||
                        FLOAT_COMPARE(ksmps,
                                      MYFLT2LRND(ksmps))))
        synterr(p, Str("%s invalid ksmps value"), err_msg);
      else if (UNLIKELY(FLOAT_COMPARE(sr,(double)kr *ksmps)))
        synterr(p, Str("%s inconsistent sr, kr, ksmps"), err_msg);
    }

    csound->ksmps = ksmps;
    csound->nchnls = nchnls;
    if(inchnls==0) csound->inchnls = nchnls;
    else csound->inchnls = inchnls;
    csound->esr = sr;
    csound->ekr = kr;
    if(_0dbfs < 0) csound->e0dbfs = DFLT_DBFS;
    else csound->e0dbfs = _0dbfs;

    OPARMS  *O = csound->oparms;
    if (UNLIKELY(csound->e0dbfs <= FL(0.0))){
      csound->Warning(csound,
                      Str("bad value for 0dbfs: must be positive. "
                          "Setting default value."));
      csound->e0dbfs = DFLT_DBFS;
    }
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                      "0dbfs = %.1f\n",
                      csound->esr, csound->ekr, csound->ksmps,
                      csound->nchnls, csound->e0dbfs);
    if (O->sr_override) {        /* if command-line overrides, apply now */
      MYFLT ensmps;
      csound->esr = (MYFLT) O->sr_override;
      csound->ekr = (MYFLT) O->kr_override;
      csound->ksmps = (int) ((ensmps = ((MYFLT) O->sr_override
                                        / (MYFLT) O->kr_override)) + FL(0.5));
      csound->Message(csound, Str("sample rate overrides: "
                                  "esr = %7.4f, ekr = %7.4f, ksmps = %d\n"),
                      csound->esr, csound->ekr, csound->ksmps);
      /* chk consistency one more time */
      {
        char  s[256];
        sprintf(s, Str("sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:"),
                csound->esr, csound->ekr, ensmps);
        if (UNLIKELY(csound->ksmps < 1 || FLOAT_COMPARE(ensmps, csound->ksmps)))
          csoundDie(csound, Str("%s invalid ksmps value"), s);
        if (UNLIKELY(csound->esr <= FL(0.0)))
          csoundDie(csound, Str("%s invalid sample rate"), s);
        if (UNLIKELY(csound->ekr <= FL(0.0)))
          csoundDie(csound, Str("%s invalid control rate"), s);
        if (UNLIKELY(FLOAT_COMPARE(csound->esr, (double) csound->ekr * ensmps)))
          csoundDie(csound, Str("%s inconsistent sr, kr, ksmps"), s);
      }
    }

    csound->tpidsr = TWOPI_F / csound->esr;               /* now set internal  */
    csound->mtpdsr = -(csound->tpidsr);                   /*    consts         */
    csound->pidsr = PI_F / csound->esr;
    csound->mpidsr = -(csound->pidsr);
    csound->onedksmps = FL(1.0) / (MYFLT) csound->ksmps;
    csound->sicvt = FMAXLEN / csound->esr;
    csound->kicvt = FMAXLEN / csound->ekr;
    csound->onedsr = FL(1.0) / csound->esr;
    csound->onedkr = FL(1.0) / csound->ekr;
    csound->global_kcounter  = csound->kcounter;

    if(csound->ksmps != DFLT_KSMPS){
      reallocateVarPoolMemory(csound, engineState->varPool);
    }
    close_instrument(csound, ip);

    return ip;
}


/**
 * Create an Instrument (INSTRTXT) from the AST node given. Called from
 * csound_orc_compile.
 */
INSTRTXT *create_instrument(CSOUND *csound, TREE *root,
                            ENGINE_STATE *engineState)
{
    INSTRTXT *ip;
    OPTXT *op;
    char *c;
    TREE *statements, *current;

    ip = (INSTRTXT *) mcalloc(csound, sizeof(INSTRTXT));
    ip->varPool = (CS_VAR_POOL*)mcalloc(csound, sizeof(CS_VAR_POOL));
    op = (OPTXT *)ip;
    statements = root->right;
    ip->mdepends = 0;
    ip->opdstot = 0;

    ip->pmax = 3L;

    /* Initialize */
    ip->t.opnum = INSTR;
    ip->t.opcod = strsav_string("instr"); /*  to hold global assigns */

    /* The following differs from otran and needs review.  otran keeps a
     * nulllist to point to for empty lists, while this is creating a new list
     * regardless
     */
    ip->t.outlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ip->t.outlist->count = 0;
    ip->t.inlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ip->t.inlist->count = 1;

    /* Maybe should do this assignment at end when instr is setup?
     * Note: look into how "instr 4,5,6,8" is handled, i.e. if copies
     * are made or if they are all set to point to the same INSTRTXT
     *
     * Note2: For now am not checking if root->left is a list (i.e. checking
     * root->left->next is NULL or not to indicate list)
     */
    if (root->left->type == INTEGER_TOKEN) { /* numbered instrument */
      int32 instrNum = (int32)root->left->value->value; /* Not used! */

      c = csound->Malloc(csound, 10); /* arbritrarily chosen number of digits */
      sprintf(c, "%ld", (long)instrNum);

      if (PARSER_DEBUG)
        csound->Message(csound,
                        Str("create_instrument: instr num %ld\n"), instrNum);

      ip->t.inlist->arg[0] = strsav_string(c);


      csound->Free(csound, c);
    } else if (root->left->type == T_IDENT &&
               !(root->left->left != NULL &&
                 root->left->left->type == UDO_ANS_TOKEN)) { /* named instrument */
      int32  insno_priority = -1L;
      c = root->left->value->lexeme;

      if (PARSER_DEBUG)
        csound->Message(csound, Str("create_instrument: instr name %s\n"), c);

      if (UNLIKELY(root->left->rate == (int) '+')) {
        insno_priority--;
      }
      /*
       if (UNLIKELY(!check_instr_name(c))) {
        synterr(csound, Str("invalid name for instrument"));
      }
      if (UNLIKELY(!named_instr_alloc(csound, c, ip,
                                      insno_priority, engineState))) {
        synterr(csound, Str("instr %s redefined"), c);
       }
       */

      ip->insname =  csound->Malloc(csound, strlen(c) + 1);
      strcpy(ip->insname, c);
    }
    current = statements;
    while (current != NULL) {
      OPTXT * optxt = create_opcode(csound, current, ip, engineState);
      op->nxtop = optxt;
      op = last_optxt(op);
      current = current->next;
    }
    close_instrument(csound, ip);
    return ip;
}

void close_instrument(CSOUND *csound, INSTRTXT * ip)
{
    OPTXT * bp, *current;
    int n;

    bp = (OPTXT *) mcalloc(csound, (int32)sizeof(OPTXT));

    bp->t.opnum = ENDIN;                          /*  send an endin to */
    bp->t.opcod = strsav_string("endin"); /*  term instr 0 blk */
    bp->t.outlist = bp->t.inlist = NULL;

    bp->nxtop = NULL;   /* terminate the optxt chain */

    current = (OPTXT *)ip;

    while (current->nxtop != NULL) {
      current = current->nxtop;
    }

    current->nxtop = bp;
    ip->mdepends = ip->mdepends; // ODD!!!!
    ip->pextrab = ((n = ip->pmax - 3L) > 0 ? (int) n * sizeof(MYFLT) : 0);
    ip->pextrab = ((int) ip->pextrab + 7) & (~7);
    ip->muted = 1;

}


int pnum(char *s)        /* check a char string for pnum format  */
/*   and return the pnum ( >= 0 )       */
{                               /* else return -1                       */
    int n;

    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return(n);
    return(-1);
}

/**
  This function deletes an inactive instrument which has been replaced
 */
void free_instrtxt(CSOUND *csound, INSTRTXT *instrtxt)
{
    INSTRTXT *ip = instrtxt;
    INSDS *active = ip->instance;
    while (active != NULL) {   /* remove instance memory */
      INSDS   *nxt = active->nxtinstance;
      if (active->fdchp != NULL)
        fdchclose(csound, active);
      if (active->auxchp != NULL)
        auxchfree(csound, active);
      mfree(csound, active);
      active = nxt;
    }
    OPTXT *t = ip->nxtop;
    while (t) {
          OPTXT *s = t->nxtop;
          mfree(csound, t);
          t = s;
        }
     mfree(csound, ip->varPool); /* need to delete the varPool memory */
     mfree(csound, ip);
     csound->Message(csound, "-- deleted instr from deadpool \n");
}

/**
 * This function has two purposes:
 * 1) check deadpool for active instances, and
 * if none is active, send it to be deleted
 * 2) add a dead instr to deadpool (because it still has active instances)
 */
void add_to_deadpool(CSOUND *csound, INSTRTXT *instrtxt)
{
    /* check current items in deadpool to see if they need deleting */
    int i;
    for(i=0; i < csound->dead_instr_no; i++){
      if(csound->dead_instr_pool[i] != NULL) {
        INSDS *active = csound->dead_instr_pool[i]->instance;
        while (active != NULL) {
          if(active->actflg) {
            add_to_deadpool(csound,csound->dead_instr_pool[i]);
            break;
          }
          active = active->nxtinstance;
        }
        /* no active instances */
        if (active == NULL) {
          free_instrtxt(csound, csound->dead_instr_pool[i]);
          csound->dead_instr_pool[i] = NULL;
        }
      }
    }
    /* add latest instr to deadpool */
    csound->dead_instr_pool = (INSTRTXT**)
      mrealloc(csound, csound->dead_instr_pool,
               ++csound->dead_instr_no * sizeof(INSTRTXT*));
    csound->dead_instr_pool[csound->dead_instr_no-1] = instrtxt;
    csound->Message(csound, " -- added to deadpool slot %d \n",
                    csound->dead_instr_no-1);
}

/** Insert INSTRTXT into an engineState list of INSTRTXT's,
    checking to see if number is greater than number of pointers currently
    allocated and if so expand pool of instruments
 */
void insert_instrtxt(CSOUND *csound, INSTRTXT *instrtxt,
                     int32 instrNum, ENGINE_STATE *engineState)
{
    int i;

    if (UNLIKELY(instrNum > engineState->maxinsno)) {
      int old_maxinsno = engineState->maxinsno;

      /* expand */
      while (instrNum > engineState->maxinsno) {
        engineState->maxinsno += MAXINSNO;
      }

      engineState->instrtxtp =
        (INSTRTXT**)mrealloc(csound,
                             engineState->instrtxtp,
                             (1 + engineState->maxinsno) * sizeof(INSTRTXT*));

      /* Array expected to be nulled so.... */
      for (i = old_maxinsno + 1; i <= engineState->maxinsno; i++) {
        engineState->instrtxtp[i] = NULL;
      }
    }

    if (UNLIKELY(engineState->instrtxtp[instrNum] != NULL)) {
      /* redefinition does not raise an error now, just a warning */
      csound->Warning(csound,
                      Str("instr %ld redefined, replacing previous definition\n"),
                      instrNum);
      /* here we should move the old instrument definition into a deadpool
         which will be checked for active instances and freed when there are no
         further ones
      */
      for(i=0; i < engineState->maxinsno; i++) {
        /* check for duplicate numbers and do nothing */
        if(i != instrNum &&
           engineState->instrtxtp[i] == engineState->instrtxtp[instrNum]) goto end;
      }
      INSDS *active = engineState->instrtxtp[instrNum]->instance;
      while (active != NULL) {
        if(active->actflg) {
          add_to_deadpool(csound, engineState->instrtxtp[instrNum]);
          break;
        }
        active = active->nxtinstance;
      }
      /* no active instances */
      if (active == NULL) free_instrtxt(csound, engineState->instrtxtp[instrNum]);
      /* err++; continue; */
    }
 end:
    instrtxt->isNew = 1;
    engineState->instrtxtp[instrNum] = instrtxt;

}

void insert_opcodes(CSOUND *csound, OPCODINFO *opcodeInfo,
                    ENGINE_STATE *engineState)
{
    if (opcodeInfo) {
      int num = engineState->maxinsno;  /* store after any other instruments */
      OPCODINFO *inm = opcodeInfo;
      while (inm) {
        /* we may need to expand the instrument array */
        if (UNLIKELY(++num > engineState->maxopcno)) {
          int i;
          i = (engineState->maxopcno > 0 ?
               engineState->maxopcno : engineState->maxinsno);
          engineState->maxopcno = i + MAXINSNO;
          engineState->instrtxtp = (INSTRTXT**)
            mrealloc(csound, engineState->instrtxtp, (1 + engineState->maxopcno)
                     * sizeof(INSTRTXT*));
          /* Array expected to be nulled so.... */
          while (++i <= engineState->maxopcno) engineState->instrtxtp[i] = NULL;
        }
        inm->instno = num;
        csound->Message(csound, "UDO INSTR NUM: %d\n", num);
        engineState->instrtxtp[num] = inm->ip;
        inm = inm->prv;
      }
    }
}


OPCODINFO *find_opcode_info(CSOUND *csound, char *opname)
{
    OPCODINFO *opinfo = csound->opcodeInfo;
    if (UNLIKELY(opinfo == NULL)) {
      csound->Message(csound, Str("!!! csound->opcodeInfo is NULL !!!\n"));
      return NULL;
    }

    while (opinfo != NULL) {
      if (UNLIKELY(strcmp(opinfo->name, opname) == 0)) {
        return opinfo;
      }
      opinfo = opinfo->prv;   /* Move on: JPff suggestion */
    }

    return NULL;
}

/**
  Merge a new engineState into csound->engineState
  1) Add to stringPool, constantsPool and varPool (globals)
  2) Add to opinfo and UDOs
  3) Call insert_instrtxt() on csound->engineState for each new instrument
  4) Call insprep() and recalculateVarPoolMemory() for each new instrument
  5) patch up nxtinstxt order
*/
int engineState_merge(CSOUND *csound, ENGINE_STATE *engineState)
{
    int i, end = engineState->maxinsno;
    ENGINE_STATE *current_state = &csound->engineState;
    INSTRTXT *current;

    STRING_VAL* val = engineState->stringPool->values;
    int count = 0;
    while(val != NULL) {
      csound->Message(csound, " merging strings %d) %s\n", count++, val->value);
      string_pool_find_or_add(csound, current_state->stringPool, val->value);
      val = val->next;
    }

    for(count = 0; count < engineState->constantsPool->count; count++) {
      csound->Message(csound, " merging constants %d) %f\n",
                      count, engineState->constantsPool->values[count]);
      myflt_pool_find_or_add(csound, current_state->constantsPool,
                             engineState->constantsPool->values[count]);
    }
    CS_VARIABLE* gVar = engineState->varPool->head;
    count = 0;
    while(gVar != NULL) {
      CS_VARIABLE* var;
      count++;
      csound->Message(csound, " merging  %d) %s:%s\n", count,
                      gVar->varName, gVar->varType->varTypeName);
      var = csoundFindVariableWithName(current_state->varPool, gVar->varName);
      if(var == NULL){
      var = csoundCreateVariable(csound, csound->typePool,
                                 gVar->varType, gVar->varName, NULL);
      csoundAddVariable(current_state->varPool, var);
      /* memory has already been allocated, so we just point to it */
      /* when disposing of the engineState global vars, we do not
         delete the memBlock */
      var->memBlock = gVar->memBlock;
      }
      gVar = gVar->next;
    }
    /* do we need to recalculate global pool and allocate memory ? */
    //FIXME - need to reinitialize variables here using intializeVariableMemory...
    if(count) {
      recalculateVarPoolMemory(csound, current_state->varPool);
      /* VL 15.3.2013 realloc will not work because it messes with the
         memory that has been set in a running instance
         The best we can do at the moment is to alloc plenty of
         memory to start with so that new vars can be accommodated there */
      //csound->globalVarPool = krealloc(csound, csound->globalVarPool,
      //                             current_state->varPool->poolSize);
    }
    /* merge opcodinfo */
    insert_opcodes(csound, csound->opcodeInfo, current_state);
    for(i=1; i < end; i++){
      current = engineState->instrtxtp[i];
      if(current != NULL){
        if(current->insname == NULL) {
          csound->Message(csound, "merging instr %d \n", i);
          /* a first attempt at this merge is to make it use
             insert_instrtxt again */
          /* insert instrument in current engine */
          insert_instrtxt(csound,current,i,current_state);
        }
        else {
          csound->Message(csound, "merging instr %s \n", current->insname);
          /* allocate a named_instr string in the current engine */
          /* FIXME: check the redefinition case for named instrs */
          named_instr_alloc(csound,current->insname,current,-1L,current_state);
        }
      }
    }
    /* merges all named instruments */
    named_instr_assign_numbers(csound,current_state);
    /* this needs to be called in a separate loop
       in case of multiple instr numbers, so insprep() is called only once */
    current = (&(engineState->instxtanchor))->nxtinstxt;
    while ((current = current->nxtinstxt) != NULL) {
      csound->Message(csound, "insprep %p \n", current);
      insprep(csound, current, current_state);/* run insprep() to connect ARGS  */
      recalculateVarPoolMemory(csound,
                               current->varPool); /* recalculate var pool */
  }
    /* now we need to patch up instr order */
    end = current_state->maxinsno;
    end = end < current_state->maxopcno ? current_state->maxopcno : end;
    for(i=0; i < end; i++){
      int j;
      current = current_state->instrtxtp[i];
      if(current != NULL){
        csound->Message(csound, "instr %d \n", i);
        current->nxtinstxt = NULL;
        j = i;
        while(++j < end-1) {
          if(current_state->instrtxtp[j] != NULL){
            current->nxtinstxt = current_state->instrtxtp[j];
            break;
          }
        }
      }
    }
    return 0;
}

int engineState_free(CSOUND *csound, ENGINE_STATE *engineState)
{
    /* FIXME: we need functions to deallocate stringPool, constantPool */
    mfree(csound, engineState->instrumentNames);
    myflt_pool_free(csound, engineState->constantsPool);
    string_pool_free(csound, engineState->stringPool);
    mfree(csound, engineState->varPool);
    mfree(csound, engineState);
    return 0;
}

/**
 * Compile the given TREE node into structs

   In the the first compilation run, it:
   1) Uses the empty csound->engineState
   2) Creates instrument 0
   3) Creates other instruments and UDOs
   4) Runs insprep() and recalculateVarpool() for each instrument

   In any subsequent compilation run, it:
   1) Creates a new engineState
   2) Creates instruments other than instrument 0 (which is ignored)
   3) Calls engineState_merge() and engineState_free()

  VL 20-12-12

 * ASSUMES: TREE has been validated prior to compilation
 *
 *
 */
PUBLIC int csoundCompileTree(CSOUND *csound, TREE *root)
{
    INSTRTXT    *instrtxt = NULL;
    INSTRTXT    *ip = NULL;
    INSTRTXT    *prvinstxt;
    OPTXT       *bp;
    char        *opname;
    int32        count, sumcount; //, instxtcount, optxtcount;
    TREE * current = root;
    ENGINE_STATE *engineState;

    if(csound->instr0 == NULL) {
      engineState = &csound->engineState;
      csound->instr0 = create_instrument0(csound, root, engineState);
      string_pool_find_or_add(csound, engineState->stringPool, "\"\"");
      prvinstxt = &(engineState->instxtanchor);
       engineState->instrtxtp =
      (INSTRTXT **) mcalloc(csound, (1 + engineState->maxinsno)
                            * sizeof(INSTRTXT*));
       prvinstxt = prvinstxt->nxtinstxt = csound->instr0;
      insert_instrtxt(csound, csound->instr0, 0, engineState);
    }
    else {
      engineState = (ENGINE_STATE *) mcalloc(csound, sizeof(ENGINE_STATE));
      engineState->stringPool = string_pool_create(csound);
      engineState->constantsPool = myflt_pool_create(csound);
      engineState->varPool = csound->Calloc(csound, sizeof(CS_VAR_POOL));
      prvinstxt = &(engineState->instxtanchor);
       engineState->instrtxtp =
      (INSTRTXT **) mcalloc(csound, (1 + engineState->maxinsno)
                            * sizeof(INSTRTXT*));
      prvinstxt = prvinstxt->nxtinstxt = csound->instr0;
    }


    while (current != NULL) {

      switch (current->type) {
      case '=':
        /* csound->Message(csound, "Assignment found\n"); */
        break;
      case INSTR_TOKEN:
        //print_tree(csound, "Instrument found\n", current);
        instrtxt = create_instrument(csound, current,engineState);

        prvinstxt = prvinstxt->nxtinstxt = instrtxt;

        /* Handle Inserting into CSOUND here by checking ids (name or
         * numbered) and using new insert_instrtxt?
         */
        /* Temporarily using the following code */
        if (current->left->type == INTEGER_TOKEN) { /* numbered instrument */
          int32 instrNum = (int32)current->left->value->value;
          insert_instrtxt(csound, instrtxt, instrNum, engineState);
        }
        else if (current->left->type == T_INSTLIST) {
          TREE *p =  current->left;
          //printf("instlist case:\n"); /* This code is suspect */
          while (p) {
            if (PARSER_DEBUG) print_tree(csound, "Top of loop\n", p);
            if (p->left) {
              //print_tree(csound, "Left\n", p->left);
              if (p->left->type == INTEGER_TOKEN) {
                insert_instrtxt(csound, instrtxt, p->left->value->value,
                                engineState);
              }
              else if (p->left->type == T_IDENT) {
                int32  insno_priority = -1L;
                char *c;
                c = p->left->value->lexeme;

                if (UNLIKELY(p->left->rate == (int) '+')) {
                  insno_priority--;
                }
                if (UNLIKELY(!check_instr_name(c))) {
                  synterr(csound, Str("invalid name for instrument"));
                }
                if (UNLIKELY(!named_instr_alloc(csound, c,
                                                instrtxt, insno_priority,
                                                engineState))) {
                  synterr(csound, Str("instr %s redefined"), c);
                }
                instrtxt->insname = csound->Malloc(csound, strlen(c) + 1);
                strcpy(instrtxt->insname, c);
              }
            }
            else {
              if (p->type == INTEGER_TOKEN) {
                insert_instrtxt(csound, instrtxt, p->value->value, engineState);
              }
              else if (p->type == T_IDENT) {
                int32  insno_priority = -1L;
                char *c;
                c = p->value->lexeme;

                if (UNLIKELY(p->rate == (int) '+')) {
                  insno_priority--;
                }
                if (UNLIKELY(!check_instr_name(c))) {
                  synterr(csound, Str("invalid name for instrument"));
                }
                if (UNLIKELY(!named_instr_alloc(csound, c,
                                                instrtxt, insno_priority,
                                                engineState))) {
                  synterr(csound, Str("instr %s redefined"), c);
                }
                instrtxt->insname = csound->Malloc(csound, strlen(c) + 1);
                strcpy(instrtxt->insname, c);
              }
              break;
            }
            p = p->right;
          }
        }
        break;
      case UDO_TOKEN:
        /* csound->Message(csound, "UDO found\n"); */
        instrtxt = create_instrument(csound, current, engineState);
        prvinstxt = prvinstxt->nxtinstxt = instrtxt;
        opname = current->left->value->lexeme;
        OPCODINFO *opinfo = find_opcode_info(csound, opname);

        if (UNLIKELY(opinfo == NULL)) {
          csound->Message(csound,
                          "ERROR: Could not find OPCODINFO for opname: %s\n",
                          opname);
        }
        else {
          opinfo->ip = instrtxt;
          instrtxt->insname = (char*)mmalloc(csound, 1+strlen(opname));
          strcpy(instrtxt->insname, opname);
          instrtxt->opcode_info = opinfo;
        }

        /* Handle Inserting into CSOUND here by checking id's (name or
         * numbered) and using new insert_instrtxt?
         */

        break;
      case T_OPCODE:
      case T_OPCODE0:
        break;
      default:
        csound->Message(csound,
                        Str("Unknown TREE node of type %d found in root.\n"),
                        current->type);
        if (PARSER_DEBUG) print_tree(csound, NULL, current);
      }
      current = current->next;
    }

    if (UNLIKELY(csound->synterrcnt)) {
      print_opcodedir_warning(csound);
      csound->Warning(csound, Str("%d syntax errors in orchestra.  "
                              "compilation invalid\n"),
                  csound->synterrcnt);
      return CSOUND_ERROR;
    }

    /* now add the instruments with names, assigning them fake instr numbers */
    named_instr_assign_numbers(csound,engineState);

    /* lock to ensure thread-safety */
    csoundLockMutex(csound->API_lock);
    if(csound->oparms->realtime) csoundLockMutex(csound->init_pass_threadlock);
    if(engineState != &csound->engineState) {
      /* merge ENGINE_STATE */
      engineState_merge(csound, engineState);
      /* delete ENGINE_STATE  */
      engineState_free(csound, engineState);
    }
    else {
      insert_opcodes(csound, csound->opcodeInfo, engineState);
      ip = engineState->instxtanchor.nxtinstxt;
      bp = (OPTXT *) ip;
      while (bp != (OPTXT *) NULL && (bp = bp->nxtop) != NULL) {
        /* chk instr 0 for illegal perfs */
        int thread, opnum = bp->t.opnum;
        if (opnum == ENDIN) break;
        if (opnum == LABEL) continue;
        if (PARSER_DEBUG)
          csound->DebugMsg(csound, "Instr 0 check on opcode=%s\n", bp->t.opcod);
        if (UNLIKELY((thread = csound->opcodlst[opnum].thread) & 06 ||
                     (!thread && bp->t.pftype != 'b'))) {
          csound->DebugMsg(csound, "***opcode=%s thread=%d pftype=%c\n",
                           bp->t.opcod, thread, bp->t.pftype);
          synterr(csound, Str("perf-pass statements illegal in header blk\n"));
        }
      }

      ip = &(engineState->instxtanchor);
      while ((ip = ip->nxtinstxt) != NULL) {        /* add all other entries */
        insprep(csound, ip, engineState);           /*   as combined offsets */
        recalculateVarPoolMemory(csound, ip->varPool);
      }

      /* create memblock for global variables */
      ///recalculateVarPoolMemory(csound, engineState->varPool);
      /* VL: 15.3.2013 allocating 10 times for space than requested,
         for use with variables allocated later */
      /* csound->globalVarPool = mcalloc(csound, engineState->varPool->poolSize*10);
         initializeVarPool(csound->globalVarPool, engineState->varPool);*/

      /*MYFLT* globals = csound->globalVarPool;
      globals[0] = csound->esr;
      globals[1] = csound->ekr;
      globals[2] = (MYFLT) csound->ksmps;
      globals[3] = (MYFLT) csound->nchnls;
      if (csound->inchnls<0) csound->inchnls = csound->nchnls;
      globals[4] = (MYFLT) csound->inchnls;
      globals[5] = csound->e0dbfs;*/

      CS_VARIABLE *var;
      var = csoundFindVariableWithName(engineState->varPool, "sr");
      *((MYFLT *)(var->memBlock)) = csound->esr;
      var = csoundFindVariableWithName(engineState->varPool, "kr");
      *((MYFLT *)(var->memBlock)) = csound->ekr;
      var = csoundFindVariableWithName(engineState->varPool, "ksmps");
      *((MYFLT *)(var->memBlock)) = csound->ksmps;
      var = csoundFindVariableWithName(engineState->varPool, "nchnls");
      *((MYFLT *)(var->memBlock)) = csound->nchnls;
      if (csound->inchnls<0) csound->inchnls = csound->nchnls;
      var = csoundFindVariableWithName(engineState->varPool, "nchnls_i");
      *((MYFLT *)(var->memBlock)) = csound->inchnls;
      var = csoundFindVariableWithName(engineState->varPool, "0dbfs");
      *((MYFLT *)(var->memBlock)) = csound->inchnls;

    }

    ip = &(csound->engineState.instxtanchor);
    ip = ip->nxtinstxt;
    for (sumcount = 0; (ip = ip->nxtinstxt) != NULL; ) {/* for each instxt */
      OPTXT *optxt = (OPTXT *) ip;
      int optxtcount = 0;
      while ((optxt = optxt->nxtop) != NULL) {      /* for each op in instr  */
        TEXT *ttp = &optxt->t;
        optxtcount += 1;
        if (ttp->opnum == ENDIN                     /*    (until ENDIN)      */
            || ttp->opnum == ENDOP) break;
        if ((count = ttp->inlist->count)!=0)
          sumcount += count +1;                     /* count the non-nullist */
        if ((count = ttp->outlist->count)!=0)       /* slots in all arglists */
          sumcount += (count + 1);
      }
      ip->optxtcount = optxtcount;                  /* optxts in this instxt */
    }
    if(csound->oparms->realtime) csoundUnlockMutex(csound->init_pass_threadlock);
    /* notify API lock  */
    csoundUnlockMutex(csound->API_lock);
    return CSOUND_SUCCESS;
}

/**
    Parse and compile an orchestra given on an string (OPTIONAL)
    if str is NULL the string is taken from the internal corfile
    containing the initial orchestra file passed to Csound.
*/
PUBLIC int csoundCompileOrc(CSOUND *csound, char *str)
{
    int retVal;
    TREE *root = csoundParseOrc(csound, str);
    if(root != NULL) {
    retVal = csoundCompileTree(csound, root);
    } else return  CSOUND_ERROR;
    delete_tree(csound, root);
    if (csound->oparms->odebug)
      debugPrintCsound(csound);
    return retVal;
}


/* prep an instr template for efficient allocs  */
/* repl arg refs by offset ndx to lcl/gbl space */
static void insprep(CSOUND *csound, INSTRTXT *tp, ENGINE_STATE *engineState)
{
    OPARMS      *O = csound->oparms;
    OPTXT       *optxt;
    OENTRY      *ep;
    int         opnum;
    char        **argp;

    int n, inreqd;
    char**  argStringParts;
    ARGLST      *outlist, *inlist;
    optxt = (OPTXT *)tp;
    while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
      TEXT *ttp = &optxt->t;
      if ((opnum = ttp->opnum) == ENDIN         /*  (until ENDIN)  */
          || opnum == ENDOP)
        break;
      if (opnum == LABEL) {
        continue;
      }
      ep = &(csound->opcodlst[opnum]);
      if (O->odebug)
        csound->Message(csound, "%s args:\n", ep->opname);
      if ((outlist = ttp->outlist) == NULL || !outlist->count)
        ttp->outArgs = NULL;
      else {
        n = outlist->count;
        argp = outlist->arg;                    /* get outarg indices */
        while (n--) {
          ARG* arg = createArg(csound, tp, *argp++, engineState);

          if(ttp->outArgs == NULL) {
            ttp->outArgs = arg;
          } else {
            ARG* current = ttp->outArgs;
            while(current->next != NULL) {
              current = current->next;
            }
            current->next = arg;
            arg->next = NULL;
          }
        }
        ttp->outArgCount = argCount(ttp->outArgs);
      }
      if ((inlist = ttp->inlist) == NULL || !inlist->count)
        ttp->inArgs = NULL;
      else {
        inreqd = argsRequired(ep->intypes);
        argStringParts = splitArgs(csound, ep->intypes);
        argp = inlist->arg;                     /* get inarg indices */
        for (n=0; n < inlist->count; n++, argp++) {
          ARG* arg = NULL;
          if (n < inreqd && *argStringParts[n] == 'l') {
            arg = csound->Calloc(csound, sizeof(ARG));
            arg->type = ARG_LABEL;
            arg->argPtr = mmalloc(csound, strlen(*argp) + 1);
            strcpy(arg->argPtr, *argp);
            if (UNLIKELY(O->odebug))
              csound->Message(csound, "\t***lbl");  /* if arg is label,  */
          } else {
            char *s = *argp;
            arg = createArg(csound, tp, s, engineState);
          }

          if(ttp->inArgs == NULL) {
            ttp->inArgs = arg;
          } else {
            ARG* current = ttp->inArgs;
            while(current->next != NULL) {
              current = current->next;
            }
            current->next = arg;
            arg->next = NULL;
          }
        }

        ttp->inArgCount = argCount(ttp->inArgs);
        mfree(csound, argStringParts);
      }
    }
}

/* returns non-zero if 's' is defined in the global or local pool of names */
static int lgexist2(INSTRTXT* ip, const char* s, ENGINE_STATE *engineState)
{
    int retVal = 0;
    if(csoundFindVariableWithName(engineState->varPool, s) != NULL) {
      retVal = 1;
    } else if(csoundFindVariableWithName(ip->varPool, s) != NULL) {
      retVal = 1;
    }

    return retVal;
}

/* build pool of floating const values  */
/* build lcl/gbl list of ds names, offsets */
/* (no need to save the returned values) */
static void lgbuild(CSOUND *csound, INSTRTXT* ip, char *s,
                    int inarg, ENGINE_STATE *engineState)
{
    char    c;
    char* temp;

    c = *s;
    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0)) {
      myflt_pool_find_or_addc(csound, engineState->constantsPool, s);
    } else if (c == '"') {
      // FIXME need to unquote_string here
      //unquote_string
      temp = mcalloc(csound, strlen(s) + 1);
      unquote_string(temp, s);
      string_pool_find_or_add(csound, engineState->stringPool, temp);
    } else if (!lgexist2(ip, s, engineState) && !inarg) {
      if (c == 'g' || (c == '#' && s[1] == 'g'))
        gblnamset(csound, s, engineState);
      else
        lclnamset(csound, ip, s);
    }
}

/* get storage ndx of const, pnum, lcl or gbl */
/* argument const/gbl indexes are positiv+1, */
/* pnum/lcl negativ-1 called only after      */
/* poolcount & lclpmax are finalised */
static ARG* createArg(CSOUND *csound, INSTRTXT* ip,
                      char *s, ENGINE_STATE *engineState)
{
    char        c;
    char*       temp;
    int         n;

    c = *s;

    ARG* arg = csound->Calloc(csound, sizeof(ARG));

    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0)) {
      arg->type = ARG_CONSTANT;
      arg->index = myflt_pool_find_or_addc(csound, engineState->constantsPool, s);
    } else if (c == '"') {
      arg->type = ARG_STRING;
      temp = mcalloc(csound, strlen(s) + 1);
      unquote_string(temp, s);
      arg->argPtr = string_pool_find_or_add(csound, engineState->stringPool, temp);
    } else if ((n = pnum(s)) >= 0) {
      arg->type = ARG_PFIELD;
      arg->index = n;
    }
    else if (c == 'g' || (c == '#' && *(s+1) == 'g') ||
             csoundFindVariableWithName(csound->engineState.varPool, s) != NULL) {
      // FIXME - figure out why string pool searched with gexist
      //|| string_pool_indexof(csound->engineState.stringPool, s) > 0) {
      arg->type = ARG_GLOBAL;
      arg->argPtr = csoundFindVariableWithName(engineState->varPool, s);
    }
    else {
      arg->type = ARG_LOCAL;
      arg->argPtr = csoundFindVariableWithName(ip->varPool, s);

      if(arg->argPtr == NULL) {
        csound->Message(csound, "Missing local arg: %s\n", s);
      }
    }
    /*    csound->Message(csound, " [%s -> %d (%x)]\n", s, indx, indx); */
    return arg;
}

/* builds namelist & type counts for gbl names */

static void gblnamset(CSOUND *csound, char *s, ENGINE_STATE *engineState)
{
    CS_TYPE* type;
    char* argLetter;
    CS_VARIABLE* var;
    char* t = s;
    ARRAY_VAR_INIT varInit;

    var = csoundFindVariableWithName(engineState->varPool, s);

    if (var != NULL) {
      return;
    }

    argLetter = csound->Malloc(csound, 2 * sizeof(char));
    argLetter[1] = 0;

    if (*t == '#')  t++;
    if (*t == 'g')  t++;

    void* typeArg = NULL;

    if(*t == '[') {
        int dimensions = 1;
        CS_TYPE* varType;
        char* b = t + 1;

        while(*b == '[' && b != NULL) {
            b++;
            dimensions++;
        }
        argLetter[0] = *b;

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type = varType;
        typeArg = &varInit;
    }

    argLetter[0] = *t;

    type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    /*
    var = csoundCreateVariable(csound, csound->typePool, type, s, typeArg);
    csoundAddVariable(engineState->varPool, var);
    */
    addGlobalVariable(csound, engineState, type, s, typeArg);
}

static void lclnamset(CSOUND *csound, INSTRTXT* ip, char *s)
{
    CS_TYPE* type;
    char argLetter[2];
    CS_VARIABLE* var;
    char* t = s;
    ARRAY_VAR_INIT varInit;

    var = csoundFindVariableWithName(ip->varPool, s);

    if (var != NULL) {
      return;
    }

    argLetter[1] = 0;

    if (*t == '#')  t++;

    void* typeArg = NULL;

    if(*t == '[') {
        int dimensions = 1;
        CS_TYPE* varType;
        char* b = t + 1;

        while(*b == '[') {
            b++;
            dimensions++;
        }
        argLetter[0] = *b;

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type = varType;
        typeArg = &varInit;
    }

    argLetter[0] = *t;

    type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    var = csoundCreateVariable(csound, csound->typePool, type, s, typeArg);
    csoundAddVariable(ip->varPool, var);
}

char argtyp2(char *s)
{                   /* find arg type:  d, w, a, k, i, c, p, r, S, B, b, t */
    char c = *s;    /*   also set lgprevdef if !c && !p && !S */

    /* trap this before parsing for a number! */
    /* two situations: defined at header level: 0dbfs = 1.0
     *  and returned as a value:  idb = 0dbfs
     */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0))
      return('c');                              /* const */
    if (pnum(s) >= 0)
      return('p');                              /* pnum */
    if (c == '"')
      return('S');                              /* quoted String */
    if (strcmp(s,"sr") == 0    || strcmp(s,"kr") == 0 ||
        strcmp(s,"0dbfs") == 0 || strcmp(s,"nchnls_i") == 0 ||
        strcmp(s,"ksmps") == 0 || strcmp(s,"nchnls") == 0)
      return('r');                              /* rsvd */
    if (c == 'w')               /* N.B. w NOT YET #TYPE OR GLOBAL */
      return(c);
    if (c == '#')
      c = *(++s);
    if (c == 'g')
      c = *(++s);
    if (c == '[') {
      while(c == '[') {
        c = *(++s);
      }
    }
    if (strchr("akiBbfSt", c) != NULL)
      return(c);
    else return('?');
}

/* For diagnostics map file name or macro name to an index */
uint8_t file_to_int(CSOUND *csound, const char *name)
{
    extern char *strdup(const char *);
    uint8_t n = 0;
    char **filedir = csound->filedir;
    while (filedir[n] && n<63) {        /* Do we have it already? */
      if (strcmp(filedir[n], name)==0) return n; /* yes */
      n++;                                       /* look again */
    }
    // Not there so add
    // ensure long enough?
    if (n==63) {
      filedir[n] = strdup("**unknown**");
    }
    else {
      filedir[n] = strdup(name);
      filedir[n+1] = NULL;
    }
    return n;
}

void debugPrintCsound(CSOUND* csound)
{
    INSTRTXT    *current;
    STRING_VAL* val = csound->engineState.stringPool->values;
    int count = 0;
    csound->Message(csound, "Compile State:\n");
    csound->Message(csound, "String Pool:\n");

    while(val != NULL) {
      csound->Message(csound, "    %d) %s\n", count++, val->value);
      val = val->next;
    }
    csound->Message(csound, "Constants Pool:\n");
    count = 0;
    for(count = 0; count < csound->engineState.constantsPool->count; count++) {
      csound->Message(csound, "    %d) %f\n",
                      count, csound->engineState.constantsPool->values[count]);
    }

    csound->Message(csound, "Global Variables:\n");
    CS_VARIABLE* gVar = csound->engineState.varPool->head;
    count = 0;
    while(gVar != NULL) {
      csound->Message(csound, "  %d) %s:%s\n", count++,
                      gVar->varName, gVar->varType->varTypeName);
      gVar = gVar->next;
    }

    /* bad practice to declare variables in middle of block */
    current = &(csound->engineState.instxtanchor);
    current = current->nxtinstxt;
    count = 0;
    while (current != NULL) {
      csound->Message(csound, "Instrument %d %p %p\n",
                      count, current, current->nxtinstxt);
      csound->Message(csound, "Variables\n");

      if(current->varPool != NULL) {
        CS_VARIABLE* var = current->varPool->head;
        int index = 0;
        while(var != NULL) {
          csound->Message(csound, "  %d) %s:%s\n", index++,
                          var->varName, var->varType->varTypeName);
          var = var->next;
        }
      }
      count++;
      current = current->nxtinstxt;
    }
}



#if 0
/*
 the code in this function has been refactored (see comments below)
*/
void initialize_instrument0(CSOUND *csound)
{
    //INSTRTXT *ip;
    OPARMS  *O = csound->oparms;
    ENGINE_STATE *engineState = &csound->engineState;

    //ip = engineState->instxtanchor.nxtinstxt;        /* for instr 0 optxts:  */

    /* this code has been moved to create_instrument0 */

    if (UNLIKELY(csound->e0dbfs <= FL(0.0)))
      csound->Die(csound, Str("bad value for 0dbfs: must be positive."));
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                      "0dbfs = %.1f\n",
                      csound->esr, csound->ekr, csound->ksmps,
                      csound->nchnls, csound->e0dbfs);
    if (O->sr_override) {
      MYFLT ensmps;
      csound->esr = (MYFLT) O->sr_override;
      csound->ekr = (MYFLT) O->kr_override;
      csound->ksmps = (int) ((ensmps = ((MYFLT) O->sr_override
                                        / (MYFLT) O->kr_override)) + FL(0.5));
      csound->Message(csound, Str("sample rate overrides: "
                                  "esr = %7.4f, ekr = %7.4f, ksmps = %d\n"),
                      csound->esr, csound->ekr, csound->ksmps);
      {
        char  s[256];
        sprintf(s, Str("sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:"),
                csound->esr, csound->ekr, ensmps);
        if (UNLIKELY(csound->ksmps < 1 || FLOAT_COMPARE(ensmps, csound->ksmps)))
          csoundDie(csound, Str("%s invalid ksmps value"), s);
        if (UNLIKELY(csound->esr <= FL(0.0)))
          csoundDie(csound, Str("%s invalid sample rate"), s);
        if (UNLIKELY(csound->ekr <= FL(0.0)))
          csoundDie(csound, Str("%s invalid control rate"), s);
        if (UNLIKELY(FLOAT_COMPARE(csound->esr, (double) csound->ekr * ensmps)))
          csoundDie(csound, Str("%s inconsistent sr, kr, ksmps"), s);
      }
    }


    /* this code has been moved to compileTree */

    recalculateVarPoolMemory(csound, engineState->varPool);
    csound->globalVarPool = mcalloc(csound, engineState->varPool->poolSize);

    MYFLT* globals = csound->globalVarPool;
    globals[0] = csound->esr;
    globals[1] = csound->ekr;
    globals[2] = (MYFLT) csound->ksmps;
    globals[3] = (MYFLT) csound->nchnls;
    if (csound->inchnls<0) csound->inchnls = csound->nchnls;
    globals[4] = (MYFLT) csound->inchnls;
    globals[5] = csound->e0dbfs;

#ifdef SOME_FINE_DAY
    /* the code below does not appear to have any current use */
    ip = &(engineState->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL) {      /* EXPAND NDX for A & S Cells */
      optxt = (OPTXT *) ip;                     /*   (and set localen)        */
      /* recalculateVarPoolMemory(csound, ip->varPool);
         moved to csoundCompileTree_async */
      //FIXME - note alignment
      /* align to 64 bits */
      // ip->localen = (ip->localen + 7L) & (~7L);
      for (insno = 0, n = 0; insno <= engineState->maxinsno; insno++)
        if (engineState->instrtxtp[insno] == ip)  n++;   /* count insnos  */

      lp = ip->inslist = (int32 *) mmalloc(csound, (int32)(n+1) * sizeof(int32));
      for (insno=0; insno <= engineState->maxinsno; insno++)
        if (engineState->instrtxtp[insno] == ip)
          *lp++ = insno;                                /* creat inslist */
      *lp = -1;                                         /*   & terminate */
      insno = *ip->inslist;                             /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT    *ttp = &optxt->t;
        int     opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;
        if (opnum == LABEL) continue;
        n = argCount(ttp->outArgs);
      }
    }
#endif
/* this code has been moved to create_instrument0 */

    csound->tpidsr = TWOPI_F / csound->esr;
    csound->mtpdsr = -(csound->tpidsr);
    csound->pidsr = PI_F / csound->esr;
    csound->mpidsr = -(csound->pidsr);
    csound->onedksmps = FL(1.0) / (MYFLT) csound->ksmps;
    csound->sicvt = FMAXLEN / csound->esr;
    csound->kicvt = FMAXLEN / csound->ekr;
    csound->onedsr = FL(1.0) / csound->esr;
    csound->onedkr = FL(1.0) / csound->ekr;
    csound->global_kcounter  = csound->kcounter;
    /* these calls were moved to musmon() in musmon.c */
    reverbinit(csound);
    dbfs_init(csound, csound->e0dbfs);
    csound->nspout = csound->ksmps * csound->nchnls;  /* alloc spin & spout */
    csound->nspin = csound->ksmps * csound->inchnls; /* JPff: in preparation */
    csound->spin  = (MYFLT *) mcalloc(csound, csound->nspin * sizeof(MYFLT));
    csound->spout = (MYFLT *) mcalloc(csound, csound->nspout * sizeof(MYFLT));

    /* initialise sensevents state */
    csound->prvbt = csound->curbt = csound->nxtbt = 0.0;
    csound->curp2 = csound->nxtim = csound->timeOffs = csound->beatOffs = 0.0;
    csound->icurTime = 0L;
    if (O->Beatmode && O->cmdTempo > 0) {
      /* if performing from beats, set the initial tempo */
      csound->curBeat_inc = (double) O->cmdTempo / (60.0 * (double) csound->ekr);
      csound->ibeatTime = (int64_t)(csound->esr*60.0 / (double) O->cmdTempo);
    }
    else {
      csound->curBeat_inc = 1.0 / (double) csound->ekr;
      csound->ibeatTime = 1;
    }
    csound->cyclesRemaining = 0;
    memset(&(csound->evt), 0, sizeof(EVTBLK));

    /* run instr 0 inits */
    if (UNLIKELY(init0(csound) != 0))
      csoundDie(csound, Str("header init errors"));
}

/* get size of string in MYFLT units */

static int strlen_to_samples(const char *s)
{
    int n = (int) strlen(s);
    n = (n + (int) sizeof(MYFLT)) / (int) sizeof(MYFLT);
    return n;
}

static int create_strconst_ndx_list(CSOUND *csound, int **lst, int offs)
{
    int     *ndx_lst;
    char    **strpool;
    int     strpool_cnt, ndx, i;

    strpool_cnt = STA(strpool_cnt);
    strpool = STA(strpool);
    /* strpool_cnt always >= 1 because of empty string at index 0 */
    ndx_lst = (int*) csound->Malloc(csound, strpool_cnt * sizeof(int));
    for (i = 0, ndx = offs; i < strpool_cnt; i++) {
      ndx_lst[i] = ndx;
      ndx += strlen_to_samples(strpool[i]);
    }
    *lst = ndx_lst;
    /* return with total size in MYFLT units */
    return (ndx - offs);
}

static void convert_strconst_pool(CSOUND *csound, MYFLT *dst)
{
    char    **strpool, *s;
    int     strpool_cnt, ndx, i;

    strpool_cnt = STA(strpool_cnt);
    strpool = STA(strpool);
    if (strpool == NULL)
      return;
    for (ndx = i = 0; i < strpool_cnt; i++) {
      s = (char*) ((MYFLT*) dst + (int) ndx);
      unquote_string(s, strpool[i]);
      ndx += strlen_to_samples(strpool[i]);
    }
    /* original pool is no longer needed */
    STA(strpool) = NULL;
    STA(strpool_cnt) = 0;
    for (i = 0; i < strpool_cnt; i++)
      csound->Free(csound, strpool[i]);
    csound->Free(csound, strpool);
}
#endif

#include "interlocks.h"
int find_opcode(CSOUND *csound, char *name);
void query_deprecated_opcode(CSOUND *csound, ORCTOKEN *o)
{
    char *name = o->lexeme;
    int32 opnum = find_opcode(csound, name);
    OENTRY *ep = csound->opcodlst + opnum;
    if (ep->flags&_QQ)
      csound->Warning(csound, Str("Opcode \"%s\" is deprecated\n"), name);
}
