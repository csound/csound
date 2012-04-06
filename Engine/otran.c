/*
    otran.c:

    Copyright (C) 1991, 1997, 2003 Barry Vercoe, John ffitch, Istvan Varga

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

#include "csoundCore.h"         /*                      OTRAN.C         */
#include <math.h>
#include <ctype.h>
#include <locale.h>

#include "oload.h"
#include "insert.h"
#include "pstream.h"
#include "namedins.h"           /* IV - Oct 31 2002 */
#include "corfile.h"

static  NAME    *lclnamset(CSOUND *, char *);
static  void    delete_global_namepool(CSOUND *);
static  void    delete_local_namepool(CSOUND *);

#define txtcpy(a,b) memcpy(a,b,sizeof(TEXT));
#define STA(x)   (csound->otranStatics.x)

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

void tranRESET(CSOUND *csound)
{
    void  *p;

    delete_local_namepool(csound);
    delete_global_namepool(csound);
    p = (void*) csound->opcodlst;
    csound->opcodlst = NULL;
    csound->oplstend = NULL;
    if (p != NULL)
      free(p);
    //csound->otranGlobals = NULL;
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

static inline unsigned int MYFLT_hash(const MYFLT *x)
{
    const unsigned char *c = (const unsigned char*) x;
    size_t              i;
    unsigned int        h = 0U;

    for (i = (size_t) 0; i < sizeof(MYFLT); i++)
      h = (unsigned int) strhash_tabl_8[(unsigned int) c[i] ^ h];

    return h;
}

void putop(CSOUND *csound, TEXT *tp)
{
    int n, nn;

    if ((n = tp->outlist->count) != 0) {
      nn = 0;
      while (n--)
        csound->Message(csound, "%s\t", tp->outlist->arg[nn++]);
    }
    else
      csound->Message(csound, "\t");
    csound->Message(csound, "%s\t", tp->opcod);
    if ((n = tp->inlist->count) != 0) {
      nn = 0;
      while (n--)
        csound->Message(csound, "%s\t", tp->inlist->arg[nn++]);
    }
    csound->Message(csound, "\n");
}

static NAME *lclnamset(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(csound, s);
    NAME          *p = STA(lclNames)[h];
                                                /* search lcl namelist: */
    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)                              /* if name is there     */
      return p;                                 /*    return ptr        */
    p = (NAME*) malloc(sizeof(NAME));
    if (UNLIKELY(p == NULL))
      csound->Die(csound, Str("lclnamset(): memory allocation failure"));
    p->namep = s;                               /* else record newname  */
    p->nxt = STA(lclNames)[h];
    STA(lclNames)[h] = p;
    if (*s == '#')  s++;
    switch (*s) {                               /*   and its type-count */
      case 'w': p->type = WTYPE; p->count = STA(lclnxtwcnt)++; break;
      case 'a': p->type = ATYPE; p->count = STA(lclnxtacnt)++; break;
      case 'f': p->type = PTYPE; p->count = STA(lclnxtpcnt)++; break;
      case 'S': p->type = STYPE; p->count = STA(lclnxtscnt)++; break;
      default:  p->type = KTYPE; p->count = STA(lclnxtkcnt)++; break;
    }
    return p;
}

static void delete_global_namepool(CSOUND *csound)
{
    int i;

    /* if (csound->otranGlobals == NULL) */
    /*   return; */
    for (i = 0; i < 256; i++) {
      while (STA(gblNames)[i] != NULL) {
        NAME  *nxt = STA(gblNames)[i]->nxt;
        free(STA(gblNames)[i]);
        STA(gblNames)[i] = nxt;
      }
    }
}

static void delete_local_namepool(CSOUND *csound)
{
    int i;

    /* if (csound->otranGlobals == NULL) */
    /*   return; */
    for (i = 0; i < 256; i++) {
      while (STA(lclNames)[i] != NULL) {
        NAME  *nxt = STA(lclNames)[i]->nxt;
        free(STA(lclNames)[i]);
        STA(lclNames)[i] = nxt;
      }
    }
}

 /* ------------------------------------------------------------------------ */

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

void oload(CSOUND *p)
{
    int32    n, combinedsize, insno, *lp;
    int32    gblabeg, gblsbeg, gblsbas, gblscbeg, lclabeg, lclsbeg, lclsbas;
    MYFLT   *combinedspc, *gblspace, *fp1;
    INSTRTXT *ip;
    OPTXT   *optxt;
    OPARMS  *O = p->oparms;
    int     *strConstIndexList;
    MYFLT   ensmps;

    p->esr = p->tran_sr; p->ekr = p->tran_kr;
    p->e0dbfs = p->tran_0dbfs;
    p->ksmps = (int) ((ensmps = p->tran_ksmps) + FL(0.5));
    ip = p->instxtanchor.nxtinstxt;        /* for instr 0 optxts:  */
    optxt = (OPTXT *) ip;
    while ((optxt = optxt->nxtop) !=  NULL) {
      TEXT  *ttp = &optxt->t;
      ARGOFFS *inoffp, *outoffp;
      int opnum = ttp->opnum;
      if (opnum == ENDIN) break;
      if (opnum == LABEL) continue;
      outoffp = ttp->outoffs;           /* use unexpanded ndxes */
      inoffp = ttp->inoffs;             /* to find sr.. assigns */
      if (outoffp->count == 1 && inoffp->count == 1) {
        int rindex = (int) outoffp->indx[0] - (int) p->poolcount;
        if (rindex > 0 && rindex <= 6) {
          MYFLT conval = p->pool[inoffp->indx[0] - 1];
          switch (rindex) {
            case 1:  p->esr = conval;   break;  /* & use those values */
            case 2:  p->ekr = conval;   break;  /*  to set params now */
            case 3:  p->ksmps = (int) ((ensmps = conval) + FL(0.5)); break;
            case 4:  p->nchnls = (int) (conval + FL(0.5));  break;
            case 5:  p->inchnls = (int) (conval + FL(0.5));  break;
            case 6:
            default: p->e0dbfs = conval; break;
          }
        }
      }
    }
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
    p->strVarSamples = (p->strVarMaxLen + (int) sizeof(MYFLT) - 1)
                       / (int) sizeof(MYFLT);
    p->strVarMaxLen = p->strVarSamples * (int) sizeof(MYFLT);
    /* calculate total size of global pool */
    combinedsize = p->poolcount                 /* floating point constants */
                   + p->gblfixed                /* k-rate / spectral        */
                   + p->gblacount * p->ksmps            /* a-rate variables */
                   + p->gblscount * p->strVarSamples;   /* string variables */
    gblscbeg = combinedsize + 1;                /* string constants         */
    combinedsize += create_strconst_ndx_list(p, &strConstIndexList, gblscbeg);

    combinedspc = (MYFLT*) mcalloc(p, combinedsize * sizeof(MYFLT));
    /* copy pool into combined space */
    memcpy(combinedspc, p->pool, p->poolcount * sizeof(MYFLT));
    mfree(p, (void*) p->pool);
    p->pool = combinedspc;
    gblspace = p->pool + p->poolcount;
    gblspace[0] = p->esr;           /*   & enter        */
    gblspace[1] = p->ekr;           /*   rsvd word      */
    gblspace[2] = (MYFLT) p->ksmps; /*   curr vals      */
    gblspace[3] = (MYFLT) p->nchnls;
    if (p->inchnls<0) p->inchnls = p->nchnls;
    gblspace[4] = (MYFLT) p->inchnls;
    gblspace[5] = p->e0dbfs;
    p->gbloffbas = p->pool - 1;
    /* string constants: unquote, convert escape sequences, and copy to pool */
    convert_strconst_pool(p, (MYFLT*) p->gbloffbas + (int32) gblscbeg);

    gblabeg = p->poolcount + p->gblfixed + 1;
    gblsbeg = gblabeg + p->gblacount;
    gblsbas = gblabeg + (p->gblacount * p->ksmps);
    ip = &(p->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL) {      /* EXPAND NDX for A & S Cells */
      optxt = (OPTXT *) ip;                     /*   (and set localen)        */
      lclabeg = (int32) (ip->pmax + ip->lclfixed + 1);
      lclsbeg = (int32) (lclabeg + ip->lclacnt);
      lclsbas = (int32) (lclabeg + (ip->lclacnt * (int32) p->ksmps));
      if (UNLIKELY(O->odebug)) p->Message(p, "lclabeg %d, lclsbeg %d\n",
                                   lclabeg, lclsbeg);
      ip->localen = ((int32) ip->lclfixed
                     + (int32) ip->lclacnt * (int32) p->ksmps
                     + (int32) ip->lclscnt * (int32) p->strVarSamples)
                    * (int32) sizeof(MYFLT);
      /* align to 64 bits */
      ip->localen = (ip->localen + 7L) & (~7L);
      for (insno = 0, n = 0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  n++;            /* count insnos  */
      lp = ip->inslist = (int32 *) mmalloc(p, (int32)(n+1) * sizeof(int32));
      for (insno=0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  *lp++ = insno;  /* creat inslist */
      *lp = -1;                                         /*   & terminate */
      insno = *ip->inslist;                             /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT    *ttp = &optxt->t;
        ARGOFFS *aoffp;
        int32    indx;
        int32    posndx;
        int     *ndxp;
        int     opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;    /* IV - Sep 8 2002 */
        if (opnum == LABEL) continue;
        aoffp = ttp->outoffs;           /* ------- OUTARGS -------- */
        n = aoffp->count;
        for (ndxp = aoffp->indx; n--; ndxp++) {
          indx = *ndxp;
          if (indx > 0) {               /* positive index: global   */
            if (UNLIKELY(indx >= STR_OFS))        /* string constant          */
              p->Die(p, Str("internal error: string constant outarg"));
            if (indx > gblsbeg)         /* global string variable   */
              indx = gblsbas + (indx - gblsbeg) * p->strVarSamples;
            else if (indx > gblabeg)    /* global a-rate variable   */
              indx = gblabeg + (indx - gblabeg) * p->ksmps;
            else if (indx <= 3 && O->sr_override &&
                     ip == p->instxtanchor.nxtinstxt)   /* for instr 0 */
              indx += 3;        /* deflect any old sr,kr,ksmps targets */
          }
          else {                        /* negative index: local    */
            posndx = -indx;
            if (indx < LABELIM)         /* label                    */
              continue;
            if (posndx > lclsbeg)       /* local string variable    */
              indx = -(lclsbas + (posndx - lclsbeg) * p->strVarSamples);
            else if (posndx > lclabeg)  /* local a-rate variable    */
              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
          }
          *ndxp = (int) indx;
        }
        aoffp = ttp->inoffs;            /* inargs:                  */
        if (opnum >= SETEND) goto realops;
        switch (opnum) {                /*      do oload SETs NOW   */
        case PSET:
          p->Message(p, "PSET: isno=%d, pmax=%d\n", insno, ip->pmax);
          if ((n = aoffp->count) != ip->pmax) {
            p->Warning(p, Str("i%d pset args != pmax"), (int) insno);
            if (n < ip->pmax) n = ip->pmax; /* cf pset, pmax    */
          }                                 /* alloc the larger */
          ip->psetdata = (MYFLT *) mcalloc(p, n * sizeof(MYFLT));
          for (n = aoffp->count, fp1 = ip->psetdata, ndxp = aoffp->indx;
               n--; ) {
            *fp1++ = p->gbloffbas[*ndxp++];
            p->Message(p, "..%f..", *(fp1-1));
          }
          p->Message(p, "\n");
          break;
        }
        continue;       /* no runtime role for the above SET types */

      realops:
        n = aoffp->count;               /* -------- INARGS -------- */
        for (ndxp = aoffp->indx; n--; ndxp++) {
          indx = *ndxp;
          if (indx > 0) {               /* positive index: global   */
            if (indx >= STR_OFS)        /* string constant          */
              indx = (int32) strConstIndexList[indx - (int32) (STR_OFS + 1)];
            else if (indx > gblsbeg)    /* global string variable   */
              indx = gblsbas + (indx - gblsbeg) * p->strVarSamples;
            else if (indx > gblabeg)    /* global a-rate variable   */
              indx = gblabeg + (indx - gblabeg) * p->ksmps;
          }
          else {                        /* negative index: local    */
            posndx = -indx;
            if (indx < LABELIM)         /* label                    */
              continue;
            if (posndx > lclsbeg)       /* local string variable    */
              indx = -(lclsbas + (posndx - lclsbeg) * p->strVarSamples);
            else if (posndx > lclabeg)  /* local a-rate variable    */
              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
          }
          *ndxp = (int) indx;
        }
      }
    }
    p->Free(p, strConstIndexList);

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
    p->lopds = (LBLBLK**) mmalloc(p, sizeof(LBLBLK*) * p->nlabels);
    p->larg = (LARGNO*) mmalloc(p, sizeof(LARGNO) * p->ngotos);

    /* run instr 0 inits */
    if (UNLIKELY(init0(p) != 0))
      csoundDie(p, Str("header init errors"));
}
