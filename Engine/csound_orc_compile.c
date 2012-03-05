 /*
    csound_orc_compile.c:
    (Based on otran.c)

    Copyright (C) 1991, 1997, 2003, 2006
    Barry Vercoe, John ffitch, Istvan Varga, Steven Yi

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
#include "csound_orc.h"
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "oload.h"
#include "insert.h"
#include "pstream.h"
#include "namedins.h"
#include "typetabl.h"

typedef struct NAME_ {
    char          *namep;    struct NAME_  *nxt;
    int           type, count;
} NAME;

typedef struct {
    NAME      *gblNames[256], *lclNames[256];   /* for 8 bit hash */
    ARGLST    *nullist;
    ARGOFFS   *nulloffs;
    int       lclkcnt, lclwcnt, lclfixed;
    int       lclpcnt, lclscnt, lclacnt, lclnxtpcnt;
    int       lclnxtkcnt, lclnxtwcnt, lclnxtacnt, lclnxtscnt;
    int       gblnxtkcnt, gblnxtpcnt, gblnxtacnt, gblnxtscnt;
    int       gblfixed, gblkcount, gblacount, gblscount;
    int       *nxtargoffp, *argofflim, lclpmax;
    char      **strpool;
    int32      poolcount, strpool_cnt, argoffsize;
    int       nconsts;
    int       *constTbl;
    int32     *typemask_tabl;
    int32     *typemask_tabl_in, *typemask_tabl_out;
    int       lgprevdef;
    char      *filedir[101];
} OTRAN_GLOBALS;

static  int     gexist(CSOUND *, char *), gbloffndx(CSOUND *, char *);
static  int     lcloffndx(CSOUND *, char *);
static  int     constndx(CSOUND *, const char *);
static  int     strconstndx(CSOUND *, const char *);
static  void    insprep(CSOUND *, INSTRTXT *);
static  void    lgbuild(CSOUND *, char *, int inarg);
static  void    gblnamset(CSOUND *, char *);
static  int     plgndx(CSOUND *, char *);
static  NAME    *lclnamset(CSOUND *, char *);
/*        int     lgexist(CSOUND *, const char *);*/
static  void    delete_global_namepool(CSOUND *);
static  void    delete_local_namepool(CSOUND *);
static  int     pnum(char *s) ;
static  int     lgexist2(CSOUND *csound, const char *s);

extern void     print_tree(CSOUND *, char *, TREE *);

void close_instrument(CSOUND *csound, INSTRTXT * ip);

char argtyp2(CSOUND *csound, char *s);

#define txtcpy(a,b) memcpy(a,b,sizeof(TEXT));
#define ST(x)   (((OTRAN_GLOBALS*) ((CSOUND*) csound)->otranGlobals)->x)

#define KTYPE   1
#define WTYPE   2
#define ATYPE   3
#define PTYPE   4
#define STYPE   5
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

#define lblclear(x)
#if 0
/** This function body copied from rdorch.c, not currently used */
static void lblclear(CSOUND *csound)
{
    /* ST(lblcnt) = 0; */
}
#endif

static void intyperr(CSOUND *csound, int n, char *s, char *opname,
                     char tfound, char expect, int line)
{
    char    t[10];

    switch (tfound) {
    case 'w':
    case 'f':
    case 'a':
    case 'k':
    case 'i':
    case 'P':
    case 't':
    case 'p': t[0] = tfound;
      t[1] = '\0';
      break;
    case 'r':
    case 'c': strcpy(t,"const");
      break;
    case 'S': strcpy(t,"string");
      break;
    case 'b':
    case 'B': strcpy(t,"boolean");
      break;
    case '?': strcpy(t,"?");
      break;
  }
    synterr(csound, Str("input arg %d '%s' of type %s not allowed when "
                        "expecting %c (for opcode %s), line %d\n"),
            n+1, s, t, expect, opname, line);
}

#if 0
static void lblrequest(CSOUND *csound, char *s)
{
    /* for (req=0; req<ST(lblcnt); req++) */
    /*   if (strcmp(ST(lblreq)[req].label,s) == 0) */
    /*     return; */
    /* if (++ST(lblcnt) >= ST(lblmax)) { */
    /*   LBLREQ *tmp; */
    /*   ST(lblmax) += LBLMAX; */
    /*   tmp = mrealloc(csound, ST(lblreq), ST(lblmax) * sizeof(LBLREQ)); */
    /*   ST(lblreq) = tmp; */
    /* } */
    /* ST(lblreq)[req].reqline = ST(curline); */
    /* ST(lblreq)[req].label =s; */
}
#endif

static inline void resetouts(CSOUND *csound)
{
    csound->acount = csound->kcount = csound->icount =
      csound->Bcount = csound->bcount = 0;
}

/* Unused */
#if 0
TEXT *create_text(CSOUND *csound)
{
    TEXT        *tp = (TEXT *)mcalloc(csound, (int32)sizeof(TEXT));
    return tp;
}
#endif

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


/**
 * Current not used; intended to do the job of counting lcl counts
 * but is flawed as it does not take into account counting variables
 * only once if used multiple times; to be removed or reused in context
 * of redoing namset functions (if even desirable)
 */

/*
void update_lclcount(CSOUND *csound, INSTRTXT *ip, TREE *argslist)
{
    TREE * current = argslist;

    while (current != NULL) {
      switch(current->type) {
      case T_IDENT_S:
        ip->lclscnt++;
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "S COUNT INCREASED: %d\n", ip->lclscnt);
        break;
      case T_IDENT_W:
        ip->lclwcnt++;
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "W COUNT INCREASED: %d\n", ip->lclwcnt);
        break;
      case T_IDENT_A:
        ip->lclacnt++;
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "A COUNT INCREASED: %d\n", ip->lclacnt);
        break;
      case T_IDENT_K:
      case T_IDENT_F:
      case T_IDENT_I:
      case NUMBER_TOKEN:
      case INTEGER_TOKEN:
      default:
        ip->lclkcnt++;
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "K COUNT INCREASED: %d\n", ip->lclkcnt);
      }
      current = current->next;
    }
}
*/

void set_xincod(CSOUND *csound, TEXT *tp, OENTRY *ep, int line)
{
    int n = tp->inlist->count;
    char *s;
    char *types = ep->intypes;
    int nreqd = strlen(types);
    char      tfound = '\0', treqd;

    if (n > nreqd) {                 /* IV - Oct 24 2002: end of new code */
      if ((treqd = types[nreqd-1]) == 'n') {  /* indef args: */
        int incnt = -1;                       /* Should count args */
        if (!(incnt & 01))                    /* require odd */
          synterr(csound, Str("missing or extra arg"));
      }       /* IV - Sep 1 2002: added 'M' */
      else if (treqd != 'm' && treqd != 'z' && treqd != 'y' &&
               treqd != 'Z' && treqd != 'M' && treqd != 'N') /* else any no */
        synterr(csound, Str("too many input args\n"));
    }

    while (n--) {                     /* inargs:   */
      int32    tfound_m, treqd_m = 0L;
      s = tp->inlist->arg[n];

      if (n >= nreqd) {               /* det type required */
        csound->DebugMsg(csound, "%s(%d): type required: %c\n",
                         __FILE__, __LINE__, types[nreqd-1]);
        switch (types[nreqd-1]) {
        case 'M':
        case 'N':
        case 'Z':
        case 'y':
        case 'z':   treqd = types[nreqd-1]; break;
        default:    treqd = 'i';    /*   (indef in-type) */
        }
      }
      else treqd = types[n];          /*       or given)   */
      csound->DebugMsg(csound, "%s(%d): treqd: %c\n", __FILE__, __LINE__, treqd);
      if (treqd == 'l') {             /* if arg takes lbl  */
        csound->DebugMsg(csound, "treqd = l");
        //        lblrequest(csound, s);        /*      req a search */
        continue;                     /*      chk it later */
      }
      tfound = argtyp2(csound, s);     /* else get arg type */
      /* IV - Oct 31 2002 */
      tfound_m = ST(typemask_tabl)[(unsigned char) tfound];
      csound->DebugMsg(csound, "%s(%d): treqd: %c, tfound %c\n",
                       __FILE__, __LINE__,treqd, tfound);
      csound->DebugMsg(csound, "treqd %c, tfound_m %d ST(lgprevdef) %d\n",
                       treqd, tfound_m);
      if (!(tfound_m & (ARGTYP_c|ARGTYP_p)) && !ST(lgprevdef) && *s != '"') {
        synterr(csound,
                Str("input arg '%s' used before defined (in opcode %s),"
                    " line %d\n"),
                s, ep->opname, line);
      }
      if (tfound == 'a' && n < 31) /* JMC added for FOG */
                                   /* 4 for FOF, 8 for FOG; expanded to 15  */
        tp->xincod |= (1 << n);
      if (tfound == 'S' && n < 31)
        tp->xincod_str |= (1 << n);
      /* IV - Oct 31 2002: simplified code */
      if (!(tfound_m & ST(typemask_tabl_in)[(unsigned char) treqd])) {
      /* check for exceptional types */
      switch (treqd) {
      case 'Z':                             /* indef kakaka ... */
        if (!(tfound_m & (n & 1 ? ARGTYP_a : ARGTYP_ipcrk)))
          intyperr(csound, n, s, ep->opname, tfound, treqd, line);
        break;
      case 'x':
        treqd_m = ARGTYP_ipcr;              /* also allows i-rate */
      case 's':                             /* a- or k-rate */
      treqd_m |= ARGTYP_a | ARGTYP_k;
      if (tfound_m & treqd_m) {
        if (tfound == 'a' && tp->outlist->count != 0) {
          long outyp_m =                  /* ??? */
            ST(typemask_tabl)[(unsigned char) argtyp2(csound,
                                                     tp->outlist->arg[0])];
          if (outyp_m & (ARGTYP_a | ARGTYP_w)) break;
        }
        else
          break;
      }
      default:
        intyperr(csound, n, s, ep->opname, tfound, treqd, line);
        break;
      }
      }
    }
    csound->DebugMsg(csound, "xincod = %d", tp->xincod);
}

void set_xoutcod(CSOUND *csound, TEXT *tp, OENTRY *ep, int line)
{
    int n = tp->outlist->count;
    char *s;
    char *types = ep->outypes;
    int nreqd = -1;
    char      tfound = '\0', treqd;

    if (nreqd < 0)    /* for other opcodes */
      nreqd = strlen(types = ep->outypes);
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
      long    tfound_m;       /* IV - Oct 31 2002 */
      s = tp->outlist->arg[n];
      treqd = types[n];
      tfound = argtyp2(csound, s);                     /*  found    */
      /* IV - Oct 31 2002 */
      tfound_m = ST(typemask_tabl)[(unsigned char) tfound];
      /* IV - Sep 1 2002: xoutcod is the same as xincod for input */
      if (tfound == 'a' && n < 31)
        tp->xoutcod |= (1 << n);
      if (tfound == 'S' && n < 31)
        tp->xoutcod_str |= (1 << n);
      csound->DebugMsg(csound, "treqd %c, tfound %c", treqd, tfound);
      /* if (tfound_m & ARGTYP_w) */
      /*   if (ST(lgprevdef)) { */
      /*     synterr(csound, Str("output name previously used, " */
      /*                         "type '%c' must be uniquely defined, line %d"), */
      /*             tfound, line); */
      /*   } */
      /* IV - Oct 31 2002: simplified code */
      if (!(tfound_m & ST(typemask_tabl_out)[(unsigned char) treqd])) {
        synterr(csound, Str("output arg '%s' illegal type (for opcode %s),"
                            " line %d\n"),
                s, ep->opname, line);
      }
    }
}

/**
 * Create an Opcode (OPTXT) from the AST node given. Called from
 * create_udo and create_instrument.
 */
OPTXT *create_opcode(CSOUND *csound, TREE *root, INSTRTXT *ip)
{
    TEXT *tp;
    TREE *inargs, *outargs;
    OPTXT *optxt, *retOptxt = NULL;
    char *arg;
    int opnum;
    int n, nreqd;;

    /* printf("%d(%d): tree=%p\n", __FILE__, __LINE__, root); */
    /* print_tree(csound, "create_opcode", root); */
    optxt = (OPTXT *) mcalloc(csound, (int32)sizeof(OPTXT));
    tp = &(optxt->t);

    switch(root->type) {
    case LABEL_TOKEN:
      /* TODO - Need to verify here or elsewhere that this label is not
         already defined */
      tp->opnum = LABEL;
      tp->opcod = strsav_string(csound, root->value->lexeme);

      tp->outlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
      tp->outlist->count = 0;
      tp->inlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
      tp->inlist->count = 0;

      ip->mdepends |= csound->opcodlst[LABEL].thread;
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
        if (nreqd > OPCODENUMOUTS_HIGH)
          opnum = find_opcode(csound, ".xin256");
        else
          opnum = find_opcode(csound, ".xin64");
      }
      else {
        opnum = find_opcode(csound, root->value->lexeme);
      }

      /* INITIAL SETUP */
      tp->opnum = opnum;
      tp->opcod = strsav_string(csound, csound->opcodlst[opnum].opname);
      ip->mdepends |= csound->opcodlst[opnum].thread;
      ip->opdstot += csound->opcodlst[opnum].dsblksiz;

      /* BUILD ARG LISTS */
      {
        int incount = tree_arg_list_count(root->right);
        int outcount = tree_arg_list_count(root->left);
        int argcount = 0;

        //    csound->Message(csound, "Tree: In Count: %d\n", incount);
        //    csound->Message(csound, "Tree: Out Count: %d\n", outcount);

        size_t m = sizeof(ARGLST) + (incount - 1) * sizeof(char*);
        tp->inlist = (ARGLST*) mrealloc(csound, tp->inlist, m);
        tp->inlist->count = incount;

        m = sizeof(ARGLST) + (outcount - 1) * sizeof(char*);
        tp->outlist = (ARGLST*) mrealloc(csound, tp->outlist, m);
        tp->outlist->count = outcount;


        for (inargs = root->right; inargs != NULL; inargs = inargs->next) {
          /* INARGS */

          //      csound->Message(csound, "IN ARG TYPE: %d\n", inargs->type);

          arg = inargs->value->lexeme;

          tp->inlist->arg[argcount++] = strsav_string(csound, arg);

          if ((n = pnum(arg)) >= 0) {
            if (n > ip->pmax)  ip->pmax = n;
          }
          /* VL 14/12/11 : calling lgbuild here seems to be problematic for
             undef arg checks */
          else {
            lgbuild(csound, arg, 1);
          }


        }

        /* update_lclcount(csound, ip, root->right); */

        argcount = 0;

        /* OUTARGS */
        for (outargs = root->left; outargs != NULL; outargs = outargs->next) {

          arg = outargs->value->lexeme;

          tp->outlist->arg[argcount++] =
            strsav_string(csound, arg);

          if ((n = pnum(arg)) >= 0) {
            if (n > ip->pmax)  ip->pmax = n;
          }
          else {
            if (arg[0] == 'w' &&
                lgexist2(csound, arg) != 0) {
              synterr(csound, Str("output name previously used, "
                                  "type 'w' must be uniquely defined, line %d"),
                      root->line);
            }
            lgbuild(csound, arg, 0);
          }

        }
      }
      /* update_lclcount(csound, ip, root->left); */


      /* VERIFY ARG LISTS MATCH OPCODE EXPECTED TYPES */

      {
        OENTRY *ep = csound->opcodlst + tp->opnum;

        //        csound->Message(csound, "Opcode InTypes: %s\n", ep->intypes);
        //        csound->Message(csound, "Opcode OutTypes: %s\n", ep->outypes);

        set_xincod(csound, tp, ep, root->line);
        set_xoutcod(csound, tp, ep, root->line);

        if (root->right != NULL) {
          if (ep->intypes[0] != 'l') {     /* intype defined by 1st inarg */
            tp->intype = argtyp2(csound, tp->inlist->arg[0]);
          }
          else  {
            tp->intype = 'l';          /*   (unless label)  */
          }
        }

        if (root->left != NULL) {      /* pftype defined by outarg */
          tp->pftype = argtyp2(csound, root->left->value->lexeme);
        }
        else {                            /*    else by 1st inarg     */
          tp->pftype = tp->intype;
        }

//        csound->Message(csound,
//                        Str("create_opcode[%s]: opnum for opcode: %d\n"),
//                        root->value->lexeme, opnum);
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
 * Create a UDO (INSTRTXT) from the AST node given. Called from
 * csound_orc_compile.
 */
INSTRTXT *create_udo(CSOUND *csound, TREE *root)
{
    INSTRTXT *ip = (INSTRTXT *) mcalloc(csound, sizeof(INSTRTXT));
    return ip;
}

/**
 * Create an Instrument (INSTRTXT) from the AST node given for use as
 * Instrument0. Called from csound_orc_compile.
 */
INSTRTXT *create_instrument0(CSOUND *csound, TREE *root)
{
    INSTRTXT *ip;
    OPTXT *op;

    TREE *current;

    ip = (INSTRTXT *) mcalloc(csound, sizeof(INSTRTXT));
    op = (OPTXT *)ip;

    current = root;

    /* initialize */
    ip->lclkcnt = 0;
    ip->lclwcnt = 0;
    ip->lclacnt = 0;
    ip->lclpcnt = 0;
    ip->lclscnt = 0;

    delete_local_namepool(csound);
    ST(lclnxtkcnt) = 0;                     /*   for rebuilding  */
    ST(lclnxtwcnt) = ST(lclnxtacnt) = 0;
    ST(lclnxtpcnt) = ST(lclnxtscnt) = 0;

    ip->mdepends = 0;
    ip->opdstot = 0;

    ip->pmax = 3L;

    /* start chain */
    ip->t.opnum = INSTR;
    ip->t.opcod = strsav_string(csound, "instr"); /*  to hold global assigns */

      /* The following differs from otran and needs review.  otran keeps a
       * nulllist to point to for empty lists, while this is creating a new list
       * regardless
       */
    ip->t.outlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ip->t.outlist->count = 0;
    ip->t.inlist = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ip->t.inlist->count = 1;

    ip->t.inlist->arg[0] = strsav_string(csound, "0");


    while (current != NULL) {

      if (current->type != INSTR_TOKEN && current->type != UDO_TOKEN) {

        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "In INSTR 0: %s\n", current->value->lexeme);

        if (current->type == '='
           && strcmp(current->value->lexeme, "=.r") == 0) {

          MYFLT val = csound->pool[constndx(csound,
                                            current->right->value->lexeme)];


          /* if (current->right->type == INTEGER_TOKEN) {
             val = FL(current->right->value->value);
             } else {
             val = FL(current->right->value->fvalue);
             }*/

          /* modify otran defaults*/
          if (current->left->type == SRATE_TOKEN) {
            csound->tran_sr = val;
          }
          else if (current->left->type == KRATE_TOKEN) {
            csound->tran_kr = val;
          }
          else if (current->left->type == KSMPS_TOKEN) {
            csound->tran_ksmps = val;
          }
          else if (current->left->type == NCHNLS_TOKEN) {
            csound->tran_nchnls = current->right->value->value;
          }
          else if (current->left->type == NCHNLSI_TOKEN) {
            csound->tran_nchnlsi = current->right->value->value;
            /* csound->Message(csound, "SETTING NCHNLS: %d\n",
                               csound->tran_nchnls); */
          }
          else if (current->left->type == ZERODBFS_TOKEN) {
            csound->tran_0dbfs = val;
            /* csound->Message(csound, "SETTING 0DBFS: %f\n",
                               csound->tran_0dbfs); */
          }

        }

        op->nxtop = create_opcode(csound, current, ip);

        op = last_optxt(op);

        }
        current = current->next;
    }

    close_instrument(csound, ip);

    return ip;
}


/**
 * Create an Instrument (INSTRTXT) from the AST node given. Called from
 * csound_orc_compile.
 */
INSTRTXT *create_instrument(CSOUND *csound, TREE *root)
{
    INSTRTXT *ip;
    OPTXT *op;
    char *c;

    TREE *statements, *current;

    ip = (INSTRTXT *) mcalloc(csound, sizeof(INSTRTXT));
    op = (OPTXT *)ip;
    statements = root->right;

    ip->lclkcnt = 0;
    ip->lclwcnt = 0;
    ip->lclacnt = 0;
    ip->lclpcnt = 0;
    ip->lclscnt = 0;

    delete_local_namepool(csound);
    ST(lclnxtkcnt) = 0;                     /*   for rebuilding  */
    ST(lclnxtwcnt) = ST(lclnxtacnt) = 0;
    ST(lclnxtpcnt) = ST(lclnxtscnt) = 0;


    ip->mdepends = 0;
    ip->opdstot = 0;

    ip->pmax = 3L;

    /* Initialize */
    ip->t.opnum = INSTR;
    ip->t.opcod = strsav_string(csound, "instr"); /*  to hold global assigns */

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
      sprintf(c, "%ld", instrNum);

      if (PARSER_DEBUG)
          csound->Message(csound,
                          Str("create_instrument: instr num %ld\n"), instrNum);

      ip->t.inlist->arg[0] = strsav_string(csound, c);

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
      /* IV - Oct 31 2002: some error checking */
      if (UNLIKELY(!check_instr_name(c))) {
        synterr(csound, Str("invalid name for instrument"));
      }
      /* IV - Oct 31 2002: store the name */
      if (UNLIKELY(!named_instr_alloc(csound, c, ip, insno_priority))) {
        synterr(csound, Str("instr %s redefined"), c);
      }
      ip->insname = c;  /* IV - Nov 10 2002: also in INSTRTXT */

    }


    current = statements;

    while (current != NULL) {
        OPTXT * optxt = create_opcode(csound, current, ip);

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
    bp->t.opcod = strsav_string(csound, "endin"); /*  term instr 0 blk */
    bp->t.outlist = bp->t.inlist = NULL;

    bp->nxtop = NULL;   /* terminate the optxt chain */

    current = (OPTXT *)ip;

    while (current->nxtop != NULL) {
        current = current->nxtop;
    }

    current->nxtop = bp;


    ip->lclkcnt = ST(lclnxtkcnt);
    /* align to 8 bytes for "spectral" types */
    if ((int) sizeof(MYFLT) < 8 &&
        (ST(lclnxtwcnt) + ST(lclnxtpcnt)) > 0)
      ip->lclkcnt = (ip->lclkcnt + 1) & (~1);
    ip->lclwcnt = ST(lclnxtwcnt);
    ip->lclacnt = ST(lclnxtacnt);
    ip->lclpcnt = ST(lclnxtpcnt);
    ip->lclscnt = ST(lclnxtscnt);
    ip->lclfixed = ST(lclnxtkcnt) + ST(lclnxtwcnt) * Wfloats
                                  + ST(lclnxtpcnt) * Pfloats;

    /* align to 8 bytes for "spectral" types */
/*    if ((int) sizeof(MYFLT) < 8 && (ip->lclwcnt + ip->lclpcnt) > 0) {
          ip->lclkcnt = (ip->lclkcnt + 1) & (~1);
    }

    ip->lclfixed = ip->lclkcnt +
                   ip->lclwcnt * Wfloats * ip->lclpcnt * Pfloats;*/

    ip->mdepends = ip->mdepends >> 4;

    ip->pextrab = ((n = ip->pmax - 3L) > 0 ? (int) n * sizeof(MYFLT) : 0);
    ip->pextrab = ((int) ip->pextrab + 7) & (~7);

    ip->muted = 1;

    /*ip->pmax = (int)pmax;
    ip->pextrab = ((n = pmax-3L) > 0 ? (int) n * sizeof(MYFLT) : 0);
    ip->pextrab = ((int) ip->pextrab + 7) & (~7);
    ip->mdepends = threads >> 4;
    ip->lclkcnt = ST(lclnxtkcnt); */
    /* align to 8 bytes for "spectral" types */

    /*if ((int) sizeof(MYFLT) < 8 &&
        (ST(lclnxtwcnt) + ST(lclnxtpcnt)) > 0)
      ip->lclkcnt = (ip->lclkcnt + 1) & (~1);
    ip->lclwcnt = ST(lclnxtwcnt);
    ip->lclacnt = ST(lclnxtacnt);
    ip->lclpcnt = ST(lclnxtpcnt);
    ip->lclscnt = ST(lclnxtscnt);
    ip->lclfixed = ST(lclnxtkcnt) + ST(lclnxtwcnt) * Wfloats
                                  + ST(lclnxtpcnt) * Pfloats;*/
    /*ip->opdstot = opdstot;*/      /* store total opds reqd */
    /*ip->muted = 1;*/              /* Allow to play */

}



/**
 * Append an instrument to the end of Csound's linked list of instruments
 */
void append_instrument(CSOUND * csound, INSTRTXT * instrtxt)
{
    INSTRTXT    *current = &(csound->instxtanchor);
    while (current->nxtinstxt != NULL) {
      current = current->nxtinstxt;
    }

    current->nxtinstxt = instrtxt;
}

static int pnum(char *s)        /* check a char string for pnum format  */
                                /*   and return the pnum ( >= 0 )       */
{                               /* else return -1                       */
    int n;

    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return(n);
    return(-1);
}

/** Insert INSTRTXT into Csound's list of INSTRTXT's, checking to see if number
 * is greater than number of pointers currently allocated and if so expand
 * pool of instruments
 */
void insert_instrtxt(CSOUND *csound, INSTRTXT *instrtxt, int32 instrNum) {
    int i;

    if (UNLIKELY(instrNum > csound->maxinsno)) {
        int old_maxinsno = csound->maxinsno;

        /* expand */
        while (instrNum > csound->maxinsno) {
            csound->maxinsno += MAXINSNO;
        }

        csound->instrtxtp = (INSTRTXT**)mrealloc(csound,
                csound->instrtxtp, (1 + csound->maxinsno) * sizeof(INSTRTXT*));

        /* Array expected to be nulled so.... */
        for (i = old_maxinsno + 1; i <= csound->maxinsno; i++) {
              csound->instrtxtp[i] = NULL;
        }
    }

    if (UNLIKELY(csound->instrtxtp[instrNum] != NULL)) {
        synterr(csound, Str("instr %ld redefined"), instrNum);
        /* err++; continue; */
    }

    csound->instrtxtp[instrNum] = instrtxt;
}

OPCODINFO *find_opcode_info(CSOUND *csound, char *opname)
{
    OPCODINFO *opinfo = csound->opcodeInfo;
    if (UNLIKELY(opinfo == NULL)) {
      csound->Message(csound, Str("!!! csound->opcodeInfo is NULL !!!\n"));
        return NULL;
    }

    while (opinfo != NULL) {
        csound->Message(csound, "%s : %s\n", opinfo->name, opname);
        if (UNLIKELY(strcmp(opinfo->name, opname) == 0)) {
            return opinfo;
        }
        opinfo = opinfo->prv;   /* Move on: JPff suggestion */
    }

    return NULL;
}

/**
 * Compile the given TREE node into structs for Csound to use
 */
void csound_orc_compile(CSOUND *csound, TREE *root)
{
//    csound->Message(csound, "Begin Compiling AST (Currently Implementing)\n");

    OPARMS      *O = csound->oparms;
    INSTRTXT    *instrtxt = NULL;
    INSTRTXT    *ip = NULL;
    INSTRTXT    *prvinstxt = &(csound->instxtanchor);
    OPTXT       *bp;
    char        *opname;
    int32        count, sumcount, instxtcount, optxtcount;
    TREE * current = root;
    INSTRTXT * instr0;

    strsav_create(csound);

    if (UNLIKELY(csound->otranGlobals == NULL)) {
      csound->otranGlobals = csound->Calloc(csound, sizeof(OTRAN_GLOBALS));
    }
    csound->instrtxtp = (INSTRTXT **) mcalloc(csound, (1 + csound->maxinsno)
                                                      * sizeof(INSTRTXT*));
    // csound->opcodeInfo = NULL;          /* IV - Oct 20 2002 */

    strconstndx(csound, "\"\"");

    gblnamset(csound, "sr");    /* enter global reserved words */
    gblnamset(csound, "kr");
    gblnamset(csound, "ksmps");
    gblnamset(csound, "nchnls");
    gblnamset(csound, "nchnls_i");
    gblnamset(csound, "0dbfs"); /* no commandline override for that! */
    gblnamset(csound, "$sr");   /* incl command-line overrides */
    gblnamset(csound, "$kr");
    gblnamset(csound, "$ksmps");

    csound->pool = (MYFLT*) mmalloc(csound, NCONSTS * sizeof(MYFLT));
    ST(poolcount) = 0;
    ST(nconsts) = NCONSTS;
    ST(constTbl) = (int*) mcalloc(csound, (256 + NCONSTS) * sizeof(int));
    constndx(csound, "0");

    if (!ST(typemask_tabl)) {
      const int32 *ptr = typetabl1;
      ST(typemask_tabl) = (int32*) mcalloc(csound, sizeof(int32) * 256);
      ST(typemask_tabl_in) = (int32*) mcalloc(csound, sizeof(int32) * 256);
      ST(typemask_tabl_out) = (int32*) mcalloc(csound, sizeof(int32) * 256);
      while (*ptr) {            /* basic types (both for input */
        int32 pos = *ptr++;      /* and output) */
        ST(typemask_tabl)[pos] = ST(typemask_tabl_in)[pos] =
          ST(typemask_tabl_out)[pos] = *ptr++;
      }
      ptr = typetabl2;
      while (*ptr) {            /* input types */
        int32 pos = *ptr++;
        ST(typemask_tabl_in)[pos] = *ptr++;
      }
      ptr = typetabl3;
      while (*ptr) {            /* output types */
        int32 pos = *ptr++;
        ST(typemask_tabl_out)[pos] = *ptr++;
      }
    }
    instr0 = create_instrument0(csound, root);
    prvinstxt = prvinstxt->nxtinstxt = instr0;
    insert_instrtxt(csound, instr0, 0);

    while (current != NULL) {

      switch (current->type) {
      case T_INIT:
      case '=':
        /* csound->Message(csound, "Assignment found\n"); */
        break;
      case INSTR_TOKEN:
        /* csound->Message(csound, "Instrument found\n"); */

        resetouts(csound); /* reset #out counts */
        lblclear(csound); /* restart labelist  */

        instrtxt = create_instrument(csound, current);

        prvinstxt = prvinstxt->nxtinstxt = instrtxt;

        /* Handle Inserting into CSOUND here by checking ids (name or
         * numbered) and using new insert_instrtxt?
         */
        //printf("Starting to install instruments\n");
        /* Temporarily using the following code */
        if (current->left->type == INTEGER_TOKEN) { /* numbered instrument */
          int32 instrNum = (int32)current->left->value->value;

          insert_instrtxt(csound, instrtxt, instrNum);
        }
        else if (current->left->type == T_INSTLIST) {
          TREE *p =  current->left;
          //printf("instlist case:\n"); /* This code is suspect */
          while (p) {
            if (PARSER_DEBUG) print_tree(csound, "Top of loop\n", p);
            if (p->left) {
              //print_tree(csound, "Left\n", p->left);
              if (p->left->type == INTEGER_TOKEN) {
                insert_instrtxt(csound, instrtxt, p->left->value->value);
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
                if (UNLIKELY(!named_instr_alloc(csound, c, instrtxt, insno_priority))) {
                  synterr(csound, Str("instr %s redefined"), c);
                }
                instrtxt->insname = c;
              }
            }
            else {
              if (p->type == INTEGER_TOKEN) {
                insert_instrtxt(csound, instrtxt, p->value->value);
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
                if (UNLIKELY(!named_instr_alloc(csound, c, instrtxt, insno_priority))) {
                  synterr(csound, Str("instr %s redefined"), c);
                }
                instrtxt->insname = c;
              }
              break;
            }
            p = p->right;
          }
        }
        break;
      case UDO_TOKEN:
        /* csound->Message(csound, "UDO found\n"); */

        resetouts(csound); /* reset #out counts */
        lblclear(csound); /* restart labelist  */

        instrtxt = create_instrument(csound, current);

        prvinstxt = prvinstxt->nxtinstxt = instrtxt;

        opname = current->left->value->lexeme;

        /* csound->Message(csound, */
        /*     "Searching for OPCODINFO for opname: %s\n", opname); */

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

    /* Begin code from otran */
    /* now add the instruments with names, assigning them fake instr numbers */
    named_instr_assign_numbers(csound);         /* IV - Oct 31 2002 */
    if (csound->opcodeInfo) {
      int num = csound->maxinsno;       /* store after any other instruments */
      OPCODINFO *inm = csound->opcodeInfo;
      /* IV - Oct 31 2002: now add user defined opcodes */
      while (inm) {
        /* we may need to expand the instrument array */
        if (UNLIKELY(++num > csound->maxopcno)) {
          int i;
          i = (csound->maxopcno > 0 ? csound->maxopcno : csound->maxinsno);
          csound->maxopcno = i + MAXINSNO;
          csound->instrtxtp = (INSTRTXT**)
            mrealloc(csound, csound->instrtxtp, (1 + csound->maxopcno)
                                                * sizeof(INSTRTXT*));
          /* Array expected to be nulled so.... */
          while (++i <= csound->maxopcno) csound->instrtxtp[i] = NULL;
        }
        inm->instno = num;

        /* csound->Message(csound, "UDO INSTR NUM: %d\n", num); */

        csound->instrtxtp[num] = inm->ip;
        inm = inm->prv;
      }
    }
    /* Deal with defaults and consistency */
    if (csound->tran_ksmps == FL(-1.0)) {
      if (csound->tran_sr == FL(-1.0)) csound->tran_sr = DFLT_SR;
      if (csound->tran_kr == FL(-1.0)) csound->tran_kr = DFLT_KR;
      csound->tran_ksmps = (MYFLT) ((int) (csound->tran_sr / csound->tran_kr
                                           + FL(0.5)));
    }
    else if (csound->tran_kr == FL(-1.0)) {
      if (csound->tran_sr == FL(-1.0)) csound->tran_sr = DFLT_SR;
      csound->tran_kr = csound->tran_sr / csound->tran_ksmps;
    }
    else if (csound->tran_sr == FL(-1.0)) {
      csound->tran_sr = csound->tran_kr * csound->tran_ksmps;
    }
    /* That deals with missing values, however we do need ksmps to be integer */
    {
      CSOUND    *p = (CSOUND*) csound;
      char      err_msg[128];
      sprintf(err_msg, "sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:",
                       p->tran_sr, p->tran_kr, p->tran_ksmps);
      if (UNLIKELY(p->tran_sr <= FL(0.0)))
        synterr(p, Str("%s invalid sample rate"), err_msg);
      if (UNLIKELY(p->tran_kr <= FL(0.0)))
        synterr(p, Str("%s invalid control rate"), err_msg);
      else if (UNLIKELY(p->tran_ksmps < FL(0.75) ||
                        FLOAT_COMPARE(p->tran_ksmps,
                                      MYFLT2LRND(p->tran_ksmps))))
        synterr(p, Str("%s invalid ksmps value"), err_msg);
      else if (UNLIKELY(FLOAT_COMPARE(p->tran_sr,
                                      (double) p->tran_kr * p->tran_ksmps)))
        synterr(p, Str("%s inconsistent sr, kr, ksmps"), err_msg);
    }

    ip = csound->instxtanchor.nxtinstxt;
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
    if (UNLIKELY(csound->synterrcnt)) {
      print_opcodedir_warning(csound);
      csound->Die(csound, Str("%d syntax errors in orchestra.  "
                              "compilation invalid\n"), csound->synterrcnt);
    }
    if (UNLIKELY(O->odebug)) {
      int32  n;
      MYFLT *p;
      csound->Message(csound, "poolcount = %ld, strpool_cnt = %ld\n",
                              ST(poolcount), ST(strpool_cnt));
      csound->Message(csound, "pool:");
      for (n = ST(poolcount), p = csound->pool; n--; p++)
        csound->Message(csound, "\t%g", *p);
      csound->Message(csound, "\n");
      csound->Message(csound, "strpool:");
      for (n = 0L; n < ST(strpool_cnt); n++)
        csound->Message(csound, "\t%s", ST(strpool)[n]);
      csound->Message(csound, "\n");
    }
    ST(gblfixed) = ST(gblnxtkcnt) + ST(gblnxtpcnt) * (int) Pfloats;
    ST(gblkcount) = ST(gblnxtkcnt);
    /* align to 8 bytes for "spectral" types */
    if ((int) sizeof(MYFLT) < 8 && ST(gblnxtpcnt))
      ST(gblkcount) = (ST(gblkcount) + 1) & (~1);
    ST(gblacount) = ST(gblnxtacnt);
    ST(gblscount) = ST(gblnxtscnt);

    ip = &(csound->instxtanchor);
    for (sumcount = 0; (ip = ip->nxtinstxt) != NULL; ) {/* for each instxt */
      OPTXT *optxt = (OPTXT *) ip;
      int optxtcount = 0;
      while ((optxt = optxt->nxtop) != NULL) {      /* for each op in instr  */
        TEXT *ttp = &optxt->t;
        optxtcount += 1;
        if (ttp->opnum == ENDIN                     /*    (until ENDIN)      */
            || ttp->opnum == ENDOP) break;  /* (IV - Oct 26 2002: or ENDOP) */
        if ((count = ttp->inlist->count)!=0)
          sumcount += count +1;                     /* count the non-nullist */
        if ((count = ttp->outlist->count)!=0)       /* slots in all arglists */
          sumcount += (count + 1);
      }
      ip->optxtcount = optxtcount;                  /* optxts in this instxt */
    }
    ST(argoffsize) = (sumcount + 1) * sizeof(int);  /* alloc all plus 1 null */
    /* as argoff ints */
    csound->argoffspace = (int*) mmalloc(csound, ST(argoffsize));
    ST(nxtargoffp) = csound->argoffspace;
    ST(nulloffs) = (ARGOFFS *) csound->argoffspace; /* setup the null argoff */
    *ST(nxtargoffp)++ = 0;
    ST(argofflim) = ST(nxtargoffp) + sumcount;
    ip = &(csound->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL)        /* add all other entries */
      insprep(csound, ip);                      /*   as combined offsets */
    if (UNLIKELY(O->odebug)) {
      int *p = csound->argoffspace;
      csound->Message(csound, "argoff array:\n");
      do {
        csound->Message(csound, "\t%d", *p++);
      } while (p < ST(argofflim));
      csound->Message(csound, "\n");
    }
    if (UNLIKELY(ST(nxtargoffp) != ST(argofflim)))
      csoundDie(csound, Str("inconsistent argoff sumcount"));

    ip = &(csound->instxtanchor);               /* set the OPARMS values */
    instxtcount = optxtcount = 0;
    while ((ip = ip->nxtinstxt) != NULL) {
      instxtcount += 1;
      optxtcount += ip->optxtcount;
    }
    csound->instxtcount = instxtcount;
    csound->optxtsize = instxtcount * sizeof(INSTRTXT)
                        + optxtcount * sizeof(OPTXT);
    csound->poolcount = ST(poolcount);
    csound->gblfixed = ST(gblnxtkcnt) + ST(gblnxtpcnt) * (int) Pfloats;
    csound->gblacount = ST(gblnxtacnt);
    csound->gblscount = ST(gblnxtscnt);
    /* clean up */
    delete_local_namepool(csound);
    delete_global_namepool(csound);
    mfree(csound, ST(constTbl));
    ST(constTbl) = NULL;
    /* End code from otran */

    /* csound->Message(csound, "End Compiling AST\n"); */

}


/* prep an instr template for efficient allocs  */
/* repl arg refs by offset ndx to lcl/gbl space */
static void insprep(CSOUND *csound, INSTRTXT *tp)
{
    OPARMS      *O = csound->oparms;
    OPTXT       *optxt;
    OENTRY      *ep;
    int         n, opnum, inreqd;
    char        **argp;
    char        **labels, **lblsp;
    LBLARG      *larg, *largp;
    ARGLST      *outlist, *inlist;
    ARGOFFS     *outoffs, *inoffs;
    int         indx, *ndxp;

    labels = (char **)mmalloc(csound, (csound->nlabels) * sizeof(char *));
    lblsp = labels;
    larg = (LBLARG *)mmalloc(csound, (csound->ngotos) * sizeof(LBLARG));
    largp = larg;
    ST(lclkcnt) = tp->lclkcnt;
    ST(lclwcnt) = tp->lclwcnt;
    ST(lclfixed) = tp->lclfixed;
    ST(lclpcnt) = tp->lclpcnt;
    ST(lclscnt) = tp->lclscnt;
    ST(lclacnt) = tp->lclacnt;
    delete_local_namepool(csound);              /* clear lcl namlist */
    ST(lclnxtkcnt) = 0;                         /*   for rebuilding  */
    ST(lclnxtwcnt) = ST(lclnxtacnt) = 0;
    ST(lclnxtpcnt) = ST(lclnxtscnt) = 0;
    ST(lclpmax) = tp->pmax;                     /* set pmax for plgndx */
    ndxp = ST(nxtargoffp);
    optxt = (OPTXT *)tp;
    while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
      TEXT *ttp = &optxt->t;
      if ((opnum = ttp->opnum) == ENDIN         /*  (until ENDIN)  */
          || opnum == ENDOP)            /* (IV - Oct 31 2002: or ENDOP) */
        break;
      if (opnum == LABEL) {
        if (lblsp - labels >= csound->nlabels) {
          int oldn = lblsp - labels;
          csound->nlabels += NLABELS;
          if (lblsp - labels >= csound->nlabels)
            csound->nlabels = lblsp - labels + 2;
          if (csound->oparms->msglevel)
            csound->Message(csound,
                            Str("LABELS list is full...extending to %d\n"),
                            csound->nlabels);
          labels =
            (char**)mrealloc(csound, labels, csound->nlabels*sizeof(char*));
          lblsp = &labels[oldn];
        }
        *lblsp++ = ttp->opcod;
        continue;
      }
      ep = &(csound->opcodlst[opnum]);
      if (O->odebug) csound->Message(csound, "%s argndxs:", ep->opname);
      if ((outlist = ttp->outlist) == ST(nullist) || !outlist->count)
        ttp->outoffs = ST(nulloffs);
      else {
        ttp->outoffs = outoffs = (ARGOFFS *) ndxp;
        outoffs->count = n = outlist->count;
        argp = outlist->arg;                    /* get outarg indices */
        ndxp = outoffs->indx;
        while (n--) {
          *ndxp++ = indx = plgndx(csound, *argp++);
          if (O->odebug) csound->Message(csound, "\t%d", indx);
        }
      }
      if ((inlist = ttp->inlist) == ST(nullist) || !inlist->count)
        ttp->inoffs = ST(nulloffs);
      else {
        ttp->inoffs = inoffs = (ARGOFFS *) ndxp;
        inoffs->count = inlist->count;
        inreqd = strlen(ep->intypes);
        argp = inlist->arg;                     /* get inarg indices */
        ndxp = inoffs->indx;
        for (n=0; n < inlist->count; n++, argp++, ndxp++) {
          if (n < inreqd && ep->intypes[n] == 'l') {
            if (largp - larg >= csound->ngotos) {
              int oldn = csound->ngotos;
              csound->ngotos += NGOTOS;
              if (csound->oparms->msglevel)
                csound->Message(csound,
                                Str("GOTOS list is full..extending to %d\n"),
                                csound->ngotos);
              if (largp - larg >= csound->ngotos)
                csound->ngotos = largp - larg + 1;
              larg = (LBLARG *)
                mrealloc(csound, larg, csound->ngotos * sizeof(LBLARG));
              largp = &larg[oldn];
            }
            if (UNLIKELY(O->odebug))
              csound->Message(csound, "\t***lbl");  /* if arg is label,  */
            largp->lbltxt = *argp;
            largp->ndxp = ndxp;                     /*  defer till later */
            largp++;
          }
          else {
            char *s = *argp;
            indx = plgndx(csound, s);
            if (UNLIKELY(O->odebug)) csound->Message(csound, "\t%d", indx);
            *ndxp = indx;
          }
        }
      }
      if (UNLIKELY(O->odebug)) csound->Message(csound, "\n");
    }
 nxt:
    while (--largp >= larg) {                   /* resolve the lbl refs */
      char *s = largp->lbltxt;
      char **lp;
      for (lp = labels; lp < lblsp; lp++)
        if (strcmp(s, *lp) == 0) {
          *largp->ndxp = lp - labels + LABELOFS;
          goto nxt;
        }
      csoundDie(csound, Str("target label '%s' not found"), s);
    }
    ST(nxtargoffp) = ndxp;
    mfree(csound, labels);
    mfree(csound, larg);
}

/* returns non-zero if 's' is defined in the global or local pool of names */

static int lgexist2(CSOUND *csound, const char *s)
{
    unsigned char h = name_hash(csound, s);
    NAME          *p = NULL;
    for (p = ST(gblNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)
      return 1;
    for (p = ST(lclNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
    return (p == NULL ? 0 : 1);
}

/* build pool of floating const values  */
/* build lcl/gbl list of ds names, offsets */
/* (no need to save the returned values) */
static void lgbuild(CSOUND *csound, char *s, int inarg)
{
    char    c;

    c = *s;
    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0))
      constndx(csound, s);
    else if (c == '"')
      strconstndx(csound, s);
    else if (!(lgexist2(csound, s)) && !inarg) {
      if (c == 'g' || (c == '#' && s[1] == 'g'))
        gblnamset(csound, s);
      else
        lclnamset(csound, s);
    }
}

/* get storage ndx of const, pnum, lcl or gbl */
/* argument const/gbl indexes are positiv+1, */
/* pnum/lcl negativ-1 called only after      */
/* poolcount & lclpmax are finalised */
static int plgndx(CSOUND *csound, char *s)
{
    char        c;
    int         n, indx;

    c = *s;

    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0))
      indx = constndx(csound, s) + 1;
    else if (c == '"')
      indx = strconstndx(csound, s) + STR_OFS + 1;
    else if ((n = pnum(s)) >= 0)
      indx = -n;
    else if (c == 'g' || (c == '#' && *(s+1) == 'g') || gexist(csound, s))
      indx = (int) (ST(poolcount) + 1 + gbloffndx(csound, s));
    else
      indx = -(ST(lclpmax) + 1 + lcloffndx(csound, s));
/*    csound->Message(csound, " [%s -> %d (%x)]\n", s, indx, indx); */
    return(indx);
}

/* get storage ndx of string const value */
/* builds value pool on 1st occurrence   */
static int strconstndx(CSOUND *csound, const char *s)
{
    int     i, cnt;

    /* check syntax */
    cnt = (int) strlen(s);
    if (UNLIKELY(cnt < 2 || *s != '"' || s[cnt - 1] != '"')) {
      synterr(csound, Str("string syntax '%s'"), s);
      return 0;
    }
    /* check if a copy of the string is already stored */
    for (i = 0; i < ST(strpool_cnt); i++) {
      if (strcmp(s, ST(strpool)[i]) == 0)
        return i;
    }
    /* not found, store new string */
    cnt = ST(strpool_cnt)++;
    if (!(cnt & 0x7F)) {
      /* extend list */
      if (!cnt) ST(strpool) = csound->Malloc(csound, 0x80 * sizeof(MYFLT*));
      else      ST(strpool) = csound->ReAlloc(csound, ST(strpool),
                                              (cnt + 0x80) * sizeof(MYFLT*));
    }
    ST(strpool)[cnt] = (char*) csound->Malloc(csound, strlen(s) + 1);
    strcpy(ST(strpool)[cnt], s);
    /* and return index */
    return cnt;
}

static inline unsigned int MYFLT_hash(const MYFLT *x)
{
    const unsigned char *c = (const unsigned char*) x;
    size_t              i;
    unsigned int        h = 0U;

    for (i = (size_t) 0; i < sizeof(MYFLT); i++)
      h = (unsigned int) strhash_tabl_8[(unsigned int) c[i] ^ h];

    return h;
}

/* get storage ndx of float const value */
/* builds value pool on 1st occurrence  */
/* final poolcount used in plgndx above */
/* pool may be moved w. ndx still valid */

static int constndx(CSOUND *csound, const char *s)
{
    MYFLT   newval;
    int     h, n, prv;

    {
      volatile MYFLT  tmpVal;   /* make sure it really gets rounded to MYFLT */
      char            *tmp = (char*) s;
      tmpVal = (MYFLT) strtod(s, &tmp);
      newval = tmpVal;
      if (UNLIKELY(tmp == s || *tmp != (char) 0)) {
        synterr(csound, Str("numeric syntax '%s'"), s);
        return 0;
      }
    }
    /* calculate hash value (0 to 255) */
    h = (int) MYFLT_hash(&newval);
    n = ST(constTbl)[h];                        /* now search constpool */
    prv = 0;
    while (n) {
      if (csound->pool[n - 256] == newval) {    /* if val is there      */
        if (prv) {
          /* move to the beginning of the chain, so that */
          /* frequently searched values are found faster */
          ST(constTbl)[prv] = ST(constTbl)[n];
          ST(constTbl)[n] = ST(constTbl)[h];
          ST(constTbl)[h] = n;
        }
        return (n - 256);                       /*    return w. index   */
      }
      prv = n;
      n = ST(constTbl)[prv];
    }
    n = ST(poolcount)++;
    if (n >= ST(nconsts)) {
      ST(nconsts) = ((ST(nconsts) + (ST(nconsts) >> 3)) | (NCONSTS - 1)) + 1;
      if (UNLIKELY(csound->oparms->msglevel))
        csound->Message(csound, Str("extending Floating pool to %d\n"),
                                ST(nconsts));
      csound->pool = (MYFLT*) mrealloc(csound, csound->pool, ST(nconsts)
                                                             * sizeof(MYFLT));
      ST(constTbl) = (int*) mrealloc(csound, ST(constTbl), (256 + ST(nconsts))
                                                           * sizeof(int));
    }
    csound->pool[n] = newval;                   /* else enter newval    */
    ST(constTbl)[n + 256] = ST(constTbl)[h];    /*   link into chain    */
    ST(constTbl)[h] = n + 256;

    return n;                                   /*   and return new ndx */
}

/* tests whether variable name exists   */
/*      in gbl namelist                 */

static int gexist(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(csound, s);
    NAME          *p;

    for (p = ST(gblNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
    return (p == NULL ? 0 : 1);
}


/* builds namelist & type counts for gbl names */

static void gblnamset(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(csound, s);
    NAME          *p = ST(gblNames)[h];
                                                /* search gbl namelist: */
    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)                              /* if name is there     */
      return;                                   /*    return            */
    p = (NAME*) malloc(sizeof(NAME));
    if (UNLIKELY(p == NULL))
      csound->Die(csound, Str("gblnamset(): memory allocation failure"));
    p->namep = s;                               /* else record newname  */
    p->nxt = ST(gblNames)[h];
    ST(gblNames)[h] = p;
    if (*s == '#')  s++;
    if (*s == 'g')  s++;
    switch ((int) *s) {                         /*   and its type-count */
      case 'a': p->type = ATYPE; p->count = ST(gblnxtacnt)++; break;
      case 'S': p->type = STYPE; p->count = ST(gblnxtscnt)++; break;
      case 'f': p->type = PTYPE; p->count = ST(gblnxtpcnt)++; break;
      default:  p->type = KTYPE; p->count = ST(gblnxtkcnt)++;
    }
}

/* builds namelist & type counts for lcl names  */
/*  called by otran for each instr for lcl cnts */
/*  lists then redone by insprep via lcloffndx  */

static NAME *lclnamset(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(csound, s);
    NAME          *p = ST(lclNames)[h];
                                                /* search lcl namelist: */
    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)                              /* if name is there     */
      return p;                                 /*    return ptr        */
    p = (NAME*) malloc(sizeof(NAME));
    if (UNLIKELY(p == NULL))
      csound->Die(csound, Str("lclnamset(): memory allocation failure"));
    p->namep = s;                               /* else record newname  */
    p->nxt = ST(lclNames)[h];
    ST(lclNames)[h] = p;
    if (*s == '#')  s++;
    switch (*s) {                               /*   and its type-count */
      case 'w': p->type = WTYPE; p->count = ST(lclnxtwcnt)++; break;
      case 'a': p->type = ATYPE; p->count = ST(lclnxtacnt)++; break;
      case 'f': p->type = PTYPE; p->count = ST(lclnxtpcnt)++; break;
      case 't': p->type = PTYPE; p->count = ST(lclnxtpcnt)++; break;
      case 'S': p->type = STYPE; p->count = ST(lclnxtscnt)++; break;
      default:  p->type = KTYPE; p->count = ST(lclnxtkcnt)++; break;
    }
    return p;
}

/* get named offset index into gbl dspace     */
/* called only after otran and gblfixed valid */

static int gbloffndx(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(csound, s);
    NAME          *p = ST(gblNames)[h];

    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (UNLIKELY(p == NULL))
      csoundDie(csound, Str("unexpected global name"));
    switch (p->type) {
      case ATYPE: return (ST(gblfixed) + p->count);
      case STYPE: return (ST(gblfixed) + ST(gblacount) + p->count);
      case PTYPE: return (ST(gblkcount) + p->count * (int) Pfloats);
    }
    return p->count;
}

/* get named offset index into instr lcl dspace   */
/* called by insprep aftr lclcnts, lclfixed valid */

static int lcloffndx(CSOUND *csound, char *s)
{
    NAME    *np = lclnamset(csound, s);         /* rebuild the table    */

    switch (np->type) {                         /* use cnts to calc ndx */
      case KTYPE: return np->count;
      case WTYPE: return (ST(lclkcnt) + np->count * Wfloats);
      case ATYPE: return (ST(lclfixed) + np->count);
      case PTYPE: return (ST(lclkcnt) + ST(lclwcnt) * Wfloats
                                      + np->count * (int) Pfloats);
      case STYPE: return (ST(lclfixed) + ST(lclacnt) + np->count);
      default:    csoundDie(csound, Str("unknown nametype"));
    }
    return 0;
}

static void delete_global_namepool(CSOUND *csound)
{
    int i;

    if (csound->otranGlobals == NULL)
      return;
    for (i = 0; i < 256; i++) {
      while (ST(gblNames)[i] != NULL) {
        NAME  *nxt = ST(gblNames)[i]->nxt;
        free(ST(gblNames)[i]);
        ST(gblNames)[i] = nxt;
      }
    }
}

static void delete_local_namepool(CSOUND *csound)
{
    int i;

    if (csound->otranGlobals == NULL)
      return;
    for (i = 0; i < 256; i++) {
      while (ST(lclNames)[i] != NULL) {
        NAME  *nxt = ST(lclNames)[i]->nxt;
        free(ST(lclNames)[i]);
        ST(lclNames)[i] = nxt;
      }
    }
}

 /* ------------------------------------------------------------------------ */
#if 0

/* get size of string in MYFLT units */

static int strlen_to_samples(const char *s)
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

static int create_strconst_ndx_list(CSOUND *csound, int **lst, int offs)
{
    int     *ndx_lst;
    char    **strpool;
    int     strpool_cnt, ndx, i;

    strpool_cnt = ST(strpool_cnt);
    strpool = ST(strpool);
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

    strpool_cnt = ST(strpool_cnt);
    strpool = ST(strpool);
    if (strpool == NULL)
      return;
    for (ndx = i = 0; i < strpool_cnt; i++) {
      s = (char*) ((MYFLT*) dst + (int) ndx);
      unquote_string(s, strpool[i]);
      ndx += strlen_to_samples(strpool[i]);
    }
    /* original pool is no longer needed */
    ST(strpool) = NULL;
    ST(strpool_cnt) = 0;
    for (i = 0; i < strpool_cnt; i++)
      csound->Free(csound, strpool[i]);
    csound->Free(csound, strpool);
}
#endif

char argtyp2(CSOUND *csound, char *s)
{                   /* find arg type:  d, w, a, k, i, c, p, r, S, B, b, t */
    char c = *s;    /*   also set lgprevdef if !c && !p && !S */

    /* VL: added this to make sure the object exists before we try to read
       from it */
    if (UNLIKELY(csound->otranGlobals == NULL)) {
      csound->otranGlobals = csound->Calloc(csound, sizeof(OTRAN_GLOBALS));
    }
    /* csound->Message(csound, "\nArgtyp2: received %s\n", s); */

    /*trap this before parsing for a number! */
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
     ST(lgprevdef) = lgexist2(csound, s);         /* (lgprev) */
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
    if (strchr("akiBbfSt", c) != NULL)
      return(c);
    else return('?');
}

/* For diagnostics map file name or macro name to an index */
int file_to_int(CSOUND *csound, const char *name)
{
    extern char *strdup(const char *);
    int n = 0;
    char **filedir = csound->filedir;
    while (filedir[n] && n<63) {        /* Do we have it already? */
      if (strcmp(filedir[n], name)==0) return n; /* yes */
      n++;                                       /* look again */
    }
    // Not there so add
    // ensure long enough?
    if (n==63) {
      //csound->Die(csound, "Too many file/macros\n");
      filedir[n] = strdup("**unknown**");
    }
    else {
      filedir[n] = strdup(name);
      filedir[n+1] = NULL;
    }
    return n;
}
