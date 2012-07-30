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

//typedef struct otranStatics__ {
//    NAME      *gblNames[256], *lclNames[256];   /* for 8 bit hash */
//    ARGLST    *nullist;
//    ARGOFFS   *nulloffs;
//    int       lclkcnt, lclwcnt, lclfixed;
//    int       lclpcnt, lclscnt, lclacnt, lclnxtpcnt;
//    int       lclnxtkcnt, lclnxtwcnt, lclnxtacnt, lclnxtscnt;
//    int       gblnxtkcnt, gblnxtpcnt, gblnxtacnt, gblnxtscnt;
//    int       gblfixed, gblkcount, gblacount, gblscount;
//    int       *nxtargoffp, *argofflim, lclpmax;
//    char      **strpool;
//    int32      poolcount, strpool_cnt, argoffsize;
//    int       nconsts;
//    int       *constTbl;
//    int32     *typemask_tabl;
//    int32     *typemask_tabl_in, *typemask_tabl_out;
//} TRANS_DATA;

//static  int     gexist(CSOUND *, TRANS_DATA*, char *), gbloffndx(CSOUND *, TRANS_DATA*, char *);
//static  int     lcloffndx(CSOUND *, TRANS_DATA*, char *);
//static  int     constndx(CSOUND *, TRANS_DATA*, const char *);
//static  int     strconstndx(CSOUND *, TRANS_DATA*, const char *);
static ARG* createArg(CSOUND *csound, INSTRTXT* ip, char *s);
static  void    insprep(CSOUND *, INSTRTXT *);
static  void    lgbuild(CSOUND *, INSTRTXT *, char *, int inarg);
static  void    gblnamset(CSOUND *, char *);
//static  int     plgndx(CSOUND *, TRANS_DATA*, char *);
static  void    lclnamset(CSOUND *, INSTRTXT* ip, char *);
/*        int     lgexist(CSOUND *, const char *);*/
static  int     pnum(char *s) ;
static  int     lgexist2(CSOUND *csound, INSTRTXT*, const char *s);
static void     unquote_string(char *, const char *);

extern void     print_tree(CSOUND *, char *, TREE *);

void close_instrument(CSOUND *csound, INSTRTXT * ip);

char argtyp2(CSOUND *csound, char *s);

#define strsav_string(a) string_pool_save_string(csound, csound->stringSavePool, a);

//#define txtcpy(a,b) memcpy(a,b,sizeof(TEXT));

//#define ST(x)   (((OTRAN_GLOBALS*) ((CSOUND*) csound)->otranGlobals)->x)



//static const TRANS_DATA TRANS_DATA_TEMPLATE = {
//    {NULL}, {NULL}, /* gblNames, lclNames */
//    NULL, NULL,   /*  nullist, nulloffs   */
//    0, 0, 0,      /*  lclkcnt, lclwcnt, lclfixed */
//    0, 0, 0, 0,   /*  lclpcnt, lclscnt, lclacnt, lclnxtpcnt */
//    0, 0, 0, 0,   /*  lclnxtkcnt, lclnxtwcnt, lclnxtacnt, lclnxtscnt */
//    0, 0, 0, 0,   /*  gblnxtkcnt, gblnxtpcnt, gblnxtacnt, gblnxtscnt */
//    0, 0, 0, 0,   /*  gblfixed, gblkcount, gblacount, gblscount */
//    NULL, NULL, 0, /* nxtargoffp, argofflim, lclpmax */
//    NULL,         /*  strpool */
//    0, 0, 0,      /*  poolcount, strpool_cnt, argoffsize */
//    0, NULL,      /*  nconsts, constTbl */
//    NULL,         /*  typemask_tabl */
//    NULL, NULL,   /*  typemask_tabl_in, typemask_tabl_out */
//};
//
//static  void    delete_global_namepool(TRANS_DATA *);
//static  void    delete_local_namepool(TRANS_DATA *);


//#define STA(x)   (transData->x)

//#define KTYPE   1
//#define WTYPE   2
//#define ATYPE   3
//#define PTYPE   4
//#define STYPE   5
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

//TRANS_DATA* createTransData(CSOUND* csound) {
//    TRANS_DATA* data = csound->Malloc(csound, sizeof(TRANS_DATA));
//    memcpy(data, &TRANS_DATA_TEMPLATE, sizeof(TRANS_DATA));
//    return data;
//}

//void tranRESET(CSOUND *csound, TRANS_DATA* transData)
//{
//    void  *p;
//
//    delete_local_namepool(transData);
//    delete_global_namepool(transData);
//    p = (void*) csound->opcodlst;
//    csound->opcodlst = NULL;
//    csound->oplstend = NULL;
//    if (p != NULL)
//      free(p);
//}

//static void delete_global_namepool(TRANS_DATA* transData)
//{
//    int i;
//
//    for (i = 0; i < 256; i++) {
//      while (STA(gblNames)[i] != NULL) {
//        NAME  *nxt = STA(gblNames)[i]->nxt;
//        free(STA(gblNames)[i]);
//        STA(gblNames)[i] = nxt;
//      }
//    }
//}

 /* ------------------------------------------------------------------------ */

int argCount(ARG* arg) {
    int retVal = -1;
    if(arg != NULL) {
        retVal = 0;
        ARG* current = arg;
        while(current != NULL) {
            current = current->next;
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

//static int create_strconst_ndx_list(CSOUND *csound, TRANS_DATA* transData, int **lst, int offs)
//{
//    int     *ndx_lst;
//    char    **strpool;
//    int     strpool_cnt, ndx, i;
//
//    strpool_cnt = STA(strpool_cnt);
//    strpool = STA(strpool);
//    /* strpool_cnt always >= 1 because of empty string at index 0 */
//    ndx_lst = (int*) csound->Malloc(csound, strpool_cnt * sizeof(int));
//    for (i = 0, ndx = offs; i < strpool_cnt; i++) {
//      ndx_lst[i] = ndx;
//      ndx += strlen_to_samples(strpool[i]);
//    }
//    *lst = ndx_lst;
//    /* return with total size in MYFLT units */
//    return (ndx - offs);
//}
//
//static void convert_strconst_pool(CSOUND* csound, TRANS_DATA *transData, MYFLT *dst)
//{
//    char    **strpool, *s;
//    int     strpool_cnt, ndx, i;
//
//    strpool_cnt = STA(strpool_cnt);
//    strpool = STA(strpool);
//    if (strpool == NULL)
//      return;
//    for (ndx = i = 0; i < strpool_cnt; i++) {
//      s = (char*) ((MYFLT*) dst + (int) ndx);
//      unquote_string(s, strpool[i]);
//      ndx += strlen_to_samples(strpool[i]);
//    }
//    /* original pool is no longer needed */
//    STA(strpool) = NULL;
//    STA(strpool_cnt) = 0;
//    for (i = 0; i < strpool_cnt; i++)
//      csound->Free(csound, strpool[i]);
//    csound->Free(csound, strpool);
//}

//static void intyperr(CSOUND *csound, int n, char *s, char *opname,
//                     char tfound, char expect, int line)
//{
//    char    t[10];
//
//    switch (tfound) {
//    case 'w':
//    case 'f':
//    case 'a':
//    case 'k':
//    case 'i':
//    case 'P':
//    case 't':
//    case 'p': t[0] = tfound;
//      t[1] = '\0';
//      break;
//    case 'r':
//    case 'c': strcpy(t,"const");
//      break;
//    case 'S': strcpy(t,"string");
//      break;
//    case 'b':
//    case 'B': strcpy(t,"boolean");
//      break;
//    case '?': strcpy(t,"?");
//      break;
//  }
//    synterr(csound, Str("input arg %d '%s' of type %s not allowed when "
//                        "expecting %c (for opcode %s), line %d\n"),
//            n+1, s, t, expect, opname, line);
//}

static inline void resetouts(CSOUND *csound)
{
    csound->acount = csound->kcount = csound->icount =
      csound->Bcount = csound->bcount = 0;
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

//void set_xincod(CSOUND *csound, TEXT *tp, OENTRY *ep, int line)
//{
//    int n = tp->inlist->count;
//    char *s;
//    char *types = ep->intypes;
//    int nreqd = strlen(types);
//    int lgprevdef = 0;
//    char      tfound = '\0', treqd;
//
//    if (n > nreqd) {                 /* IV - Oct 24 2002: end of new code */
//      if ((treqd = types[nreqd-1]) == 'n') {  /* indef args: */
//        int incnt = -1;                       /* Should count args */
//        if (!(incnt & 01))                    /* require odd */
//          synterr(csound, Str("missing or extra arg"));
//      }       /* IV - Sep 1 2002: added 'M' */
//      else if (treqd != 'm' && treqd != 'z' && treqd != 'y' &&
//               treqd != 'Z' && treqd != 'M' && treqd != 'N') /* else any no */
//        synterr(csound, Str("too many input args\n"));
//    }
//
//    while (n--) {                     /* inargs:   */
//      int32    tfound_m, treqd_m = 0L;
//      s = tp->inlist->arg[n];
//
//      if (n >= nreqd) {               /* det type required */
//        switch (types[nreqd-1]) {
//        case 'M':
//        case 'N':
//        case 'Z':
//        case 'y':
//        case 'z':   treqd = types[nreqd-1]; break;
//        default:    treqd = 'i';    /*   (indef in-type) */
//        }
//      }
//      else treqd = types[n];          /*       or given)   */
//      if (treqd == 'l') {             /* if arg takes lbl  */
//        csound->DebugMsg(csound, "treqd = l");
//        //        lblrequest(csound, s);        /*      req a search */
//        continue;                     /*      chk it later */
//      }
//      tfound = argtyp2(csound, s);     /* else get arg type */
//      /* IV - Oct 31 2002 */
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
//      if (tfound == 'a' && n < 31) /* JMC added for FOG */
//                                   /* 4 for FOF, 8 for FOG; expanded to 15  */
//        tp->xincod |= (1 << n);
//      if (tfound == 'S' && n < 31)
//        tp->xincod_str |= (1 << n);
//      /* IV - Oct 31 2002: simplified code */
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
//                                                           tp->outlist->arg[0])];
//              if (outyp_m & (ARGTYP_a | ARGTYP_w | ARGTYP_f)) break;
//            }
//            else
//              break;
//          }
//        default:
//          intyperr(csound, n, s, ep->opname, tfound, treqd, line);
//          break;
//        }
//      }
//    }
//    //csound->DebugMsg(csound, "xincod = %d", tp->xincod);
//}
//
//void set_xoutcod(CSOUND *csound, TEXT *tp, OENTRY *ep, int line)
//{
//    int n = tp->outlist->count;
//    char *s;
//    char *types = ep->outypes;
//    int nreqd = -1;
//    char      tfound = '\0', treqd;
//
//    if (nreqd < 0)    /* for other opcodes */
//      nreqd = strlen(types = ep->outypes);
///* if ((n != nreqd) && */        /* IV - Oct 24 2002: end of new code */
///*          !(n > 0 && n < nreqd &&
//            (types[n] == (char) 'm' || types[n] == (char) 'z' ||
//             types[n] == (char) 'X' || types[n] == (char) 'N' ||
//             types[n] == (char) 'F' || types[n] == (char) 'I'))) {
//             synterr(csound, Str("illegal no of output args"));
//             if (n > nreqd)
//             n = nreqd;
//             }*/
//
//
//    while (n--) {                                     /* outargs:  */
//      long    tfound_m;       /* IV - Oct 31 2002 */
//      s = tp->outlist->arg[n];
//      treqd = types[n];
//      tfound = argtyp2(csound, s);                     /*  found    */
//      /* IV - Oct 31 2002 */
//      tfound_m = STA(typemask_tabl)[(unsigned char) tfound];
//      /* IV - Sep 1 2002: xoutcod is the same as xincod for input */
//      if (tfound == 'a' && n < 31)
//        tp->xoutcod |= (1 << n);
//      if (tfound == 'S' && n < 31)
//        tp->xoutcod_str |= (1 << n);
//      csound->DebugMsg(csound, "treqd %c, tfound %c", treqd, tfound);
//      /* if (tfound_m & ARGTYP_w) */
//      /*   if (STA(lgprevdef)) { */
//      /*     synterr(csound, Str("output name previously used, " */
//      /*                         "type '%c' must be uniquely defined, line %d"), */
//      /*             tfound, line); */
//      /*   } */
//      /* IV - Oct 31 2002: simplified code */
//      if (!(tfound_m & STA(typemask_tabl_out)[(unsigned char) treqd])) {
//        synterr(csound, Str("output arg '%s' illegal type (for opcode %s),"
//                            " line %d\n"),
//                s, ep->opname, line);
//      }
//    }
//}

/**
 * Create an Opcode (OPTXT) from the AST node given. Called from
 * create_instrument.
 */
OPTXT *create_opcode(CSOUND *csound, TREE *root, INSTRTXT *ip)
{
    TEXT *tp;
    TREE *inargs, *outargs;
    OPTXT *optxt, *retOptxt = NULL;
    char *arg;
    int opnum;
    int n, nreqd;;

    //printf("%d(%d): tree=%p\n", __FILE__, __LINE__, root);
    //print_tree(csound, "create_opcode", root);
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
      tp->opcod = strsav_string(csound->opcodlst[opnum].opname);
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

          tp->inlist->arg[argcount++] = strsav_string(arg);

          if ((n = pnum(arg)) >= 0) {
            if (n > ip->pmax)  ip->pmax = n;
          }
          /* VL 14/12/11 : calling lgbuild here seems to be problematic for
             undef arg checks */
          else {
            lgbuild(csound, ip, arg, 1);
          }


        }

        /* update_lclcount(csound, ip, root->right); */

      }
      /* update_lclcount(csound, ip, root->left); */


      /* VERIFY ARG LISTS MATCH OPCODE EXPECTED TYPES */

      {
        OENTRY *ep = csound->opcodlst + tp->opnum;
        int argcount = 0;
        //        csound->Message(csound, "Opcode InTypes: %s\n", ep->intypes);
        //        csound->Message(csound, "Opcode OutTypes: %s\n", ep->outypes);

        for (outargs = root->left; outargs != NULL; outargs = outargs->next) {
          arg = outargs->value->lexeme;
          tp->outlist->arg[argcount++] = strsav_string(arg);
        }
//        set_xincod(csound, tp, ep, root->line);
 
        /* OUTARGS */
        for (outargs = root->left; outargs != NULL; outargs = outargs->next) {

          arg = outargs->value->lexeme;

          if ((n = pnum(arg)) >= 0) {
            if (n > ip->pmax)  ip->pmax = n;
          }
          else {
            if (arg[0] == 'w' &&
                lgexist2(csound, ip, arg) != 0) {
              synterr(csound, Str("output name previously used, "
                                  "type 'w' must be uniquely defined, line %d"),
                      root->line);
            }
            lgbuild(csound, ip, arg, 0);
          }

        }
//        set_xoutcod(csound, tp, ep, root->line);

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
 * Create an Instrument (INSTRTXT) from the AST node given for use as
 * Instrument0. Called from csound_orc_compile.
 */
INSTRTXT *create_instrument0(CSOUND *csound, TREE *root)
{
    INSTRTXT *ip;
    OPTXT *op;

    TREE *current;

    ip = (INSTRTXT *) mcalloc(csound, sizeof(INSTRTXT));
    ip->varPool = (CS_VAR_POOL*)mcalloc(csound, sizeof(CS_VAR_POOL));
    op = (OPTXT *)ip;

    current = root;

    /* initialize */
//    ip->lclkcnt = 0;
//    ip->lclwcnt = 0;
//    ip->lclacnt = 0;
//    ip->lclpcnt = 0;
//    ip->lclscnt = 0;

//    delete_local_namepool(transData);
//    STA(lclnxtkcnt) = 0;                     /*   for rebuilding  */
//    STA(lclnxtwcnt) = STA(lclnxtacnt) = 0;
//    STA(lclnxtpcnt) = STA(lclnxtscnt) = 0;

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

      if (current->type != INSTR_TOKEN && current->type != UDO_TOKEN) {

        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "In INSTR 0: %s\n", current->value->lexeme);

        if (current->type == '='
           && strcmp(current->value->lexeme, "=.r") == 0) {

//          MYFLT val = csound->pool[constndx(csound,
//                                            current->right->value->lexeme)];
            
            //FIXME - perhaps should add check as it was in 
            //constndx?  Not sure if necessary due to assumption
            //that tree will be verified

            MYFLT val = (MYFLT) strtod(current->right->value->lexeme, NULL);

            myflt_pool_find_or_add(csound, csound->constantsPool, val);


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
    ip->varPool = (CS_VAR_POOL*)mcalloc(csound, sizeof(CS_VAR_POOL));    
    op = (OPTXT *)ip;
    statements = root->right;

//    ip->lclkcnt = 0;
//    ip->lclwcnt = 0;
//    ip->lclacnt = 0;
//    ip->lclpcnt = 0;
//    ip->lclscnt = 0;
//
//    delete_local_namepool(transData);
//    STA(lclnxtkcnt) = 0;                     /*   for rebuilding  */
//    STA(lclnxtwcnt) = STA(lclnxtacnt) = 0;
//    STA(lclnxtpcnt) = STA(lclnxtscnt) = 0;


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
      sprintf(c, "%ld", instrNum);

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
    bp->t.opcod = strsav_string("endin"); /*  term instr 0 blk */
    bp->t.outlist = bp->t.inlist = NULL;

    bp->nxtop = NULL;   /* terminate the optxt chain */

    current = (OPTXT *)ip;

    while (current->nxtop != NULL) {
        current = current->nxtop;
    }

    current->nxtop = bp;


//    ip->lclkcnt = STA(lclnxtkcnt);
//    /* align to 8 bytes for "spectral" types */
//    if ((int) sizeof(MYFLT) < 8 &&
//        (STA(lclnxtwcnt) + STA(lclnxtpcnt)) > 0)
//      ip->lclkcnt = (ip->lclkcnt + 1) & (~1);
//    ip->lclwcnt = STA(lclnxtwcnt);
//    ip->lclacnt = STA(lclnxtacnt);
//    ip->lclpcnt = STA(lclnxtpcnt);
//    ip->lclscnt = STA(lclnxtscnt);
//    ip->lclfixed = STA(lclnxtkcnt) + STA(lclnxtwcnt) * Wfloats
//                                  + STA(lclnxtpcnt) * Pfloats;

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
    ip->lclkcnt = STA(lclnxtkcnt); */
    /* align to 8 bytes for "spectral" types */

    /*if ((int) sizeof(MYFLT) < 8 &&
        (STA(lclnxtwcnt) + STA(lclnxtpcnt)) > 0)
      ip->lclkcnt = (ip->lclkcnt + 1) & (~1);
    ip->lclwcnt = STA(lclnxtwcnt);
    ip->lclacnt = STA(lclnxtacnt);
    ip->lclpcnt = STA(lclnxtpcnt);
    ip->lclscnt = STA(lclnxtscnt);
    ip->lclfixed = STA(lclnxtkcnt) + STA(lclnxtwcnt) * Wfloats
                                  + STA(lclnxtpcnt) * Pfloats;*/
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
 *
 * ASSUMES: TREE has been validated prior to compilation
 *
 */
PUBLIC int csoundCompileTree(CSOUND *csound, TREE *root)
{
//    csound->Message(csound, "Begin Compiling AST (Currently Implementing)\n");

    OPARMS      *O = csound->oparms;
    INSTRTXT    *instrtxt = NULL;
    INSTRTXT    *ip = NULL;
    INSTRTXT    *prvinstxt = &(csound->instxtanchor);
    OPTXT       *bp;
    char        *opname;
    int32        count, sumcount; //, instxtcount, optxtcount;
    TREE * current = root;
    INSTRTXT * instr0;
//    TRANS_DATA* transData = createTransData(csound);

//    strsav_create(csound);

    /* if (UNLIKELY(csound->otranGlobals == NULL)) { */
    /*   csound->otranGlobals = csound->Calloc(csound, sizeof(OTRAN_GLOBALS)); */
    /* } */
    csound->instrtxtp = (INSTRTXT **) mcalloc(csound, (1 + csound->maxinsno)
                                                      * sizeof(INSTRTXT*));
    // csound->opcodeInfo = NULL;          /* IV - Oct 20 2002 */

//    strconstndx(csound, "\"\"");
    string_pool_find_or_add(csound, csound->stringPool, "\"\"");

    CS_TYPE* rType = csoundGetTypeWithVarTypeName(csound->typePool, "r");
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "sr"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "kr"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "ksmps"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "nchnls"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "nchnls_i"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "0dbfs"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "$sr"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "$kr"));
    csoundAddVariable(csound->varPool, csoundCreateVariable(csound, csound->typePool, rType, "$ksmps"));

    //FIXME - check if this is setting a var or reserving a string in the global pools
//    gblnamset(csound, "sr");    /* enter global reserved words */
//    gblnamset(csound, "kr");
//    gblnamset(csound, "ksmps");
//    gblnamset(csound, "nchnls");
//    gblnamset(csound, "nchnls_i");
//    gblnamset(csound, "0dbfs"); /* no commandline override for that! */
//    gblnamset(csound, "$sr");   /* incl command-line overrides */
//    gblnamset(csound, "$kr");
//    gblnamset(csound, "$ksmps");

//    csound->pool = (MYFLT*) mmalloc(csound, NCONSTS * sizeof(MYFLT));
//    STA(poolcount) = 0;
//    STA(nconsts) = NCONSTS;
//    STA(constTbl) = (int*) mcalloc(csound, (256 + NCONSTS) * sizeof(int));
    myflt_pool_find_or_add(csound, csound->constantsPool, 0);
//    constndx(csound, "0");

//    if (!STA(typemask_tabl)) {
//      const int32 *ptr = typetabl1;
//      STA(typemask_tabl) = (int32*) mcalloc(csound, sizeof(int32) * 256);
//      STA(typemask_tabl_in) = (int32*) mcalloc(csound, sizeof(int32) * 256);
//      STA(typemask_tabl_out) = (int32*) mcalloc(csound, sizeof(int32) * 256);
//      while (*ptr) {            /* basic types (both for input */
//        int32 pos = *ptr++;      /* and output) */
//        STA(typemask_tabl)[pos] = STA(typemask_tabl_in)[pos] =
//          STA(typemask_tabl_out)[pos] = *ptr++;
//      }
//      ptr = typetabl2;
//      while (*ptr) {            /* input types */
//        int32 pos = *ptr++;
//        STA(typemask_tabl_in)[pos] = *ptr++;
//      }
//      ptr = typetabl3;
//      while (*ptr) {            /* output types */
//        int32 pos = *ptr++;
//        STA(typemask_tabl_out)[pos] = *ptr++;
//      }
//    }
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
//    if (UNLIKELY(O->odebug)) {
//      int32  n;
//      MYFLT *p;
//      csound->Message(csound, "poolcount = %ld, strpool_cnt = %ld\n",
//                              STA(poolcount), STA(strpool_cnt));
//      csound->Message(csound, "pool:");
//      for (n = STA(poolcount), p = csound->pool; n--; p++)
//        csound->Message(csound, "\t%g", *p);
//      csound->Message(csound, "\n");
//      csound->Message(csound, "strpool:");
//      for (n = 0L; n < STA(strpool_cnt); n++)
//        csound->Message(csound, "\t%s", STA(strpool)[n]);
//      csound->Message(csound, "\n");
//    }
//    STA(gblfixed) = STA(gblnxtkcnt) + STA(gblnxtpcnt) * (int) Pfloats;
//    STA(gblkcount) = STA(gblnxtkcnt);
    //FIXME - consider issue of alignment
//    /* align to 8 bytes for "spectral" types */
//    if ((int) sizeof(MYFLT) < 8 && STA(gblnxtpcnt))
//      STA(gblkcount) = (STA(gblkcount) + 1) & (~1);
//    STA(gblacount) = STA(gblnxtacnt);
//    STA(gblscount) = STA(gblnxtscnt);

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
//    STA(argoffsize) = (sumcount + 1) * sizeof(int);  /* alloc all plus 1 null */
//    /* as argoff ints */
//    csound->argoffspace = (int*) mmalloc(csound, STA(argoffsize));
//    STA(nxtargoffp) = csound->argoffspace;
//    STA(nulloffs) = (ARGOFFS *) csound->argoffspace; /* setup the null argoff */
//    *STA(nxtargoffp)++ = 0;
//    STA(argofflim) = STA(nxtargoffp) + sumcount;
    ip = &(csound->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL)        /* add all other entries */
      insprep(csound, ip);                      /*   as combined offsets */
//    if (UNLIKELY(O->odebug)) {
//      int *p = csound->argoffspace;
//      csound->Message(csound, "argoff array:\n");
//      do {
//        csound->Message(csound, "\t%d", *p++);
//      } while (p < STA(argofflim));
//      csound->Message(csound, "\n");
//    }
//    if (UNLIKELY(STA(nxtargoffp) != STA(argofflim)))
//      csoundDie(csound, Str("inconsistent argoff sumcount"));
//
    ip = &(csound->instxtanchor);               /* set the OPARMS values */
    //instxtcount = optxtcount = 0;
//    while ((ip = ip->nxtinstxt) != NULL) {
//      instxtcount += 1;
//      optxtcount += ip->optxtcount;
//    }
    //    csound->instxtcount = instxtcount;
//    csound->optxtsize = instxtcount * sizeof(INSTRTXT)
//                        + optxtcount * sizeof(OPTXT);
//    csound->poolcount = STA(poolcount);
//    csound->gblfixed = STA(gblnxtkcnt) + STA(gblnxtpcnt) * (int) Pfloats;
//    csound->gblacount = STA(gblnxtacnt);
//    csound->gblscount = STA(gblnxtscnt);
    /* clean up */
//    delete_local_namepool(transData);
//    delete_global_namepool(transData);
//    mfree(csound, STA(constTbl));
//    STA(constTbl) = NULL;
//    mfree(csound, transData);
    /* End code from otran */

    /* csound->Message(csound, "End Compiling AST\n"); */
    return CSOUND_SUCCESS;
}

void debugPrintCsound(CSOUND* csound) {
    csound->Message(csound, "Compile State:\n");
    csound->Message(csound, "String Pool:\n");
    STRING_VAL* val = csound->stringPool->values;
    int count = 0;
    while(val != NULL) {
        csound->Message(csound, "    %d) %s\n", count++, val->value);
        val = val->next;
    }
    csound->Message(csound, "Constants Pool:\n");    
    count = 0;
    for(count = 0; count < csound->constantsPool->count; count++) {
        csound->Message(csound, "    %d) %f\n", count, csound->constantsPool->values[count]);
    }
    
    csound->Message(csound, "Global Variables:\n");    
    CS_VARIABLE* gVar = csound->varPool->head;
    count = 0;
    while(gVar != NULL) {
        csound->Message(csound, "  %d) %s:%s\n", count++, 
                        gVar->varName, gVar->varType->varTypeName);
        gVar = gVar->next;
    }
    
    
    INSTRTXT    *current = &(csound->instxtanchor);
    current = current->nxtinstxt;
    count = 0;
    while (current != NULL) {
        csound->Message(csound, "Instrument %d\n", count);
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

PUBLIC int csoundCompileOrc(CSOUND *csound, char *str)
{
    TREE *root = csoundParseOrc(csound, str);
    int retVal = csoundCompileTree(csound, root);
    
//    if(csound->oparms->odebug) {
    
    debugPrintCsound(csound);
            
//    }
    
    return retVal;
}

/* prep an instr template for efficient allocs  */
/* repl arg refs by offset ndx to lcl/gbl space */
static void insprep(CSOUND *csound, INSTRTXT *tp)
{
    OPARMS      *O = csound->oparms;
    OPTXT       *optxt;
    OENTRY      *ep;
    int         opnum;
//    int         n, opnum, inreqd;
    char        **argp;
//    char        **labels, **lblsp;
//    LBLARG      *larg, *largp;

    int n, inreqd;
    ARGLST      *outlist, *inlist;
//    ARGOFFS     *outoffs, *inoffs;
    int         indx;
    int *ndxp;

//    labels = (char **)mmalloc(csound, (csound->nlabels) * sizeof(char *));
//    lblsp = labels;
//    larg = (LBLARG *)mmalloc(csound, (csound->ngotos) * sizeof(LBLARG));
//    largp = larg;
//    STA(lclkcnt) = tp->lclkcnt;
//    STA(lclwcnt) = tp->lclwcnt;
//    STA(lclfixed) = tp->lclfixed;
//    STA(lclpcnt) = tp->lclpcnt;
//    STA(lclscnt) = tp->lclscnt;
//    STA(lclacnt) = tp->lclacnt;
//    delete_local_namepool(transData);              /* clear lcl namlist */
//    STA(lclnxtkcnt) = 0;                         /*   for rebuilding  */
//    STA(lclnxtwcnt) = STA(lclnxtacnt) = 0;
//    STA(lclnxtpcnt) = STA(lclnxtscnt) = 0;
//    STA(lclpmax) = tp->pmax;                     /* set pmax for plgndx */
//    ndxp = STA(nxtargoffp);
    optxt = (OPTXT *)tp;
    while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
      TEXT *ttp = &optxt->t;
      if ((opnum = ttp->opnum) == ENDIN         /*  (until ENDIN)  */
          || opnum == ENDOP)            /* (IV - Oct 31 2002: or ENDOP) */
        break;
      if (opnum == LABEL) {
//        if (lblsp - labels >= csound->nlabels) {
//          int oldn = lblsp - labels;
//          csound->nlabels += NLABELS;
//          if (lblsp - labels >= csound->nlabels)
//            csound->nlabels = lblsp - labels + 2;
//          if (csound->oparms->msglevel)
//            csound->Message(csound,
//                            Str("LABELS list is full...extending to %d\n"),
//                            csound->nlabels);
//          labels =
//            (char**)mrealloc(csound, labels, csound->nlabels*sizeof(char*));
//          lblsp = &labels[oldn];
//        }
        //FIXME - label handling
//        *lblsp++ = ttp->opcod;
          csound->Message(csound, "Fixme: insprep label: %s\n", ttp->opcod);
        continue;
      }
      ep = &(csound->opcodlst[opnum]);
      if (O->odebug) csound->Message(csound, "%s args:", ep->opname);
      if ((outlist = ttp->outlist) == NULL || !outlist->count)
        ttp->outArgs = NULL;
      else {
//        ttp->outoffs = outoffs = (ARGOFFS *) ndxp;
//        outoffs->count =
          n = outlist->count;
        argp = outlist->arg;                    /* get outarg indices */
//        ndxp = outoffs->indx;
        while (n--) {
          //*ndxp++ = indx = plgndx(csound, *argp++);
            ARG* arg = createArg(csound, tp, *argp++);
            
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
            
            //FIXME - print arg
          //if (O->odebug) csound->Message(csound, "\t%d", indx);
        }
        ttp->outArgCount = argCount(ttp->outArgs);
      }
      if ((inlist = ttp->inlist) == NULL || !inlist->count)
        ttp->inArgs = NULL;
      else {
//        ttp->inoffs = inoffs = (ARGOFFS *) ndxp;
//        inoffs->count = inlist->count;
        inreqd = strlen(ep->intypes);
        argp = inlist->arg;                     /* get inarg indices */
//        ndxp = inoffs->indx;
        for (n=0; n < inlist->count; n++, argp++) {
          if (n < inreqd && ep->intypes[n] == 'l') {
//            if (largp - larg >= csound->ngotos) {
//              int oldn = csound->ngotos;
//              csound->ngotos += NGOTOS;
//              if (csound->oparms->msglevel)
//                csound->Message(csound,
//                                Str("GOTOS list is full..extending to %d\n"),
//                                csound->ngotos);
//              if (largp - larg >= csound->ngotos)
//                csound->ngotos = largp - larg + 1;
//              larg = (LBLARG *)
//                mrealloc(csound, larg, csound->ngotos * sizeof(LBLARG));
//              largp = &larg[oldn];
//            }
            if (UNLIKELY(O->odebug))
              csound->Message(csound, "\t***lbl");  /* if arg is label,  */
//            largp->lbltxt = *argp;
//            largp->ndxp = ndxp;                     /*  defer till later */
//            largp++;
            csound->Message(csound, "Fixme: in-arg label: %s\n", *argp);
          }
          else {
              char *s = *argp;
              ARG* arg = createArg(csound, tp, s);
              
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
//            indx = plgndx(csound, transData, s);
//            if (UNLIKELY(O->odebug)) csound->Message(csound, "\t%d", indx);
//            *ndxp = indx;
          }
          ttp->inArgCount = argCount(ttp->inArgs);
        }
      }
//      if (UNLIKELY(O->odebug)) csound->Message(csound, "\n");
    }
 nxt:
        /****/
    csound->Message(csound, "Fixme: insprep label handling\n");
//    while (--largp >= larg) {                   /* resolve the lbl refs */
//      char *s = largp->lbltxt;
//      char **lp;
//      for (lp = labels; lp < lblsp; lp++)
//        if (strcmp(s, *lp) == 0) {
//          *largp->ndxp = lp - labels + LABELOFS;
//          goto nxt;
//        }
//      csoundDie(csound, Str("target label '%s' not found"), s);
//    }
    /****/
//    STA(nxtargoffp) = ndxp;
//    mfree(csound, labels);
//    mfree(csound, larg);
}

/* returns non-zero if 's' is defined in the global or local pool of names */

static int lgexist2(CSOUND* csound, INSTRTXT* ip, const char* s) {
    int retVal = 0;
    
    if(csoundFindVariableWithName(csound->varPool, s) != NULL) {
        retVal = 1;
    } else if(csoundFindVariableWithName(ip->varPool, s) != NULL) {
        retVal = 1;
    }
    
    return retVal;
}

//static int lgexist2(CSOUND *csound, TRANS_DATA* transData, const char *s)
//{
//    unsigned char h = name_hash(csound, s);
//    NAME          *p = NULL;
//    for (p = STA(gblNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
//    if (p != NULL)
//      return 1;
//    for (p = STA(lclNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
//    return (p == NULL ? 0 : 1);
//}

/* build pool of floating const values  */
/* build lcl/gbl list of ds names, offsets */
/* (no need to save the returned values) */
static void lgbuild(CSOUND *csound, INSTRTXT* ip, char *s, int inarg)
{
    char    c;

    c = *s;
    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0))
        myflt_pool_find_or_addc(csound, csound->constantsPool, s);
    else if (c == '"')
        // FIXME need to unquote_string here
        //unquote_string
        string_pool_find_or_add(csound, csound->stringPool, s);
    else if (!lgexist2(csound, ip, s) && !inarg) {
      if (c == 'g' || (c == '#' && s[1] == 'g'))
        gblnamset(csound, s);
      else
        lclnamset(csound, ip, s);
    }
}

/* get storage ndx of const, pnum, lcl or gbl */
/* argument const/gbl indexes are positiv+1, */
/* pnum/lcl negativ-1 called only after      */
/* poolcount & lclpmax are finalised */
static ARG* createArg(CSOUND *csound, INSTRTXT* ip, char *s)
{
    char        c;
    int         n;

    c = *s;
    
    ARG* arg = csound->Calloc(csound, sizeof(ARG));
    
    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0)) {
        arg->type = ARG_CONSTANT;
        arg->index = myflt_pool_find_or_addc(csound, csound->constantsPool, s);
    } else if (c == '"') {
        arg->type = ARG_STRING;
        arg->index = string_pool_find_or_add(csound, csound->stringPool, s);
    } else if ((n = pnum(s)) >= 0) {
        arg->type = ARG_PFIELD;
        arg->index = n;
    } else if (c == 'g' || (c == '#' && *(s+1) == 'g') ||
          csoundFindVariableWithName(csound->varPool, s) != NULL) {
        // FIXME - figure out why string pool searched with gexist
    //|| string_pool_indexof(csound->stringPool, s) > 0) {
        arg->type = ARG_GLOBAL;
        arg->argPtr = csoundFindVariableWithName(csound->varPool, s);
    } else {
        arg->type = ARG_LOCAL;
        arg->argPtr = csoundFindVariableWithName(ip->varPool, s);
        
        if(arg->argPtr == NULL) {
            csound->Message(csound, "Missing local arg: %s\n", s);
        }
    }
/*    csound->Message(csound, " [%s -> %d (%x)]\n", s, indx, indx); */
    return arg;
}

/* get storage ndx of string const value */
/* builds value pool on 1st occurrence   */
//static int strconstndx(CSOUND *csound, TRANS_DATA* transData, const char *s)
//{
//    int     i, cnt;
//
//    /* check syntax */
//    cnt = (int) strlen(s);
//    if (UNLIKELY(cnt < 2 || *s != '"' || s[cnt - 1] != '"')) {
//      synterr(csound, Str("string syntax '%s'"), s);
//      return 0;
//    }
//    /* check if a copy of the string is already stored */
//    for (i = 0; i < STA(strpool_cnt); i++) {
//      if (strcmp(s, STA(strpool)[i]) == 0)
//        return i;
//    }
//    /* not found, store new string */
//    cnt = STA(strpool_cnt)++;
//    if (!(cnt & 0x7F)) {
//      /* extend list */
//      if (!cnt) STA(strpool) = csound->Malloc(csound, 0x80 * sizeof(MYFLT*));
//      else      STA(strpool) = csound->ReAlloc(csound, STA(strpool),
//                                              (cnt + 0x80) * sizeof(MYFLT*));
//    }
//    STA(strpool)[cnt] = (char*) csound->Malloc(csound, strlen(s) + 1);
//    strcpy(STA(strpool)[cnt], s);
//    /* and return index */
//    return cnt;
//}

//static inline unsigned int MYFLT_hash(const MYFLT *x)
//{
//    const unsigned char *c = (const unsigned char*) x;
//    size_t              i;
//    unsigned int        h = 0U;
//
//    for (i = (size_t) 0; i < sizeof(MYFLT); i++)
//      h = (unsigned int) strhash_tabl_8[(unsigned int) c[i] ^ h];
//
//    return h;
//}

/* get storage ndx of float const value */
/* builds value pool on 1st occurrence  */
/* final poolcount used in plgndx above */
/* pool may be moved w. ndx still valid */

//static int constndx(CSOUND *csound, TRANS_DATA* transData, const char *s)
//{
//    MYFLT   newval;
//    int     h, n, prv;
//
//    {
//      volatile MYFLT  tmpVal;   /* make sure it really gets rounded to MYFLT */
//      char            *tmp = (char*) s;
//      tmpVal = (MYFLT) strtod(s, &tmp);
//      newval = tmpVal;
//      if (UNLIKELY(tmp == s || *tmp != (char) 0)) {
//        synterr(csound, Str("numeric syntax '%s'"), s);
//        return 0;
//      }
//    }
//    /* calculate hash value (0 to 255) */
//    h = (int) MYFLT_hash(&newval);
//    n = STA(constTbl)[h];                        /* now search constpool */
//    prv = 0;
//    while (n) {
//      if (csound->pool[n - 256] == newval) {    /* if val is there      */
//        if (prv) {
//          /* move to the beginning of the chain, so that */
//          /* frequently searched values are found faster */
//          STA(constTbl)[prv] = STA(constTbl)[n];
//          STA(constTbl)[n] = STA(constTbl)[h];
//          STA(constTbl)[h] = n;
//        }
//        return (n - 256);                       /*    return w. index   */
//      }
//      prv = n;
//      n = STA(constTbl)[prv];
//    }
//    n = STA(poolcount)++;
//    if (n >= STA(nconsts)) {
//      STA(nconsts) = ((STA(nconsts) + (STA(nconsts) >> 3)) | (NCONSTS - 1)) + 1;
//      if (UNLIKELY(csound->oparms->msglevel))
//        csound->Message(csound, Str("extending Floating pool to %d\n"),
//                                STA(nconsts));
//      csound->pool = (MYFLT*) mrealloc(csound, csound->pool, STA(nconsts)
//                                                             * sizeof(MYFLT));
//      STA(constTbl) = (int*) mrealloc(csound, STA(constTbl), (256 + STA(nconsts))
//                                                           * sizeof(int));
//    }
//    csound->pool[n] = newval;                   /* else enter newval    */
//    STA(constTbl)[n + 256] = STA(constTbl)[h];    /*   link into chain    */
//    STA(constTbl)[h] = n + 256;
//
//    return n;                                   /*   and return new ndx */
//}

/* tests whether variable name exists   */
/*      in gbl namelist                 */

//static int gexist(CSOUND *csound, TRANS_DATA* transData, char *s)
//{
//    unsigned char h = name_hash(csound, s);
//    NAME          *p;
//
//    for (p = STA(gblNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
//    return (p == NULL ? 0 : 1);
//}


/* builds namelist & type counts for gbl names */

static void gblnamset(CSOUND *csound, char *s)
{
    CS_TYPE* type;
    char* argLetter;
    CS_VARIABLE* var;
    char* t = s;
    
    var = csoundFindVariableWithName(csound->varPool, s);
    
    if (var != NULL) {
        return;
    }
    
    argLetter = csound->Malloc(csound, 2 * sizeof(char));
    
    if (*t == '#')  t++;
    if (*t == 'g')  t++;
    
    argLetter[0] = *t;
    argLetter[1] = 0;

    type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    var = csoundCreateVariable(csound, csound->typePool, type, s);
    csoundAddVariable(csound->varPool, var);
}

static void lclnamset(CSOUND *csound, INSTRTXT* ip, char *s)
{
    CS_TYPE* type;
    char* argLetter;
    CS_VARIABLE* var;
    char* t = s;
    
    var = csoundFindVariableWithName(ip->varPool, s);
    
    if (var != NULL) {
        return;
    }
    
    argLetter = csound->Malloc(csound, 2 * sizeof(char));
    
    if (*t == '#')  t++;
    
    argLetter[0] = *t;
    argLetter[1] = 0;
    
    type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    var = csoundCreateVariable(csound, csound->typePool, type, s);
    csoundAddVariable(ip->varPool, var);
}

/* get named offset index into gbl dspace     */
/* called only after otran and gblfixed valid */

//static int gbloffndx(CSOUND *csound, TRANS_DATA* transData, char *s)
//{
//    unsigned char h = name_hash(csound, s);
//    NAME          *p = STA(gblNames)[h];
//
//    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
//    if (UNLIKELY(p == NULL))
//      csoundDie(csound, Str("unexpected global name"));
//    switch (p->type) {
//      case ATYPE: return (STA(gblfixed) + p->count);
//      case STYPE: return (STA(gblfixed) + STA(gblacount) + p->count);
//      case PTYPE: return (STA(gblkcount) + p->count * (int) Pfloats);
//    }
//    return p->count;
//}

/* get named offset index into instr lcl dspace   */
/* called by insprep aftr lclcnts, lclfixed valid */

//static int lcloffndx(CSOUND *csound, TRANS_DATA* transData, char *s)
//{
//    NAME    *np = lclnamset(csound, transData, s);         /* rebuild the table    */
//
//    switch (np->type) {                         /* use cnts to calc ndx */
//      case KTYPE: return np->count;
//      case WTYPE: return (STA(lclkcnt) + np->count * Wfloats);
//      case ATYPE: return (STA(lclfixed) + np->count);
//      case PTYPE: return (STA(lclkcnt) + STA(lclwcnt) * Wfloats
//                                      + np->count * (int) Pfloats);
//      case STYPE: return (STA(lclfixed) + STA(lclacnt) + np->count);
//      default:    csoundDie(csound, Str("unknown nametype"));
//    }
//    return 0;
//}
//
//static void delete_local_namepool(TRANS_DATA* transData)
//{
//    int i;
//
//    for (i = 0; i < 256; i++) {
//      while (STA(lclNames)[i] != NULL) {
//        NAME  *nxt = STA(lclNames)[i]->nxt;
//        free(STA(lclNames)[i]);
//        STA(lclNames)[i] = nxt;
//      }
//    }
//}

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

/* static void unquote_string(char *dst, const char *src) */
/* { */
/*     int i, j, n = (int) strlen(src) - 1; */
/*     for (i = 1, j = 0; i < n; i++) { */
/*       if (src[i] != '\\') */
/*         dst[j++] = src[i]; */
/*       else { */
/*         switch (src[++i]) { */
/*         case 'a':   dst[j++] = '\a';  break; */
/*         case 'b':   dst[j++] = '\b';  break; */
/*         case 'f':   dst[j++] = '\f';  break; */
/*         case 'n':   dst[j++] = '\n';  break; */
/*         case 'r':   dst[j++] = '\r';  break; */
/*         case 't':   dst[j++] = '\t';  break; */
/*         case 'v':   dst[j++] = '\v';  break; */
/*         case '"':   dst[j++] = '"';   break; */
/*         case '\\':  dst[j++] = '\\';  break; */
/*         default: */
/*           if (src[i] >= '0' && src[i] <= '7') { */
/*             int k = 0, l = (int) src[i] - '0'; */
/*             while (++k < 3 && src[i + 1] >= '0' && src[i + 1] <= '7') */
/*               l = (l << 3) | ((int) src[++i] - '0'); */
/*             dst[j++] = (char) l; */
/*           } */
/*           else { */
/*             dst[j++] = '\\'; i--; */
/*           } */
/*         } */
/*       } */
/*     } */
/*     dst[j] = '\0'; */
/* } */

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

char argtyp2(CSOUND *csound, char *s)
{                   /* find arg type:  d, w, a, k, i, c, p, r, S, B, b, t */
    char c = *s;    /*   also set lgprevdef if !c && !p && !S */

    /* VL: added this to make sure the object exists before we try to read
       from it */
    /* if (UNLIKELY(csound->otranGlobals == NULL)) { */
    /*   csound->otranGlobals = csound->Calloc(csound, sizeof(OTRAN_GLOBALS)); */
    /* } */
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

void oload(CSOUND *p)
{
//    int32    n, combinedsize, insno, *lp;
    int32    n, insno, *lp;
//    int32    gblabeg, gblsbeg, gblsbas, gblscbeg, lclabeg, lclsbeg, lclsbas;
//    MYFLT   *combinedspc, *gblspace, *fp1;
    MYFLT    *fp1;
    INSTRTXT *ip;
    OPTXT   *optxt;
    OPARMS  *O = p->oparms;
    int     *strConstIndexList;
    MYFLT   ensmps;

    p->esr = p->tran_sr; p->ekr = p->tran_kr;
    p->nchnls = p->tran_nchnls;
    p->e0dbfs = p->tran_0dbfs;
    p->ksmps = (int) ((ensmps = p->tran_ksmps) + FL(0.5));
    ip = p->instxtanchor.nxtinstxt;        /* for instr 0 optxts:  */

    /* why I want oload() to return an error value.... */
    if (UNLIKELY(p->e0dbfs <= FL(0.0)))
      p->Die(p, Str("bad value for 0dbfs: must be positive."));
    if (UNLIKELY(O->odebug))
      p->Message(p, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                    "0dbfs = %.1f\n",
                    p->esr, p->ekr, p->ksmps, p->nchnls, p->e0dbfs);
    if (O->sr_override) {        /* if command-line overrides, apply now */
      p->esr = (MYFLT) O->sr_override;
      p->ekr = (MYFLT) O->kr_override;
      p->ksmps = (int) ((ensmps = ((MYFLT) O->sr_override
                                   / (MYFLT) O->kr_override)) + FL(0.5));
      p->Message(p, Str("sample rate overrides: "
                        "esr = %7.4f, ekr = %7.4f, ksmps = %d\n"),
                    p->esr, p->ekr, p->ksmps);
    }
    /* number of MYFLT locations to allocate for a string variable */
//    p->strVarSamples = (p->strVarMaxLen + (int) sizeof(MYFLT) - 1)
//                       / (int) sizeof(MYFLT);
//    p->strVarMaxLen = p->strVarSamples * (int) sizeof(MYFLT);
    /* calculate total size of global pool */
//    combinedsize = p->poolcount                 /* floating point constants */
//                   + p->gblfixed                /* k-rate / spectral        */
//                   + p->gblacount * p->ksmps            /* a-rate variables */
//                   + p->gblscount * p->strVarSamples;   /* string variables */
//    gblscbeg = combinedsize + 1;                /* string constants         */

    // FIXME
    //    combinedsize += create_strconst_ndx_list(p, transData, &strConstIndexList, gblscbeg);

//    combinedspc = (MYFLT*) mcalloc(p, combinedsize * sizeof(MYFLT));
    /* copy pool into combined space */
//    memcpy(combinedspc, p->pool, p->poolcount * sizeof(MYFLT));
//    mfree(p, (void*) p->pool);
//    p->pool = combinedspc;
    //gblspace = p->pool + p->poolcount;
    
    // create memblock for global variables
    p->globalVarPool = mcalloc(p, p->varPool->poolSize);

    MYFLT* globals = p->globalVarPool;
    
    globals[0] = p->esr;           /*   & enter        */
    globals[1] = p->ekr;           /*   rsvd word      */
    globals[2] = (MYFLT) p->ksmps; /*   curr vals      */
    globals[3] = (MYFLT) p->nchnls;
    if (p->inchnls<0) p->inchnls = p->nchnls;
    globals[4] = (MYFLT) p->inchnls;
    globals[5] = p->e0dbfs;
    
//    p->constantsPool->count = 6;
    
//    p->gbloffbas = p->pool - 1;
    /* string constants: unquote, convert escape sequences, and copy to pool */
// FIXME
//    convert_strconst_pool(p, transData, (MYFLT*) p->gbloffbas + (int32) gblscbeg);

//    gblabeg = p->poolcount + p->gblfixed + 1;
//    gblsbeg = gblabeg + p->gblacount;
//    gblsbas = gblabeg + (p->gblacount * p->ksmps);
    ip = &(p->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL) {      /* EXPAND NDX for A & S Cells */
      optxt = (OPTXT *) ip;                     /*   (and set localen)        */
//      lclabeg = (int32) (ip->pmax + ip->lclfixed + 1);
//      lclsbeg = (int32) (lclabeg + ip->lclacnt);
//      lclsbas = (int32) (lclabeg + (ip->lclacnt * (int32) p->ksmps));
//      if (UNLIKELY(O->odebug)) p->Message(p, "lclabeg %d, lclsbeg %d\n",
//                                   lclabeg, lclsbeg);
//      ip->localen = ((int32) ip->lclfixed
//                     + (int32) ip->lclacnt * (int32) p->ksmps
//                     + (int32) ip->lclscnt * (int32) p->strVarSamples)
//                    * (int32) sizeof(MYFLT);
        //FIXME - note alignment
      /* align to 64 bits */
     // ip->localen = (ip->localen + 7L) & (~7L);
      for (insno = 0, n = 0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  n++;            /* count insnos  */
      lp = ip->inslist = (int32 *) mmalloc(p, (int32)(n+1) * sizeof(int32));
      for (insno=0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  *lp++ = insno;  /* creat inslist */
      *lp = -1;                                         /*   & terminate */
      insno = *ip->inslist;                             /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT    *ttp = &optxt->t;
//        ARGOFFS *aoffp;
        int32    indx;
        int32    posndx;
        int     *ndxp;
        ARG*    arg;
        int     opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;    /* IV - Sep 8 2002 */
        if (opnum == LABEL) continue;
//        aoffp = ttp->outoffs;           /* ------- OUTARGS -------- */
        n = argCount(ttp->outArgs);
        arg = ttp->outArgs;
//        while(arg != NULL) {
//            
//	    }
//          for (ndxp = aoffp->indx; n--; ndxp++) {
//          indx = *ndxp;
//          if (indx > 0) {               /* positive index: global   */
//            if (UNLIKELY(indx >= STR_OFS))        /* string constant          */
//              p->Die(p, Str("internal error: string constant outarg"));
//            if (indx > gblsbeg)         /* global string variable   */
//              indx = gblsbas + (indx - gblsbeg) * p->strVarSamples;
//            else if (indx > gblabeg)    /* global a-rate variable   */
//              indx = gblabeg + (indx - gblabeg) * p->ksmps;
//            else if (indx <= 3 && O->sr_override &&
//                     ip == p->instxtanchor.nxtinstxt)   /* for instr 0 */
//              indx += 3;        /* deflect any old sr,kr,ksmps targets */
//          }
//          else {                        /* negative index: local    */
//            posndx = -indx;
//            if (indx < LABELIM)         /* label                    */
//              continue;
//            if (posndx > lclsbeg)       /* local string variable    */
//              indx = -(lclsbas + (posndx - lclsbeg) * p->strVarSamples);
//            else if (posndx > lclabeg)  /* local a-rate variable    */
//              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
//          }
//          *ndxp = (int) indx;
//        }
//        aoffp = ttp->inoffs;            /* inargs:                  */
//        if (opnum >= SETEND) goto realops;
//        switch (opnum) {                /*      do oload SETs NOW   */
//        case PSET:
//          p->Message(p, "PSET: isno=%d, pmax=%d\n", insno, ip->pmax);
//          if ((n = aoffp->count) != ip->pmax) {
//            p->Warning(p, Str("i%d pset args != pmax"), (int) insno);
//            if (n < ip->pmax) n = ip->pmax; /* cf pset, pmax    */
//          }                                 /* alloc the larger */
//          ip->psetdata = (MYFLT *) mcalloc(p, n * sizeof(MYFLT));
//          for (n = aoffp->count, fp1 = ip->psetdata, ndxp = aoffp->indx;
//               n--; ) {
//            *fp1++ = p->gbloffbas[*ndxp++];
//            p->Message(p, "..%f..", *(fp1-1));
//          }
//          p->Message(p, "\n");
//          break;
//        }
//        continue;       /* no runtime role for the above SET types */
//
//      realops:
//        n = aoffp->count;               /* -------- INARGS -------- */
//        for (ndxp = aoffp->indx; n--; ndxp++) {
//          indx = *ndxp;
//          if (indx > 0) {               /* positive index: global   */
//            if (indx >= STR_OFS)        /* string constant          */
//              indx = (int32) strConstIndexList[indx - (int32) (STR_OFS + 1)];
//            else if (indx > gblsbeg)    /* global string variable   */
//              indx = gblsbas + (indx - gblsbeg) * p->strVarSamples;
//            else if (indx > gblabeg)    /* global a-rate variable   */
//              indx = gblabeg + (indx - gblabeg) * p->ksmps;
//          }
//          else {                        /* negative index: local    */
//            posndx = -indx;
//            if (indx < LABELIM)         /* label                    */
//              continue;
//            if (posndx > lclsbeg)       /* local string variable    */
//              indx = -(lclsbas + (posndx - lclsbeg) * p->strVarSamples);
//            else if (posndx > lclabeg)  /* local a-rate variable    */
//              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
//          }
//          *ndxp = (int) indx;
//        }
      }
    }
//    p->Free(p, strConstIndexList);

    p->tpidsr = TWOPI_F / p->esr;               /* now set internal  */
    p->mtpdsr = -(p->tpidsr);                   /*    consts         */
    p->pidsr = PI_F / p->esr;
    p->mpidsr = -(p->pidsr);
    p->onedksmps = FL(1.0) / (MYFLT) p->ksmps;
    p->sicvt = FMAXLEN / p->esr;
    p->kicvt = FMAXLEN / p->ekr;
    p->onedsr = FL(1.0) / p->esr;
    p->onedkr = FL(1.0) / p->ekr;
    /* IV - Sep 8 2002: save global variables that depend on ksmps */
    p->global_ksmps     = p->ksmps;
    p->global_ekr       = p->ekr;
    p->global_kcounter  = p->kcounter;
    reverbinit(p);
    dbfs_init(p, p->e0dbfs);
    p->nspout = p->ksmps * p->nchnls;  /* alloc spin & spout */
    p->nspin = p->ksmps * p->inchnls; /* JPff: in preparation */
    p->spin  = (MYFLT *) mcalloc(p, p->nspin * sizeof(MYFLT));
    p->spout = (MYFLT *) mcalloc(p, p->nspout * sizeof(MYFLT));
    /* chk consistency one more time (FIXME: needed ?) */
    {
      char  s[256];
      sprintf(s, Str("sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:"),
                 p->esr, p->ekr, ensmps);
      if (UNLIKELY(p->ksmps < 1 || FLOAT_COMPARE(ensmps, p->ksmps)))
        csoundDie(p, Str("%s invalid ksmps value"), s);
      if (UNLIKELY(p->esr <= FL(0.0)))
        csoundDie(p, Str("%s invalid sample rate"), s);
      if (UNLIKELY(p->ekr <= FL(0.0)))
        csoundDie(p, Str("%s invalid control rate"), s);
      if (UNLIKELY(FLOAT_COMPARE(p->esr, (double) p->ekr * ensmps)))
        csoundDie(p, Str("%s inconsistent sr, kr, ksmps"), s);
    }
    /* initialise sensevents state */
    p->prvbt = p->curbt = p->nxtbt = 0.0;
    p->curp2 = p->nxtim = p->timeOffs = p->beatOffs = 0.0;
    p->icurTime = 0L;
    if (O->Beatmode && O->cmdTempo > 0) {
      /* if performing from beats, set the initial tempo */
      p->curBeat_inc = (double) O->cmdTempo / (60.0 * (double) p->ekr);
      p->ibeatTime = (int64_t)(p->esr*60.0 / (double) O->cmdTempo);
    }
    else {
      p->curBeat_inc = 1.0 / (double) p->ekr;
      p->ibeatTime = 1;
    }
    p->cyclesRemaining = 0;
    memset(&(p->evt), 0, sizeof(EVTBLK));

    /* pre-allocate temporary label space for instance() */
//    p->lopds = (LBLBLK**) mmalloc(p, sizeof(LBLBLK*) * p->nlabels);
//    p->larg = (LARGNO*) mmalloc(p, sizeof(LARGNO) * p->ngotos);

    /* run instr 0 inits */
    if (UNLIKELY(init0(p) != 0))
      csoundDie(p, Str("header init errors"));
}
