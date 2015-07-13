/*
    fgens.c:

    Copyright (C) 1991, 1994, 1995, 1998, 2000, 2004
                  Barry Vercoe, John ffitch, Paris Smaragdis,
                  Gabriel Maldonado, Richard Karpen, Greg Sullivan,
                  Pete Moss, Istvan Varga, Victor Lazzarini

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

#include "csoundCore.h"         /*                      FGENS.C         */
#include <ctype.h>
#include "soundio.h"
#include "cwindow.h"
#include "cmath.h"
#include "fgens.h"
#include "pstream.h"
#include "pvfileio.h"
#include <stdlib.h>

extern double besseli(double);

static int gen01raw(FGDATA *, FUNC *);
static int gen01(FGDATA *, FUNC *), gen02(FGDATA *, FUNC *);
static int gen03(FGDATA *, FUNC *), gen04(FGDATA *, FUNC *);
static int gen05(FGDATA *, FUNC *), gen06(FGDATA *, FUNC *);
static int gen07(FGDATA *, FUNC *), gen08(FGDATA *, FUNC *);
static int gen09(FGDATA *, FUNC *), gen10(FGDATA *, FUNC *);
static int gen11(FGDATA *, FUNC *), gen12(FGDATA *, FUNC *);
static int gen13(FGDATA *, FUNC *), gen14(FGDATA *, FUNC *);
static int gen15(FGDATA *, FUNC *), gen17(FGDATA *, FUNC *);
static int gen18(FGDATA *, FUNC *), gen19(FGDATA *, FUNC *);
static int gen20(FGDATA *, FUNC *), gen21(FGDATA *, FUNC *);
static int gen23(FGDATA *, FUNC *), gen24(FGDATA *, FUNC *);
static int gen16(FGDATA *, FUNC *), gen25(FGDATA *, FUNC *);
static int gen27(FGDATA *, FUNC *), gen28(FGDATA *, FUNC *);
static int gen30(FGDATA *, FUNC *), gen31(FGDATA *, FUNC *);
static int gen32(FGDATA *, FUNC *), gen33(FGDATA *, FUNC *);
static int gen34(FGDATA *, FUNC *), gen40(FGDATA *, FUNC *);
static int gen41(FGDATA *, FUNC *), gen42(FGDATA *, FUNC *);
static int gen43(FGDATA *, FUNC *);
static int gn1314(FGDATA *, FUNC *, MYFLT, MYFLT);
static int gen51(FGDATA *, FUNC *), gen52(FGDATA *, FUNC *);
static int gen53(FGDATA *, FUNC *);
static int GENUL(FGDATA *, FUNC *);
#ifndef NACL
static int gen49(FGDATA *, FUNC *);
#endif

static const GEN or_sub[GENMAX + 1] = {
    GENUL,
    gen01, gen02, gen03, gen04, gen05, gen06, gen07, gen08, gen09, gen10,
    gen11, gen12, gen13, gen14, gen15, gen16, gen17, gen18, gen19, gen20,
    gen21, GENUL, gen23, gen24, gen25, GENUL, gen27, gen28, GENUL, gen30,
    gen31, gen32, gen33, gen34, GENUL, GENUL, GENUL, GENUL, GENUL, gen40,
    gen41, gen42, gen43, GENUL, GENUL, GENUL, GENUL, GENUL,
#ifndef NACL
    gen49,
#else
    GENUL,
#endif
    GENUL,
    gen51, gen52, gen53, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL
};

typedef struct namedgen {
    char    *name;
    int     genum;
    struct namedgen *next;
} NAMEDGEN;

#define tpd360  (FL(0.0174532925199433))

#define FTAB_SEARCH_BASE (100)

static CS_NOINLINE int  fterror(const FGDATA *, const char *, ...);
static CS_NOINLINE void ftresdisp(const FGDATA *, FUNC *);
static CS_NOINLINE FUNC *ftalloc(const FGDATA *);

static int GENUL(FGDATA *ff, FUNC *ftp)
{
    (void) ftp;
    return fterror(ff, Str("unknown GEN number"));
}

/**
 * Create ftable using evtblk data, and store pointer to new table in *ftpp.
 * If mode is zero, a zero table number is ignored, otherwise a new table
 * number is automatically assigned.
 * Returns zero on success.
 */

int hfgens(CSOUND *csound, FUNC **ftpp, const EVTBLK *evtblkp, int mode)
{
    int32    genum, ltest;
    int     lobits, msg_enabled, i;
    FUNC    *ftp;
    FGDATA  ff;
    int nonpowof2_flag=0; /* gab: fixed for non-powoftwo function tables*/

    *ftpp = NULL;
    if (UNLIKELY(csound->gensub == NULL)) {
      csound->gensub = (GEN*) csound->Malloc(csound, sizeof(GEN) * (GENMAX + 1));
      memcpy(csound->gensub, or_sub, sizeof(GEN) * (GENMAX + 1));
      csound->genmax = GENMAX + 1;
    }
    msg_enabled = csound->oparms->msglevel & 7;
    ff.csound = csound;
    memcpy((char*) &(ff.e), (char*) evtblkp,
           (size_t) ((char*) &(evtblkp->p[2]) - (char*) evtblkp));
    ff.fno = (int) MYFLT2LRND(ff.e.p[1]);
    if (!ff.fno) {
      if (!mode)
        return 0;                               /*  fno = 0: return,        */
      ff.fno = FTAB_SEARCH_BASE;
      do {                                      /*      or automatic number */
        ++ff.fno;
      } while (ff.fno <= csound->maxfnum && csound->flist[ff.fno] != NULL);
      ff.e.p[1] = (MYFLT) (ff.fno);
    }
    else if (ff.fno < 0) {                      /*  fno < 0: remove         */
      ff.fno = -(ff.fno);
      if (UNLIKELY(ff.fno > csound->maxfnum ||
                   (ftp = csound->flist[ff.fno]) == NULL)) {
        return fterror(&ff, Str("ftable does not exist"));
      }
      csound->flist[ff.fno] = NULL;
      csound->Free(csound, (void*) ftp);
      if (UNLIKELY(msg_enabled))
        csoundMessage(csound, Str("ftable %d now deleted\n"), ff.fno);
      return 0;
    }
    if (UNLIKELY(ff.fno > csound->maxfnum)) {   /* extend list if necessary */
      FUNC  **nn;
      int   size;
      for (size = csound->maxfnum; size < ff.fno; size += MAXFNUM)
        ;
      nn = (FUNC**) csound->ReAlloc(csound,
                                    csound->flist, (size + 1) * sizeof(FUNC*));
      csound->flist = nn;
      for (i = csound->maxfnum + 1; i <= size; i++)
        csound->flist[i] = NULL;                /*  Clear new section       */
      csound->maxfnum = size;
    }
    if (UNLIKELY(ff.e.pcnt <= 4)) {             /*  chk minimum arg count   */
      return fterror(&ff, Str("insufficient gen arguments"));
    }
    if (ff.e.pcnt>PMAX) {
      //#ifdef BETA
      csound->DebugMsg(csound, "T%d/%d(%d): x=%p memcpy from %p to %p length %ld\n",
              (int)evtblkp->p[1], (int)evtblkp->p[4], ff.e.pcnt, evtblkp->c.extra,
              &(ff.e.p[2]), &(evtblkp->p[2]), sizeof(MYFLT) * PMAX);
      //#endif
      memcpy(&(ff.e.p[2]), &(evtblkp->p[2]), sizeof(MYFLT) * (PMAX-2));
      ff.e.c.extra = (MYFLT*)malloc(sizeof(MYFLT) * (evtblkp->c.extra[0]+1));
      memcpy(ff.e.c.extra, evtblkp->c.extra,
             sizeof(MYFLT) * (evtblkp->c.extra[0]+1));
    }
    else
      memcpy(&(ff.e.p[2]), &(evtblkp->p[2]),
             sizeof(MYFLT) * ((int) ff.e.pcnt - 1));
    if (ISSTRCOD(ff.e.p[4])) {
      /* A named gen given so search the list of extra gens */
      NAMEDGEN *n = (NAMEDGEN*) csound->namedgen;
      while (n) {
        if (strcmp(n->name, ff.e.strarg) == 0) {    /* Look up by name */
          genum = n->genum;
          break;
        }
        n = n->next;                            /*  and round again         */
      }
      if (UNLIKELY(n == NULL)) {
        return fterror(&ff, Str("Named gen \"%s\" not defined"), ff.e.strarg);
      }
    }
    else {
      genum = (int32) MYFLT2LRND(ff.e.p[4]);
      if (genum < 0)
        genum = -genum;
      if (UNLIKELY(!genum || genum > csound->genmax)) { /*   & legal gen number x*/
        return fterror(&ff, Str("illegal gen number"));
      }
    }
    ff.flen = (int32) MYFLT2LRND(ff.e.p[3]);
    if (!ff.flen) {
      /* defer alloc to gen01|gen23|gen28 */
      ff.guardreq = 1;
      if (UNLIKELY(genum != 1 && genum != 23 && genum != 28 && genum != 49)) {
        return fterror(&ff, Str("deferred size for GENs 1, 23, 28 or 49 only"));
      }
      if (msg_enabled)
        csoundMessage(csound, Str("ftable %d:\n"), ff.fno);
      i = (*csound->gensub[genum])(&ff, NULL);
      ftp = csound->flist[ff.fno];
      if (i != 0) {
        csound->flist[ff.fno] = NULL;
        csound->Free(csound, ftp);
        return -1;
      }
      *ftpp = ftp;
      return 0;
    }
    /* if user flen given */
    if (ff.flen < 0L) {                 /* gab for non-pow-of-two-length    */
      ff.guardreq = 1;
      ff.flen = -(ff.flen);             /* gab: fixed */
      if (!(ff.flen & (ff.flen - 1L)) || ff.flen > MAXLEN)
        goto powOfTwoLen;
      lobits = 0;                       /* Hope this is not needed! */
      nonpowof2_flag = 1; /* gab: fixed for non-powoftwo function tables*/
    }
    else {
      ff.guardreq = ff.flen & 01;       /*  set guard request flg   */
      ff.flen &= -2L;                   /*  flen now w/o guardpt    */
 powOfTwoLen:
      if (UNLIKELY(ff.flen <= 0L || ff.flen > MAXLEN)) {
        return fterror(&ff, Str("illegal table length"));
      }
      for (ltest = ff.flen, lobits = 0;
           (ltest & MAXLEN) == 0L;
           lobits++, ltest <<= 1)
        ;
      if (UNLIKELY(ltest != MAXLEN)) {  /*  flen is not power-of-2 */
        // return fterror(&ff, Str("illegal table length"));
        //csound->Warning(csound, Str("table %d size not power of two"), ff.fno);
        lobits = 0;
        nonpowof2_flag = 1;
        ff.guardreq = 1;
      }
    }
    ftp = ftalloc(&ff);                 /*  alloc ftable space now  */
    ftp->lenmask  = ((ff.flen & (ff.flen - 1L)) ?
                     0L : (ff.flen - 1L));      /*  init hdr w powof2 data  */
    ftp->lobits   = lobits;
    i = (1 << lobits);
    ftp->lomask   = (int32) (i - 1);
    ftp->lodiv    = FL(1.0) / (MYFLT) i;        /*    & other useful vals   */
    ftp->nchanls  = 1;                          /*    presume mono for now  */
    ftp->flenfrms = ff.flen;
    if (nonpowof2_flag)
      ftp->lenmask = 0xFFFFFFFF; /* gab: fixed for non-powoftwo function tables */

    if (msg_enabled)
      csoundMessage(csound, Str("ftable %d:\n"), ff.fno);
    if ((*csound->gensub[genum])(&ff, ftp) != 0) {
      csound->flist[ff.fno] = NULL;
      csound->Free(csound, ftp);
      return -1;
    }
    /* VL 11.01.05 for deferred GEN01, it's called in gen01raw */
    ftresdisp(&ff, ftp);                        /* rescale and display      */
    *ftpp = ftp;
    /* keep original arguments, from GEN number  */
    ftp->argcnt = ff.e.pcnt - 3;
    {  /* Note this does not handle extened args -- JPff */
      int size=ftp->argcnt;
      if (size>PMAX-4) size=PMAX-4;
      /* printf("size = %d -> %d ftp->args = %p\n", */
      /*        size, sizeof(MYFLT)*size, ftp->args); */
      memcpy(ftp->args, &(ff.e.p[4]), sizeof(MYFLT)*size); /* is this right? */
      /*for(k=0; k < size; k++)
        csound->Message(csound, "%f \n", ftp->args[k]);*/
    }
    return 0;
}

/**
 * Allocates space for 'tableNum' with a length (not including the guard
 * point) of 'len' samples. The table data is not cleared to zero.
 * Return value is zero on success.
 */

int csoundFTAlloc(CSOUND *csound, int tableNum, int len)
{
    int   i, size;
    FUNC  **nn, *ftp;

    if (UNLIKELY(tableNum <= 0 || len <= 0 || len > (int) MAXLEN))
      return -1;
    if (UNLIKELY(tableNum > csound->maxfnum)) { /* extend list if necessary     */
      for (size = csound->maxfnum; size < tableNum; size += MAXFNUM)
        ;
      nn = (FUNC**) csound->ReAlloc(csound,
                                    csound->flist, (size + 1) * sizeof(FUNC*));
      csound->flist = nn;
      for (i = csound->maxfnum + 1; i <= size; i++)
        csound->flist[i] = NULL;            /* Clear new section            */
      csound->maxfnum = size;
    }
    /* allocate space for table */
    size = (int) (len * (int) sizeof(MYFLT));
    ftp = csound->flist[tableNum];
    if (ftp == NULL) {
      csound->flist[tableNum] = (FUNC*) csound->Malloc(csound, sizeof(FUNC));
      csound->flist[tableNum]->ftable =
        (MYFLT*)csound->Malloc(csound, sizeof(MYFLT)*(len+1));
    }
    else if (len != (int) ftp->flen) {
      if (csound->actanchor.nxtact != NULL) { /*   & chk for danger    */
        /* return */  /* VL: changed this into a Warning */
          csound->Warning(csound, Str("ftable %d relocating due to size change"
                                        "\n         currently active instruments "
                                        "may find this disturbing"), tableNum);
      }
      csound->flist[tableNum] = NULL;
      csound->Free(csound, ftp);
      csound->flist[tableNum] = (FUNC*) csound->Malloc(csound, (size_t) size);
    }
    /* initialise table header */
    ftp = csound->flist[tableNum];
    //memset((void*) ftp, 0, (size_t) ((char*) &(ftp->ftable) - (char*) ftp));
    ftp->flen = (int32) len;
    if (!(len & (len - 1))) {
      /* for power of two length: */
      ftp->lenmask = (int32) (len - 1);
      for (i = len, ftp->lobits = 0L; i < (int) MAXLEN; ftp->lobits++, i <<= 1)
        ;
      i = (int) MAXLEN / len;
      ftp->lomask = (int32) (i - 1);
      ftp->lodiv = FL(1.0) / (MYFLT) i;
    }
    ftp->flenfrms = (int32) len;
    ftp->nchanls = 1L;
    ftp->fno = (int32) tableNum;

    return 0;
}

/**
 * Deletes a function table.
 * Return value is zero on success.
 */

int csoundFTDelete(CSOUND *csound, int tableNum)
{
    FUNC  *ftp;

    if (UNLIKELY((unsigned int) (tableNum - 1) >= (unsigned int) csound->maxfnum))
      return -1;
    ftp = csound->flist[tableNum];
    if (UNLIKELY(ftp == NULL))
      return -1;
    csound->flist[tableNum] = NULL;
    csound->Free(csound, ftp);

    return 0;
}

/* read ftable values directly from p-args */

static int gen02(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *fp = ftp->ftable, *pp = &(ff->e.p[5]);
    int     nvals = ff->e.pcnt - 4;
    int nsw = 1;
    CSOUND  *csound = ff->csound;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if (nvals >= (int) ff->flen)
      nvals = (int) ff->flen + 1;               /* for all vals up to flen+1 */
    while (nvals--) {
      *fp++ = *pp++;                            /*   copy into ftable   */
      if (UNLIKELY(nsw && pp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        pp = &(ff->e.c.extra[1]);
      }
    }
    return OK;
}

static int gen03(FGDATA *ff, FUNC *ftp)
{
    int     ncoefs, nargs = ff->e.pcnt - 4;
    MYFLT   xintvl, xscale;
    int     xloc, nlocs;
    MYFLT   *fp = ftp->ftable, x, sum, *coefp, *coef0, *coeflim;

    if (UNLIKELY((ncoefs = nargs - 2) <= 0)) {
      return fterror(ff, Str("no coefs present"));
    }
    coef0 = &ff->e.p[7];
    coeflim = coef0 + ncoefs;
    if (UNLIKELY((xintvl = ff->e.p[6] - ff->e.p[5]) <= 0)) {
      return fterror(ff, Str("illegal x interval"));
    }
    xscale = xintvl / (MYFLT)ff->flen;
    xloc = (int) (ff->e.p[5] / xscale);        /* initial xloc */
    nlocs = (int) ff->flen + 1;
    do {                                       /* for each loc:        */
      x     = xloc++ * xscale;
      coefp = coeflim;
      sum   = *--coefp;                        /* init sum to coef(n)  */
      while (coefp > coef0) {
        sum *= x;                              /*  & accum by Horner's rule */
        sum += *--coefp;
      }
      *fp++ = sum;
    } while (--nlocs);

    return OK;
}

static int gen04(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *valp, *rvalp, *fp = ftp->ftable;
    int     n, r;
    FUNC    *srcftp;
    MYFLT   val, max, maxinv;
    int     srcno, srcpts, ptratio;

    if (UNLIKELY(ff->e.pcnt < 6)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if (UNLIKELY((srcno = (int)ff->e.p[5]) <= 0 || srcno > csound->maxfnum ||
                 (srcftp = csound->flist[srcno]) == NULL)) {
      return fterror(ff, Str("unknown srctable number"));
    }
    if (!ff->e.p[6]) {
      srcpts = srcftp->flen;
      valp   = srcftp->ftable;
      rvalp  = NULL;
    }
    else {
      srcpts = srcftp->flen >>1;
      valp   = &srcftp->ftable[srcpts];
      rvalp  = valp - 1;
    }
    if (UNLIKELY((ptratio = srcpts / ff->flen) < 1)) {
      return fterror(ff, Str("table size too large"));
    }
    if ((val = *valp++)) {
      if (val < FL(0.0))      val = -val;
      max = val;
      maxinv = FL(1.0) / max;
    }
    else {
      max = FL(0.0);
      maxinv = FL(1.0);
    }
    *fp++ = maxinv;
    for (n = ff->flen; n--; ) {
      for (r = ptratio; r--; ) {
        if ((val = *valp++)) {
          if (val < FL(0.0)) val = -val;
          if (val > max) {
            max = val;
            maxinv = FL(1.0) / max;
          }
        }
        if (rvalp != NULL && (val = *rvalp--)) {
          if (val < 0.)   val = -val;
          if (val > max) {
            max = val;
            maxinv = FL(1.0) / max;
          }
        }
      }
      *fp++ = maxinv;
    }
    ff->guardreq = 1;                  /* disable new guard point */
    ff->e.p[4] = -FL(4.0);             /*   and rescaling         */

    return OK;
}

static int gen05(FGDATA *ff, FUNC *ftp)
{
    int     nsegs, seglen;
    MYFLT   *valp, *fp, *finp;
    MYFLT   amp1, mult;
    int nsw = 1;
    CSOUND  *csound = ff->csound;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if ((nsegs = (ff->e.pcnt-5) >> 1) <= 0)    /* nsegs = nargs-1 /2 */
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    if (UNLIKELY(*valp == 0)) goto gn5er2;
    do {
      amp1 = *valp++;
      if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])){
        valp = &(ff->e.c.extra[1]);
        nsw  = 0;
      }
      if (!(seglen = (int)*valp++)) {
        if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])){
          valp = &(ff->e.c.extra[1]);
          nsw  = 0;
        }
        continue;
      }
      if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])){
        valp = &(ff->e.c.extra[1]);
        nsw  = 0;
      }
      if (UNLIKELY(seglen < 0)) goto gn5er1;
      if (UNLIKELY((mult = *valp/amp1) <= 0)) goto gn5er2;
      mult = POWER(mult, FL(1.0)/seglen);
      while (seglen--) {
        *fp++ = amp1;
        amp1 *= mult;
        if (fp > finp)
          return OK;
      }
    } while (--nsegs);
    if (fp == finp)                 /* if 2**n pnts, add guardpt */
      *fp = amp1;
    return OK;

 gn5er1:
    return fterror(ff, Str("gen call has negative segment size:"));
 gn5er2:
    return fterror(ff, Str("illegal input vals for gen call, beginning:"));
}

static int gen07(FGDATA *ff, FUNC *ftp)
{
    int     nsegs, seglen;
    MYFLT   *valp, *fp, *finp;
    MYFLT   amp1, incr;

    if ((nsegs = (ff->e.pcnt-5) >> 1) <= 0)         /* nsegs = nargs-1 /2 */
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    do {
      amp1 = *valp++;
      if (!(seglen = (int)*valp++)) continue;
      if (UNLIKELY(seglen < 0)) goto gn7err;
      incr = (*valp - amp1) / seglen;
      while (seglen--) {
        *fp++ = amp1;
        amp1 += incr;
        if (fp > finp)
          return OK;
      }
    } while (--nsegs);
    if (fp == finp)                 /* if 2**n pnts, add guardpt */
      *fp = amp1;
    return OK;

 gn7err:
    return fterror(ff, Str("gen call has negative segment size:"));
}

static int gen06(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *segp, *extremp, *inflexp, *segptsp, *fp, *finp;
    MYFLT   y, diff2;
    int     pntno, pntinc, nsegs, npts;
    int nsw = 1;
    CSOUND  *csound = ff->csound;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if (UNLIKELY((nsegs = ((ff->e.pcnt - 5) >> 1)) < 1)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    fp = ftp->ftable;
    finp = fp + ff->flen;
    pntinc = 1;
    for (segp = &ff->e.p[3], segptsp = &ff->e.p[4]; nsegs > 0; nsegs--) {
      segp += 1;
      if (UNLIKELY(nsw && segp>&ff->e.p[PMAX])){
          segp = &(ff->e.c.extra[1]);
          nsw  = 0;
        }
      segp += 1;
      if (UNLIKELY(nsw && segp>&ff->e.p[PMAX])){
          segp = &(ff->e.c.extra[1]);
          nsw  = 0;
        }
      segptsp = segp + 1;
      if (UNLIKELY(nsw && segptsp>&ff->e.p[PMAX])){
          segptsp = &(ff->e.c.extra[1]);
        }
      if (UNLIKELY((npts = (int)*segptsp) < 0)) {
        return fterror(ff, Str("negative segsiz"));
      }
      if (pntinc > 0) {
        pntno   = 0;
        inflexp = segp + 1;
        if (UNLIKELY(nsw && inflexp>&ff->e.p[PMAX])){
          inflexp = &(ff->e.c.extra[1]);
        }
        inflexp++;
        if (UNLIKELY(nsw && inflexp>&ff->e.p[PMAX])){
          inflexp = &(ff->e.c.extra[1]);
        }
        extremp = segp;
      }
      else {
        pntno   = npts;
        inflexp = segp;
        extremp = segp + 1;
        if (UNLIKELY(nsw && extremp>&ff->e.p[PMAX])){
          extremp = &(ff->e.c.extra[1]);
        }
        extremp++;
        if (UNLIKELY(nsw && extremp>&ff->e.p[PMAX])){
          extremp = &(ff->e.c.extra[1]);
        }
      }
      diff2 = (*inflexp - *extremp) * FL(0.5);
      for ( ; npts > 0 && fp < finp; pntno += pntinc, npts--) {
        y = (MYFLT)pntno / *segptsp;
        *fp++ = (FL(3.0)-y) * y * y * diff2 + *extremp;
      }
      pntinc = -pntinc;
    }
    segp += 1;
    if (UNLIKELY(nsw && segp>&ff->e.p[PMAX])){
      segp = &(ff->e.c.extra[1]);
      nsw  = 0;
    }
    segp += 1;
    if (UNLIKELY(nsw && segp>&ff->e.p[PMAX])){
      segp = &(ff->e.c.extra[1]);
      nsw  = 0;
    }
    *fp = *(segp);                      /* write last target point */

    return OK;
}

static int gen08(FGDATA *ff, FUNC *ftp)
{
    MYFLT   R, x, c3, c2, c1, c0, *fp, *fplim, *valp;
    MYFLT   f2 = FL(0.0), f1, f0, df1, df0, dx01, dx12 = FL(0.0), curx;
    MYFLT   slope, resd1, resd0;
    int     nsegs, npts;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if (UNLIKELY((nsegs = (ff->e.pcnt - 5) >> 1) <= 0)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    fplim = fp + ff->flen;
    f0 = *valp++;                    /* 1st 3 params give vals at x0, x1 */
    if (UNLIKELY((dx01 = *valp++) <= FL(0.0))) {      /* and dist between*/
      return fterror(ff, Str("illegal x interval"));
    }
    f1 = *valp++;
    curx = df0 = FL(0.0);           /* init x to origin; slope at x0 = 0 */
    do {                            /* for each spline segmnt (x0 to x1) */
      if (nsegs > 1) {                      /* if another seg to follow  */
        MYFLT dx02;
        if (UNLIKELY((dx12 = *valp++) <= FL(0.0))) {  /*  read its distance  */
          return fterror(ff, Str("illegal x interval"));
        }
        f2 = *valp++;                       /*    and the value at x2    */
        if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])) {
#ifdef BETA
          csound->DebugMsg(csound, "Switch to extra args\n");
#endif
          nsw = 0;                /* only switch once */
          valp = &(ff->e.c.extra[1]);
        }
        dx02 = dx01 + dx12;
        df1 = ( f2*dx01*dx01 + f1*(dx12-dx01)*dx02 - f0*dx12*dx12 )
          / (dx01*dx02*dx12);
      }                                /* df1 is slope of parabola at x1 */
      else df1 = FL(0.0);
      if ((npts = (int) (dx01 - curx)) > fplim - fp)
        npts = fplim - fp;
      if (npts > 0) {                       /* for non-trivial segment: */
        slope = (f1 - f0) / dx01;           /*   get slope x0 to x1     */
        resd0 = df0 - slope;                /*   then residual slope    */
        resd1 = df1 - slope;                /*     at x0 and x1         */
        c3 = (resd0 + resd1) / (dx01*dx01);
        c2 = - (resd1 + FL(2.0)*resd0) / dx01;
        c1 = df0;                           /*   and calc cubic coefs   */
        c0 = f0;
        for (x = curx; npts>0; --npts, x += FL(1.0)) {
          R     = c3;
          R    *= x;
          R    += c2;            /* f(x) = ((c3 x + c2) x + c1) x + c0  */
          R    *= x;
          R    += c1;
          R    *= x;
          R    += c0;
          *fp++ = R;                        /* store n pts for this seg */
        }
        curx = x;
      }
      curx -= dx01;                 /* back up x by length last segment */
      dx01  = dx12;                     /* relocate to the next segment */
      f0    = f1;                       /*   by assuming its parameters */
      f1    = f2;
      df0   = df1;
    } while (--nsegs && fp<fplim);      /* loop for remaining segments  */
    while (fp <= fplim)
      *fp++ = f0;                       /* & repeat the last value      */
    return OK;
}

static int gen09(FGDATA *ff, FUNC *ftp)
{
    int     hcnt;
    MYFLT   *valp, *fp, *finp;
    double  phs, inc, amp;
    double  tpdlen = TWOPI / (double) ff->flen;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if ((hcnt = (ff->e.pcnt - 4) / 3) <= 0)         /* hcnt = nargs / 3 */
      return OK;
    valp = &ff->e.p[5];
    finp = &ftp->ftable[ff->flen];
    do {
      inc = *(valp++) * tpdlen;
      if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        valp = &(ff->e.c.extra[1]);
      }
      amp = *(valp++);
      if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        valp = &(ff->e.c.extra[1]);
      }
      phs = *(valp++) * tpd360;
      if (UNLIKELY(nsw && valp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        valp = &(ff->e.c.extra[1]);
      }
      for (fp = ftp->ftable; fp <= finp; fp++) {
        *fp += (MYFLT) (sin(phs) * amp);
        if ((phs += inc) >= TWOPI)
          phs -= TWOPI;
      }
    } while (--hcnt);

    return OK;
}

static int gen10(FGDATA *ff, FUNC *ftp)
{
    int32   phs, hcnt;
    MYFLT   amp, *fp, *finp;
    int32   flen = ff->flen;
    double  tpdlen = TWOPI / (double) flen;
    CSOUND  *csound = ff->csound;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    hcnt = ff->e.pcnt - 4;                              /* hcnt is nargs    */
    finp = &ftp->ftable[flen];
    do {
      MYFLT *valp = (hcnt+4>=PMAX ? &ff->e.c.extra[hcnt+5-PMAX] :
                                    &ff->e.p[hcnt + 4]);
      if ((amp = *valp) != FL(0.0))         /* for non-0 amps,  */
        for (phs = 0, fp = ftp->ftable; fp <= finp; fp++) {
          *fp += (MYFLT) sin(phs * tpdlen) * amp;         /* accum sin pts    */
          phs += hcnt;                                    /* phsinc is hno    */
          phs %= flen;
        }
    } while (--hcnt);

    return OK;
}

static int gen11(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *fp, *finp;
    int32    phs;
    double  x;
    MYFLT   denom, r, scale;
    int     n, k;
    int     nargs = ff->e.pcnt - 4;

    if (UNLIKELY((n = (int) ff->e.p[5]) < 1)) {
      return fterror(ff, Str("nh partials < 1"));
    }
    k = 1;
    r = FL(1.0);
    if (ff->e.pcnt > 5)
      k = (int) ff->e.p[6];
    if (nargs > 2)
      r = ff->e.p[7];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    if (ff->e.pcnt == 5 || (k == 1 && r == FL(1.0))) {  /* simple "buzz" case */
      int tnp1;
      MYFLT pdlen;

      tnp1  = (n << 1) + 1;
      scale = FL(0.5) / n;
      pdlen = PI_F / (MYFLT) ff->flen;
      for (phs = 0; fp <= finp; phs++) {
        x = phs * pdlen;
        if (!(denom = (MYFLT) sin(x)))
          *fp++ = FL(1.0);
        else *fp++ = ((MYFLT) sin(tnp1 * x) / denom - FL(1.0)) * scale;
      }
    }
    else {                                   /* complex "gbuzz" case */
      double  tpdlen = TWOPI / (double) ff->flen;
      MYFLT   numer, twor, rsqp1, rtn, rtnp1, absr;
      int     km1, kpn, kpnm1;

      km1   = k - 1;
      kpn   = k + n;
      kpnm1 = kpn - 1;
      twor  = r * FL(2.0);
      rsqp1 = r * r + FL(1.0);
      rtn   = intpow(r, (int32) n);
      rtnp1 = rtn * r;
      if ((absr =  FABS(r)) > FL(0.999) && absr < FL(1.001))
        scale = FL(1.0) / n;
      else scale = (FL(1.0) - absr) / (FL(1.0) - FABS(rtn));
      for (phs = 0; fp <= finp; phs++) {
        x = (double) phs * tpdlen;
        numer = (MYFLT)cos(x*k) - r * (MYFLT)cos(x*km1) - rtn*(MYFLT)cos(x*kpn)
                + rtnp1 * (MYFLT)cos(x*kpnm1);
        if ((denom = rsqp1 - twor * (MYFLT) cos(x)) > FL(0.0001)
            || denom < -FL(0.0001))
          *fp++ = numer / denom * scale;
        else *fp++ = FL(1.0);
      }
    }
    return OK;
}

static int gen12(FGDATA *ff, FUNC *ftp)
{
    static const double coefs[] = { 3.5156229, 3.0899424, 1.2067492,
                                    0.2659732, 0.0360768, 0.0045813 };
    const double *coefp, *cplim = coefs + 6;
    double  sum, tsquare, evenpowr;
    int     n;
    MYFLT   *fp;
    double  xscale;

    xscale = (double) ff->e.p[5] / ff->flen / 3.75;
    for (n = 0, fp = ftp->ftable; n <= ff->flen; n++) {
      tsquare  = (double) n * xscale;
      tsquare *= tsquare;
      for (sum = evenpowr = 1.0, coefp = coefs; coefp < cplim; coefp++) {
        evenpowr *= tsquare;
        sum += *coefp * evenpowr;
      }
      *fp++ = (MYFLT) log(sum);
    }
    return OK;
}

static int gen13(FGDATA *ff, FUNC *ftp)
{
    return gn1314(ff, ftp, FL(2.0), FL(0.5));
}

static int gen14(FGDATA *ff, FUNC *ftp)
{
    return gn1314(ff, ftp, FL(1.0), FL(1.0));
}

static int gn1314(FGDATA *ff, FUNC *ftp, MYFLT mxval, MYFLT mxscal)
{
    CSOUND  *csound = ff->csound;
    int32    nh, nn;
    MYFLT   *mp, *mspace, *hp, *oddhp;
    MYFLT   xamp, xintvl, scalfac, sum, prvm;
    int nsw = 1;

    if (UNLIKELY(ff->e.pcnt>=PMAX))
      csound->Warning(csound, Str("using extended arguments\n"));
    if (UNLIKELY((nh = ff->e.pcnt - 6) <= 0)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if (UNLIKELY((xintvl = ff->e.p[5]) <= 0)) {
      return fterror(ff, Str("illegal xint value"));
    }
    if (UNLIKELY((xamp = ff->e.p[6]) <= 0)) {
      return fterror(ff, Str("illegal xamp value"));
    }
    ff->e.p[5] = -xintvl;
    ff->e.p[6] = xintvl;
    nn = nh * sizeof(MYFLT) / 2;              /* alloc spc for terms 3,5,7,..*/
    mp = mspace = (MYFLT *)calloc(1, nn);       /* of 1st row of matrix, and */
    for (nn = (nh + 1) >>1; --nn; )             /* form array of non-0 terms */
      *mp++ = mxval = -mxval;                   /*  -val, val, -val, val ... */
    scalfac = 2 / xamp;
    hp = &ff->e.p[7];                           /* beginning with given h0,  */
    do {
      mp = mspace;
      oddhp = hp;
      sum = *oddhp++;                           /* sum = diag(=1) * this h   */
      if (UNLIKELY(nsw && oddhp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        oddhp = &(ff->e.c.extra[1]);
      }
      for (nn = (nh+1) >>1; --nn; ) {
        int nnsw = nsw;
        oddhp++;                                /*  + odd terms * h+2,h+4,.. */
        if (UNLIKELY(nnsw && oddhp>&ff->e.p[PMAX])) {
#ifdef BETA
          csound->DebugMsg(csound, "Switch to extra args\n");
#endif
          nnsw = 0;                /* only switch once */
          oddhp = &(ff->e.c.extra[1]);
        }
        sum += *mp++ * *oddhp++;
        if (UNLIKELY(nnsw && oddhp>&ff->e.p[PMAX])) {
#ifdef BETA
          csound->DebugMsg(csound, "Switch to extra args\n");
#endif
          nnsw = 0;                /* only switch once */
          oddhp = &(ff->e.c.extra[1]);
        }
      }
      *hp++ = sum * mxscal;                     /* repl this h w. coef (sum) */
      if (UNLIKELY(nsw && hp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        hp = &(ff->e.c.extra[1]);
      }
      mp    = mspace;
      prvm  = FL(1.0);
      for (nn = nh>>1; --nn > 0; mp++)          /* calc nxt row matrix terms */
        *mp = prvm = *mp - prvm;
      mxscal *= scalfac;
    } while (--nh);                             /* loop til all h's replaced */
    free(mspace);
    return gen03(ff, ftp);                      /* then call gen03 to write */
}

static int gen15(FGDATA *ff, FUNC *ftp)
{
    MYFLT   xint, xamp, *hsin, h, angle;
    MYFLT   *fp, *cosp, *sinp;
    int     n, nh;
    void    *lp13;
    int     nargs = ff->e.pcnt - 4;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    hsin = (MYFLT*)malloc(sizeof(MYFLT)*((1+ff->e.pcnt)/2));
    if (UNLIKELY(nargs & 01)) {
      free(hsin);
      return fterror(ff, Str("uneven number of args"));
    }
    nh = (nargs - 2) >>1;
    fp   = &ff->e.p[5];                         /* save p5, p6  */
    xint = *fp++;
    xamp = *fp++;
    for (n = nh, cosp = fp, sinp = hsin; n > 0; n--) {
      h = *fp++;                                /* rpl h,angle pairs */
      if (UNLIKELY(nsw && fp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        fp = &(ff->e.c.extra[1]);
      }
      angle = (MYFLT) (*fp++ * tpd360);
      *cosp++ = h * COS(angle);  /* with h cos angle */
      *sinp++ = h * SIN(angle);  /* and save the sine */
    }
    nargs -= nh;
    ff->e.pcnt = (int16)(nargs + 4);            /* added by F. Pinot 16-01-2012 */
    if (gen13(ff, ftp) != OK) {                  /* call gen13   */
      free(hsin);
      return NOTOK;
    }
    lp13 = (void*) ftp;
    ff->fno++;                                  /* alloc eq. space for fno+1 */
    ftp = ftalloc(ff);                          /* & copy header */
    memcpy((void*) ftp, lp13, (size_t) sizeof(FUNC)-sizeof(MYFLT*));
    ftp->fno = (int32) ff->fno;
    fp    = &ff->e.p[5];
    nsw = 1;
    *fp++ = xint;                               /* restore p5, p6,   */
    *fp++ = xamp;
    for (n = nh-1, sinp = hsin+1; n > 0; n--) { /* then skip h0*sin  */
      *fp++ = *sinp++;                          /* & copy rem hn*sin */
      if (UNLIKELY(nsw && fp>&ff->e.p[PMAX])) {
#ifdef BETA
        csound->DebugMsg(csound, "Switch to extra args\n");
#endif
        nsw = 0;                /* only switch once */
        fp = &(ff->e.c.extra[1]);
      }
    }
    nargs--;
    ff->e.pcnt = (int16)(nargs + 4); /* added by F. Pinot 16-01-2012 */
    free(hsin);
    n = gen14(ff, ftp);       /* now draw ftable   */
    ftresdisp(ff, ftp);       /* added by F. Pinot 16-01-2012 */
    ff->fno--;                /* F. Pinot, the first function table */
                              /* is scaled and displayed by hfgens */
    return n;
}

static int gen16(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *fp, *valp, val;
    int     nargs = ff->e.pcnt - 4;
    int     nseg = nargs / 3;

    fp = ftp->ftable;
    valp = &ff->e.p[5];
    *fp++ = val = *valp++;
    while (nseg-- > 0) {
      MYFLT dur    = *valp++;
      MYFLT alpha  = *valp++;
      MYFLT nxtval = *valp++;
      int32 cnt = (int32) (dur + FL(0.5));
      if (alpha == FL(0.0)) {
        MYFLT c1 = (nxtval-val)/dur;
        while (cnt-- > 0) {
          *fp++ = val = val + c1;
        }
      }
      else {
        MYFLT c1 = (nxtval - val)/(FL(1.0) - EXP(alpha));
        MYFLT x;
        alpha /= dur;
        x = alpha;
        while (cnt-->0) {
          *fp++ = val + c1 * (FL(1.0) - EXP(x));
          x += alpha;
        }
        val = *(fp-1);
      }
    }
    return OK;
}

static int gen17(FGDATA *ff, FUNC *ftp)
{
    int     nsegs, ndx, nxtndx;
    MYFLT   *valp, *fp, *finp;
    MYFLT   val;
    int     nargs = ff->e.pcnt - 4;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if ((nsegs = nargs >> 1) <= 0)       /* nsegs = nargs /2 */
      goto gn17err;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    if (UNLIKELY((ndx = (int)*valp++) != 0))
      goto gn17err;
    while (--nsegs) {
      val = *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      if (UNLIKELY((nxtndx = (int)*valp++) <= ndx))
        goto gn17err;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      do {
        *fp++ = val;
        if (fp > finp)
          return OK;
      } while (++ndx < nxtndx);
    }
    val = *valp;
    while (fp <= finp)                    /* include 2**n + 1 guardpt */
      *fp++ = val;
    return OK;

 gn17err:
    return fterror(ff, Str("gen call has illegal x-ordinate values:"));
}

/* by pete moss (petemoss@petemoss.org), jan 2002 */

static int gen18(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    int     cnt, start, finish, fnlen, j;
    MYFLT   *pp = &ff->e.p[5], fn, amp, *fp, *fp18 = ftp->ftable, range, f;
    double  i;
    FUNC    *fnp;
    int     nargs = ff->e.pcnt - 4;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if (UNLIKELY((cnt = nargs >> 2) <= 0)) {
      return fterror(ff, Str("wrong number of args"));
    }
    while (cnt--) {
      fn=*pp++;
      if (UNLIKELY(nsw && pp>=&ff->e.p[PMAX-1])) nsw =0, pp = &(ff->e.c.extra[1]);
      amp=*pp++;
      if (UNLIKELY(nsw && pp>=&ff->e.p[PMAX-1])) nsw =0, pp = &(ff->e.c.extra[1]);
      start=(int)*pp++;
      if (UNLIKELY(nsw && pp>=&ff->e.p[PMAX-1])) nsw =0, pp = &(ff->e.c.extra[1]);
      finish=(int)*pp++;
      if (UNLIKELY(nsw && pp>=&ff->e.p[PMAX-1])) nsw =0, pp = &(ff->e.c.extra[1]);

      if (UNLIKELY((start>ff->flen) || (finish>=ff->flen))) {
        /* make sure start and finish < flen */
        return fterror(ff, Str("a range given exceeds table length"));
      }

      if (LIKELY((fnp=csoundFTFind(csound,&fn))!=NULL)) { /* make sure fn exists */
        fp = fnp->ftable, fnlen = fnp->flen-1;        /* and set it up */
      }
      else {
        return fterror(ff, Str("an input function does not exist"));
      }

      range = (MYFLT) (finish - start), j = start;
      while (j <= finish) {                      /* write the table */
        f = (MYFLT)modf((fnlen*(j++ - start)/range), &i);
        if (i==fnp->flen)
          fp18[j] += amp * fp[(int)i];
        else
          fp18[j] += amp * ((f * (*(fp + (int)(i+1)) -
                                  *(fp + (int)i))) +
                            *(fp + (int)i));
      }
    }
    return OK;
}

static int gen19(FGDATA *ff, FUNC *ftp)
{
    int     hcnt;
    MYFLT   *valp, *fp, *finp;
    double  phs, inc, amp, dc, tpdlen = TWOPI / (double) ff->flen;
    int     nargs = ff->e.pcnt - 4;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if ((hcnt = nargs / 4) <= 0)                /* hcnt = nargs / 4 */
      return OK;
    valp = &ff->e.p[5];
    finp = &ftp->ftable[ff->flen];
    do {
      inc = *(valp++) * tpdlen;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      amp = *(valp++);
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      phs = *(valp++) * tpd360;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      dc = *(valp++);
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      for (fp = ftp->ftable; fp <= finp; fp++) {
        *fp += (MYFLT) (sin(phs) * amp + dc);   /* dc after str scale */
        if ((phs += inc) >= TWOPI)
          phs -= TWOPI;
      }
    } while (--hcnt);

    return OK;
}

/*  GEN20 and GEN21 by Paris Smaragdis 1994 B.C.M. Csound development team  */

static int gen20(FGDATA *ff, FUNC *ftp)
{
    MYFLT   cf[4], *ft;
    double  arg, x, xarg, beta = 0.0,varian = 1.0;
    int     i, nargs = ff->e.pcnt - 4;

    ft = ftp->ftable;
    xarg = 1.0;

    if (ff->e.p[4] < FL(0.0)) {
      xarg = ff->e.p[6];
      if ( nargs < 2 ) xarg = 1.0;
    }

    if (nargs > 2){
      beta = (double) ff->e.p[7];
      varian = (double) ff->e.p[7];
    }

    switch ((int) ff->e.p[5])  {
    case 1:                     /* Hamming */
        cf[0] = FL(0.54);
        cf[1] = FL(0.46);
        cf[2] = cf[3] = FL(0.0);
        break;
    case 2:                     /* Hanning */
        cf[0] = cf[1] = FL(0.5);
        cf[2] = cf[3] = FL(0.0);
        break;
    case 3:                     /* Bartlett */
        arg = 2.0/ff->flen;
        for (i = 0, x = 0.0 ; i < ((int) ff->flen >> 1) ; i++, x++)
            ft[i] = (MYFLT) (x * arg * xarg);
        for ( ; i < (int) ff->flen ; i++, x++)
            ft[i] = (MYFLT) ((2.0 - x * arg) * xarg);
        return OK;
    case 4:                     /* Blackman */
        cf[0] = FL(0.42);
        cf[1] = FL(0.5);
        cf[2] = FL(0.08);
        cf[3] = FL(0.0);
        break;
    case 5:                     /* Blackman-Harris */
        cf[0] = FL(0.35878);
        cf[1] = FL(0.48829);
        cf[2] = FL(0.14128);
        cf[3] = FL(0.01168);
        break;
    case 6:                     /* Gaussian */
        arg = 12.0 / ff->flen;
        for (i = 0, x = -6.0 ; i < ((int) ff->flen >> 1) ; i++, x += arg)
          ft[i] = (MYFLT)(xarg * (pow(2.718281828459,-(x*x)/(2.0*varian*varian))));
        for (x = 0.0 ; i <= (int) ff->flen ; i++, x += arg)
          ft[i] = (MYFLT)(xarg * (pow(2.718281828459,-(x*x)/(2.0*varian*varian))));
        return OK;
    case 7:                     /* Kaiser */
      {
        double flen2 = 4.0 / ((double) ff->flen * (double) ff->flen);
        double besbeta = 1.0 / besseli(beta);
        x = (double) ff->flen * (-0.5) + 1.0;
        ft[0] = ft[ff->flen] = (MYFLT) (xarg * besbeta);
        for (i = 1 ; i < (int) ff->flen ; i++, x += 1.0)
          ft[i] = (MYFLT) (xarg * besseli(beta * sqrt(1.0 - x * x * flen2))
                                * besbeta);
        return OK;
      }
    case 8:                     /* Rectangular */
        for (i = 0; i <= (int) ff->flen ; i++)
          ft[i] = FL(1.0);
        return OK;
    case 9:                     /* Sinc */
        arg = TWOPI / ff->flen;
        for (i = 0, x = -PI ; i < ((int) ff->flen >> 1) ; i++, x += arg)
          ft[i] = (MYFLT) (xarg * sin(x) / x);
        ft[i++] = (MYFLT) xarg;
        for (x = arg ; i <= (int) ff->flen ; i++, x += arg)
          ft[i] = (MYFLT) (xarg * sin(x) / x);
        return OK;
    default:
        return fterror(ff, Str("No such window!"));
    }

    arg = TWOPI / ff->flen;

    for (i = 0, x = 0.0 ; i <= (int) ff->flen ; i++, x += arg)
      ft[i] = (MYFLT) (xarg * (cf[0] - cf[1]*cos(x) + cf[2]*cos(2.0 * x)
                                     - cf[3]*cos(3.0 * x)));

    return OK;
}

static int gen21(FGDATA *ff, FUNC *ftp)
{
    int     retval = gen21_rand(ff, ftp);

    switch (retval) {
      case 0:   break;
      case -1:  return fterror(ff, Str("Wrong number of input arguments"));
      case -2:  return fterror(ff, Str("unknown distribution"));
      default:  return NOTOK;
    }
    return OK;
}

static MYFLT nextval(FILE *f)
{
    /* Read the next charcater; suppress multiple space and comments to a
       single space */
    int c;
 top:
    c = getc(f);
 top1:
    if (feof(f)) return NAN; /* Hope value is ignored */
    if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') {
      double d;                           /* A number starts */
      char buff[128];
      int j = 0;
      do {                                /* Fill buffer */
        buff[j++] = c;
        c = getc(f);
      } while (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.');
      buff[j]='\0';
      d = atof(buff);
      if (c==';' || c=='#') {             /* If exended with comment clear it now */
        while ((c = getc(f)) != '\n');
      }
      return (MYFLT)d;
    }
    while (isspace(c) || c == ',') c = getc(f);       /* Whitespace */
    if (c==';' || c=='#' || c=='<') {     /* Comment and tag*/
      while ((c = getc(f)) != '\n');
    }
    if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') goto top1;
    goto top;
}

static int gen23(FGDATA *ff, FUNC *ftp)
                                /* ASCII file table read Gab 17-feb-98*/
                                /* Modified after Paris Smaragdis by JPff */
{                               /* And further hacked 2009 by JPff */
    CSOUND  *csound = ff->csound;
    MYFLT   *fp;
    FILE    *infile;
    void    *fd;
    int     j;
    MYFLT   tmp;

    fd = csound->FileOpen2(csound, &infile, CSFILE_STD, ff->e.strarg, "r",
                           "SFDIR;SSDIR;INCDIR", CSFTYPE_FLOATS_TEXT, 0);
    if (UNLIKELY(fd == NULL)) {
      return fterror(ff, Str("error opening ASCII file"));
    }
    if (ftp == NULL) {
      /* Start counting elements */
      ff->flen = 0;
      do {
        ff->flen++;
        nextval(infile);
      } while (!feof(infile));
      ff->flen--; // overshoots by 1
      csoundMessage(csound, Str("%ld elements in %s\n"),
                    (long) ff->flen, ff->e.strarg);
      rewind(infile);
      /* Allocate memory and read them in now */
  /*  ff->flen      = ff->flen + 2;        ??? */
      ftp           = ftalloc(ff);
      ftp->lenmask  = 0xFFFFFFFF; /* avoid the error in csoundFTFind */
    }
    fp = ftp->ftable;
    j = 0;
    while (!feof(infile) && j < ff->flen) fp[j++] = nextval(infile);
    tmp = nextval(infile); // overshot value
    if (UNLIKELY(!feof(infile)))
      csound->Warning(csound,
                      Str("Number(s) after table full in GEN23, starting %f"), tmp);
    csound->FileClose(csound, fd);
    // if (def)
    {
      MYFLT *tab = ftp->ftable;
      tab[ff->flen] = tab[0];  /* guard point */
      //ftp->flen -= 1;  /* exclude guard point */
      ftresdisp(ff, ftp);       /* VL: 11.01.05  for deferred alloc tables */
    }


    return OK;
}

static int gen24(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp = ftp->ftable, *fp_source;
    FUNC    *srcftp;
    int     srcno, srcpts, j;
    MYFLT   max, min, new_max, new_min, source_amp, target_amp, amp_ratio;
    int     nargs = ff->e.pcnt - 4;

    if (UNLIKELY(nargs < 3)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if (UNLIKELY((srcno = (int) ff->e.p[5]) <= 0 ||
        srcno > csound->maxfnum         ||
                 (srcftp = csound->flist[srcno]) == NULL)) {
      return fterror(ff, Str("unknown srctable number"));
    }
    fp_source = srcftp->ftable;

    new_min = ff->e.p[6];
    new_max = ff->e.p[7];
    srcpts = srcftp->flen;
    if (UNLIKELY(srcpts!= ff->flen)) {
      return fterror(ff, Str("table size must be the same of source table"));
    }
    max = min = fp_source[0];
    for (j = 0; j < srcpts; j++) {
      if (fp_source[j] > max ) max = fp_source[j];
      if (fp_source[j] < min ) min = fp_source[j];
    }

    source_amp = max - min;
    target_amp = new_max - new_min;
    amp_ratio = target_amp/source_amp;

    for (j = 0; j < srcpts; j++) {
      fp[j] = (fp_source[j]-min) * amp_ratio + new_min;
    }
    fp[j] = fp[j-1];

    return OK;
}

static int gen25(FGDATA *ff, FUNC *ftp)
{
    int     nsegs,  seglen;
    MYFLT   *valp, *fp, *finp;
    MYFLT   x1, x2, y1, y2, mult;
    int     nargs = ff->e.pcnt - 4;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if ((nsegs = ((nargs / 2) - 1)) <= 0)
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    do {
      x1 = *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      y1 =  *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      x2 = *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      if (LIKELY(nsegs > 1)) {
        y2 =  *valp++;
        if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
          nsw =0, valp = &(ff->e.c.extra[1]);
      }
      else
        y2 = *valp;
      if (UNLIKELY(x2 < x1)) goto gn25err;
      if (UNLIKELY(x1 > ff->flen || x2 > ff->flen)) goto gn25err2;
      seglen = (int)(x2-x1);
      if (UNLIKELY(y1 <= 0 || y2 <= 0)) goto gn25err3;
      mult = y2/y1;
      mult = POWER(mult, FL(1.0)/seglen);
      while (seglen--) {
        *fp++ = y1;
        y1 *= mult;
        if (fp > finp)
          return OK;
      }
      valp -= 2;
    } while (--nsegs);
    if (fp == finp)                     /* if 2**n pnts, add guardpt */
      *fp = y1;
    return OK;

 gn25err:
    return fterror(ff, Str("x coordinates must all be in increasing order:"));

 gn25err2:
    return fterror(ff, Str("x coordinate greater than function size:"));

 gn25err3:
    return fterror(ff,
                   Str("illegal input val (y <= 0) for gen call, beginning:"));
}

static int gen27(FGDATA *ff, FUNC *ftp)
{
    int     nsegs;
    MYFLT   *valp, *fp, *finp;
    MYFLT   x1, x2, y1, y2, yy, seglen, incr;
    int     nargs = ff->e.pcnt - 4;
    CSOUND  *csound = ff->csound;
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if ((nsegs = ((nargs / 2) - 1)) <= 0)
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    //printf("valp=%p end=%p extra=%p\n",
    //       valp, &ff->e.p[PMAX-1], &(ff->e.c.extra[1]));
    x2 = *valp++; y2 = *valp++;
    do {
      x1 = x2; y1 = y2;
      x2 = *valp++;
      if (UNLIKELY(nsw && valp>&ff->e.p[PMAX-1])) {
        //printf("extend: valp=%p extra=%p\n", valp, &(ff->e.c.extra[1]));
        nsw =0, valp = &(ff->e.c.extra[1]);
        //printf("extendx2: valp=%p\n", valp);
      }
      //if (nsw==0) printf("extend: valp=%p\n", valp);
      if (LIKELY(nsegs > 1)) {
        y2 =  *valp++;
        if (UNLIKELY(nsw && valp>&ff->e.p[PMAX-1])) {
          //printf("extendy2: valp=%p extra=%p\n", valp, &(ff->e.c.extra[1]));
          nsw =0, valp = &(ff->e.c.extra[1]);
          //printf("extend: valp=%p\n", valp);
        }
      }
      else {
        y2 = *valp;
        //printf("end of list: valp = %p x1,y1,x2,y2 = %f,%f,%f,%f\n",
        //       valp, x1, y1, x2, y2);
      }
      if (UNLIKELY(x2 < x1)) goto gn27err;
      if (UNLIKELY(x1 > ff->flen || x2 > ff->flen)) goto gn27err2;
      seglen = x2-x1;
      incr = (y2 - y1) / seglen;
      yy = y1;
      while (seglen--) {
        *fp++ = yy;
        yy += incr;
        if (fp > finp)
          return OK;
      }
    } while (--nsegs);
    if (fp == finp)                     /* if 2**n pnts, add guardpt */
      *fp = y1;
    return OK;

 gn27err:
    printf("nsegs=%d x1,y1 = %f,%f x2,y2 = %f,%f\n", nsegs, x1, y1, x2, y2);
    return fterror(ff, Str("x coordinates must all be in increasing order:"));
 gn27err2:
    return fterror(ff, Str("x coordinate greater than function size:"));
}

/* read X Y values directly from ascii file */

static int gen28(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp = ftp->ftable, *finp;
    int     seglen, resolution = 100;
    FILE    *filp;
    void    *fd;
    int     i=0, j=0;
    MYFLT   *x, *y, *z;
    int     arraysize = 1000;
    MYFLT   x1, y1, z1, x2, y2, z2, incrx, incry;

    if (UNLIKELY(ff->flen))
      return fterror(ff, Str("GEN28 requires zero table length"));
    fd = csound->FileOpen2(csound, &filp, CSFILE_STD, ff->e.strarg, "r",
                          "SFDIR;SSDIR;INCDIR", CSFTYPE_FLOATS_TEXT, 0);
    if (UNLIKELY(fd == NULL))
      goto gen28err1;

    x = (MYFLT*)malloc(arraysize*sizeof(MYFLT));
    y = (MYFLT*)malloc(arraysize*sizeof(MYFLT));
    z = (MYFLT*)malloc(arraysize*sizeof(MYFLT));
#if defined(USE_DOUBLE)
    while (fscanf( filp, "%lf%lf%lf", &z[i], &x[i], &y[i])!= EOF)
#else
    while (fscanf( filp, "%f%f%f", &z[i], &x[i], &y[i])!= EOF)
#endif
      {
        i++;
        if (i>=arraysize) {
          MYFLT* newx, *newy, *newz;
          arraysize += 1000;
          newx = (MYFLT*)realloc(x, arraysize*sizeof(MYFLT));
          newy = (MYFLT*)realloc(y, arraysize*sizeof(MYFLT));
          newz = (MYFLT*)realloc(z, arraysize*sizeof(MYFLT));
          if (!newx || !newy || !newz) {
            fprintf(stderr, Str("Out of Memory\n"));
            exit(7);
          }
          x = newx; y = newy; z = newz;
        }
      }
    --i;

    ff->flen      = (int32) (z[i] * resolution * 2);
    ff->flen      = ff->flen + 2;       /* ??? */
    ftp           = ftalloc(ff);
    fp            = ftp->ftable;
    finp          = fp + ff->flen;

    do {
      x1 = x[j];
      y1 = y[j];
      x2 = x[j+1];
      y2 = y[j+1];
      z1 = z[j];
      z2 = z[j+1];

      if (UNLIKELY(z2 < z1)) goto gen28err2;
      seglen = (int)((z2-z1) * resolution);
      incrx = (x2 - x1) / (MYFLT)seglen;
      incry = (y2 - y1) / (MYFLT)seglen;
      while (seglen--) {
        *fp++ = x1;
        x1   += incrx;
        *fp++ = y1;
        y1   += incry;
      }

      j++;
    } while (--i);
    do {
      *fp++ = x[j];
      *fp++ = y[j+1];
    } while (fp < finp);

    free(x); free(y); free(z);
    csound->FileClose(csound, fd);

    return OK;

 gen28err1:
    return fterror(ff, Str("could not open space file"));
 gen28err2:
    free(x); free(y); free(z);
    return fterror(ff, Str("Time values must be in increasing order"));
}

/* gen30: extract a range of harmonic partials from source table */

static int gen30(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *x, *f1, *f2;
    int     l1, l2, minh = 0, maxh = 0, i;
    MYFLT   xsr, minfrac, maxfrac;
    int     nargs = ff->e.pcnt - 4;

    if (UNLIKELY(nargs < 3)) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    xsr = FL(1.0);
    if ((nargs > 3) && (ff->e.p[8] > FL(0.0)))
      xsr = csound->esr / ff->e.p[8];
    l2 = csoundGetTable(csound, &f2, (int) ff->e.p[5]);
    if (UNLIKELY(l2 < 0)) {
      return fterror(ff, Str("GEN30: source ftable not found"));
    }
    f1 = ftp->ftable;
    l1 = (int) ftp->flen;
    minfrac = ff->e.p[6];           /* lowest harmonic partial number */
    maxfrac = ff->e.p[7] * xsr;     /* highest harmonic partial number */
    i = (l1 < l2 ? l1 : l2) >> 1;   /* sr/2 limit */
    /* limit to 0 - sr/2 range */
    if ((maxfrac < FL(0.0)) || (minfrac > (MYFLT) i))
      return OK;
    if (maxfrac > (MYFLT) i)
      maxfrac = (MYFLT) i;
    if (minfrac < FL(0.0))
      minfrac = FL(0.0);
    if ((nargs > 4) && (ff->e.p[9] != FL(0.0))) {
      minh     = (int) minfrac;     /* "interpolation" mode */
      minfrac -= (MYFLT) minh;
      minfrac  = FL(1.0) - minfrac;
      maxh     = (int) maxfrac;
      maxfrac -= (MYFLT) (maxh++);
      if (maxh > i) {
        maxh = i; maxfrac = FL(1.0);
      }
    }
    else {
      minh = (int) ((double) minfrac + (i < 10000 ? 0.99 : 0.9));
      maxh = (int) ((double) maxfrac + (i < 10000 ? 0.01 : 0.1));
      minfrac = maxfrac = FL(1.0);
    }
    if (minh > maxh)
      return OK;
    i = (l1 > l2 ? l1 : l2) + 2;
    x = (MYFLT*) malloc(sizeof(MYFLT) * i);
    /* read src table with amplitude scale */
    xsr = csound->GetInverseRealFFTScale(csound, l1) * (MYFLT) l1 / (MYFLT) l2;
    for (i = 0; i < l2; i++)
      x[i] = xsr * f2[i];
    /* filter */
    csound->RealFFT(csound, x, l2);
    x[l2] = x[1];
    x[1] = x[l2 + 1] = FL(0.0);
    for (i = 0; i < (minh << 1); i++)
      x[i] = FL(0.0);
    x[i++] *= minfrac;
    x[i] *= minfrac;
    i = maxh << 1;
    x[i++] *= maxfrac;
    x[i++] *= maxfrac;
    for ( ; i < (l1 + 2); i++)
      x[i] = FL(0.0);
    x[1] = x[l1];
    x[l1] = x[l1 + 1] = FL(0.0);
    csound->InverseRealFFT(csound, x, l1);
    /* write dest. table */
    /* memcpy(f1, x, l1*sizeof(MYFLT)); */
    for (i = 0; i < l1; i++)
      f1[i] = x[i];
    f1[l1] = f1[0];     /* write guard point */
    free(x);

    return OK;
}

/* gen31: transpose, phase shift, and mix source table */

static int gen31(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *x, *y, *f1, *f2;
    MYFLT   a, p;
    double  d_re, d_im, p_re, p_im, ptmp;
    int     i, j, k, n, l1, l2;
    int     nargs = ff->e.pcnt - 4;
    MYFLT   *valp = &ff->e.p[6];
    int nsw = 1;

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    if (UNLIKELY(nargs < 4)) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    l2 = csoundGetTable(csound, &f2, (int) ff->e.p[5]);
    if (UNLIKELY(l2 < 0)) {
      return fterror(ff, Str("GEN31: source ftable not found"));
    }
    f1 = ftp->ftable;
    l1 = (int) ftp->flen;

    x = (MYFLT*) calloc(l2 + 2, sizeof(MYFLT));
    y = (MYFLT*) calloc(l1 + 2, sizeof(MYFLT));
    /* read and analyze src table, apply amplitude scale */
    a = csound->GetInverseRealFFTScale(csound, l1) * (MYFLT) l1 / (MYFLT) l2;
    for (i = 0; i < l2; i++)
      x[i] = a * f2[i];
    csound->RealFFT(csound, x, l2);
    x[l2] = x[1];
    x[1] = x[l2 + 1] = FL(0.0);

    for (j = 6; j < (nargs + 3); j++) {
      n = (int) (FL(0.5) + *valp++); if (n < 1) n = 1; /* frequency */
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      a = *valp++;                                     /* amplitude */
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      p = *valp++;                                       /* phase     */
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      p -= (MYFLT) ((int) p); if (p < FL(0.0)) p += FL(1.0); p *= TWOPI_F;
      d_re = cos((double) p); d_im = sin((double) p);
      p_re = 1.0; p_im = 0.0;   /* init. phase */
      for (i = k = 0; (i <= l1 && k <= l2); i += (n << 1), k += 2) {
        /* mix to table */
        y[i + 0] += a * (x[k + 0] * (MYFLT) p_re - x[k + 1] * (MYFLT) p_im);
        y[i + 1] += a * (x[k + 1] * (MYFLT) p_re + x[k + 0] * (MYFLT) p_im);
        /* update phase */
        ptmp = p_re * d_re - p_im * d_im;
        p_im = p_im * d_re + p_re * d_im;
        p_re = ptmp;
      }
    }

    /* write dest. table */
    y[1] = y[l1];
    y[l1] = y[l1 + 1] = FL(0.0);
    csound->InverseRealFFT(csound, y, l1);
    /* memcpy(f1, y, l11*sizeof(MYFLT)); */
    for (i = 0; i < l1; i++)
      f1[i] = y[i];
    f1[l1] = f1[0];     /* write guard point */

    free(x);
    free(y);

    return OK;
}

/* gen32: transpose, phase shift, and mix source tables */

static inline MYFLT paccess(FGDATA *ff, int i)
{
    if (LIKELY(i<PMAX)) return ff->e.p[i];
    else return ff->e.c.extra[i-PMAX+1];
}

static int gen32(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *x, *y, *f1, *f2;
    MYFLT   a, p;
    double  d_re, d_im, p_re, p_im, ptmp;
    int     i, j, k, n, l1, l2, ntabl, *pnum, ft;
    int     nargs = ff->e.pcnt - 4;

    if (ff->e.pcnt>=PMAX) {
      csound->Warning(csound, Str("using extended arguments\n"));
    }
    if (UNLIKELY(nargs < 4)) {
      return fterror(ff, Str("insufficient gen arguments"));
    }

    ntabl = nargs >> 2;         /* number of waves to mix */
    pnum  = (int*) malloc(sizeof(int) * ntabl);
    for (i = 0; i < ntabl; i++)
      pnum[i] = (i << 2) + 5;   /* p-field numbers */
    do {
      i = k = 0;                        /* sort by table number */
      while (i < (ntabl - 1)) {
        if (paccess(ff,pnum[i]) > paccess(ff,pnum[i + 1])) {
          j = pnum[i]; pnum[i] = pnum[i + 1]; pnum[i + 1] = j;
          k = 1;
        }
        i++;
      }
    } while (k);

    f1 = ftp->ftable;
    l1 = (int) ftp->flen;
    memset(f1, 0, l1*sizeof(MYFLT));
    /* for (i = 0; i <= l1; i++) */
    /*   f1[i] = FL(0.0); */
    x = y = NULL;

    ft = 0x7FFFFFFF;            /* last table number */
    j  = -1;                    /* current wave number */

    while (++j < ntabl) {
      p = paccess(ff,pnum[j]);                /* table number */
      i = (int) MYFLT2LRND(p);
      l2 = csoundGetTable(csound, &f2, abs(i));
      if (UNLIKELY(l2 < 0)) {
        fterror(ff, Str("GEN32: source ftable %d not found"), abs(i));
        if (x != NULL) free(x);
        if (y != NULL) free(y);
        free(pnum);
        return NOTOK;
      }
      if (i < 0) {              /* use linear interpolation */
        ft = i;
        p_re  = (double) paccess(ff,pnum[j] + 3);     /* start phase */
        p_re -= (double) ((int) p_re); if (p_re < 0.0) p_re++;
        p_re *= (double) l2;
        d_re  = (double) paccess(ff,pnum[j] + 1);     /* frequency */
        d_re *= (double) l2 / (double) l1;
        a     = paccess(ff,pnum[j] + 2);              /* amplitude */
        for (i = 0; i <= l1; i++) {
          k = (int) p_re; p = (MYFLT) (p_re - (double) k);
          if (k >= l2) k -= l2;
          f1[i] += f2[k++] * a * (FL(1.0) - p);
          f1[i] += f2[k] * a * p;
          p_re += d_re;
          while (p_re < 0.0) p_re += (double) l2;
          while (p_re >= (double) l2) p_re -= (double) l2;
        }
      }
      else {                    /* use FFT */
        if (i != ft) {
          ft = i;               /* new table */
          if (y == NULL)
            y = (MYFLT*) calloc(l1 + 2, sizeof (MYFLT));
          if (x != NULL) free(x);
          x = (MYFLT*) calloc(l2 + 2, sizeof (MYFLT));
          /* read and analyze src table */
          for (i = 0; i < l2; i++)
            x[i] = f2[i];
          csound->RealFFT(csound, x, l2);
          x[l2] = x[1];
          x[1] = x[l2 + 1] = FL(0.0);
        }
        n = (int) (FL(0.5) + paccess(ff,pnum[j] + 1));         /* frequency */
        if (n < 1) n = 1;
        a = paccess(ff,pnum[j] + 2) * (MYFLT) l1 / (MYFLT) l2; /* amplitude */
        a *= csound->GetInverseRealFFTScale(csound, (int) l1);
        p = paccess(ff,pnum[j] + 3);                           /* phase */
        p -= (MYFLT) ((int) p); if (p < FL(0.0)) p += FL(1.0); p *= TWOPI_F;
        d_re = cos ((double) p); d_im = sin ((double) p);
        p_re = 1.0; p_im = 0.0;         /* init. phase */
        for (i = k = 0; (i <= l1 && k <= l2); i += (n << 1), k += 2) {
          /* mix to table */
          y[i + 0] += a * (x[k + 0] * (MYFLT) p_re - x[k + 1] * (MYFLT) p_im);
          y[i + 1] += a * (x[k + 1] * (MYFLT) p_re + x[k + 0] * (MYFLT) p_im);
          /* update phase */
          ptmp = p_re * d_re - p_im * d_im;
          p_im = p_im * d_re + p_re * d_im;
          p_re = ptmp;
        }
      }
    }
    /* write dest. table */
    if (y != NULL) {
      y[1] = y[l1]; y[l1] = y[l1 + 1] = FL(0.0);
      csound->InverseRealFFT(csound, y, l1);
      for (i = 0; i < l1; i++)
        f1[i] += y[i];
      f1[l1] += y[0];           /* write guard point */
      free(x);         /* free tmp memory */
      free(y);
    }
    free(pnum);

    return OK;
}

/* GEN33 by Istvan Varga */

static int gen33(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   fmode, *ft, *srcft, scl, amp, phs;
    MYFLT   *x;
    int     nh, flen, srclen, i, pnum, maxp;
    int     nargs = ff->e.pcnt - 4;

    if (UNLIKELY(nargs < 3)) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    if (nargs > 3)      /* check optional argument */
      fmode = ff->e.p[8];
    else
      fmode = FL(0.0);
    /* table length and data */
    ft = ftp->ftable; flen = (int) ftp->flen;
    /* source table */
    srclen = csoundGetTable(csound, &srcft, (int) ff->e.p[5]);
    if (UNLIKELY(srclen < 0)) {
      return fterror(ff, Str("GEN33: source ftable not found"));
    }
    /* number of partials */
    nh = (int) (ff->e.p[6] + FL(0.5));
    if (nh > (srclen / 3)) nh = srclen / 3;
    if (nh < 0) nh = 0;
    /* amplitude scale */
    scl = FL(0.5) * (MYFLT) flen * ff->e.p[7];
    scl *= csound->GetInverseRealFFTScale(csound, flen);
    /* frequency mode */
    if (fmode < FL(0.0)) {
      fmode = (MYFLT) flen / (csound->esr * -fmode);  /* frequency in Hz */
    }
    else if (fmode > FL(0.0)) {
      fmode = (MYFLT) flen / fmode;             /* ref. sample rate */
    }
    else {
      fmode = FL(1.0);                          /* partial number */
    }

    /* allocate memory for tmp data */
    x = (MYFLT*) calloc(flen + 2, sizeof(MYFLT));

    maxp = flen >> 1;           /* max. partial number */
    i = nh;
    while (i--) {
      /* amplitude */
      amp = scl * *(srcft++);
      /* partial number */
      pnum = (int) MYFLT2LRND(fmode * *srcft);
      srcft++;
      if (pnum < (-maxp) || pnum > maxp) {
        srcft++; continue;      /* skip partial with too high frequency */
      }
      /* initial phase */
      phs = TWOPI_F * *(srcft++);
      if (pnum < 0) {
        phs = PI_F - phs; pnum = -pnum;         /* negative frequency */
      }
      /* mix to FFT data */
      x[pnum << 1] += amp * SIN(phs);
      x[(pnum << 1) + 1] -= amp * COS(phs);
    }

    csound->InverseRealFFT(csound, x, flen);    /* iFFT */


    memcpy(ft, x, flen*sizeof(MYFLT));
    /* for (i = 0; i < flen; i++)  /\* copy to output table *\/ */
    /*   ft[i] = x[i]; */
    ft[flen] = x[0];            /* write guard point */

    /* free tmp memory */
    free(x);

    return OK;
}

/* GEN34 by Istvan Varga */

static int gen34(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   fmode, *ft, *srcft, scl;
    double  y0, y1, x, c, v, *xn, *cn, *vn, *tmp, amp, frq, phs;
    int32    nh, flen, srclen, i, j, k, l, bs;
    FUNC    *src;
    int     nargs = ff->e.pcnt - 4;

    if (UNLIKELY(nargs < 3)) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    if (nargs > 3)      /* check optional argument */
      fmode = ff->e.p[8];
    else
      fmode = FL(0.0);
    /* table length and data */
    ft = ftp->ftable; flen = (int32) ftp->flen;
    /* source table */
    if (UNLIKELY((src = csoundFTFind(csound, &(ff->e.p[5]))) == NULL))
      return NOTOK;
    srcft = src->ftable; srclen = (int32) src->flen;
    /* number of partials */
    nh = (int32) (ff->e.p[6] + FL(0.5));
    if (nh > (srclen / 3L)) nh = srclen / 3L;
    if (nh < 0L) nh = 0L;
    /* amplitude scale */
    scl = ff->e.p[7];
    /* frequency mode */
    if (fmode < FL(0.0)) {
      fmode = TWOPI_F / (csound->esr * -fmode); /* frequency in Hz */
    }
    else if (fmode > FL(0.0)) {
      fmode = TWOPI_F / fmode;          /* ref. sample rate */
    }
    else {
      fmode = TWOPI_F / (MYFLT) flen;   /* partial number */
    }

    /* use blocks of 256 samples (2048 bytes) for speed */
    bs = 256L;
    /* allocate memory for tmp data */
    tmp = (double*) malloc(sizeof (double) * bs);
    xn  = (double*) malloc(sizeof (double) * (nh + 1L));
    cn  = (double*) malloc(sizeof (double) * (nh + 1L));
    vn  = (double*) malloc(sizeof (double) * (nh + 1L));
    /* initialise oscillators */
    i = -1L;
    while (++i < nh) {
      amp = (double) scl * (double) *(srcft++);         /* amplitude */
      frq = (double) fmode * (double) *(srcft++);       /* frequency */
      if (fabs (frq) > PI) {
        xn[i] = cn[i] = vn[i] = 0.0;
        srcft++; continue;      /* skip partial with too high frequency */
      }
      phs = TWOPI * (double) *(srcft++);                /* phase */
      /* calculate coeffs for fast sine oscillator */
      y0 = sin(phs);           /* sample 0 */
      y1 = sin(phs + frq);     /* sample 1 */
      xn[i] = y0;
      cn[i] = 2.0 * cos(frq) - 2.0;
      vn[i] = y1 - cn[i] * y0 - y0;
      /* amp. scale */
      xn[i] *= amp; vn[i] *= amp;
    }

    /* render output */
    j = flen + 1L;      /* include guard point */
    do {
      k = (j > bs ? bs : j);    /* block size */
      /* clear buffer */
      memset(tmp, 0, k*sizeof(double));
      /* for (i = 0L; i < k; i++) tmp[i] = 0.0; */
      /* fast sine oscillator */
      i = -1L;
      while (++i < nh) {
        x = xn[i]; c = cn[i]; v = vn[i];
        l = k;
        do {
          *(tmp++) += x;
          v += c * x;
          x += v;
        } while (--l);
        tmp -= k;               /* restore pointer */
        xn[i] = x; vn[i] = v;   /* save oscillator state */
      }
      /* write to output table */
      for (i = 0L; i < k; i++) *(ft++) = (MYFLT) tmp[i];
      j -= k;
    } while (j);

    /* free tmp buffers */
    free(tmp); free(xn); free(cn); free(vn);

    return OK;
}

static int gen40(FGDATA *ff, FUNC *ftp)               /*gab d5*/
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp = ftp->ftable, *fp_source, *fp_temp;
    FUNC    *srcftp;
    int     srcno, srcpts, j, k;
    MYFLT   last_value = FL(0.0), lenratio;

    if (UNLIKELY((srcno = (int) ff->e.p[5]) <= 0 ||
        srcno > csound->maxfnum         ||
                 (srcftp = csound->flist[srcno]) == NULL)) {
      return fterror(ff, Str("unknown source table number"));
    }
    fp_source = srcftp->ftable;
    srcpts = srcftp->flen;
    fp_temp = (MYFLT *) calloc(srcpts, sizeof(MYFLT));
    for (j = 0; j < srcpts; j++) {
      last_value += fp_source[j];
      fp_temp[j] = last_value;
    }
    lenratio = (ff->flen-1)/last_value;

    for (j = 0; j < ff->flen; j++) {
      k=0;
      while ( k++ < srcpts && fp_temp[k] * lenratio < j) ;
      k--;
      fp[j] = (MYFLT) k;
    }
    fp[j] = fp[j-1];
    free(fp_temp);

    return OK;
}

static int gen41(FGDATA *ff, FUNC *ftp)   /*gab d5*/
{
    MYFLT   *fp = ftp->ftable, *pp = &ff->e.p[5];
    int     j, k, width;
    MYFLT    tot_prob = FL(0.0);
    int     nargs = ff->e.pcnt - 4;

    for (j=0; j < nargs; j+=2) {
      if (pp[j+1]<0)
        return fterror(ff, Str("Gen41: negative probability not allowed"));
      tot_prob += pp[j+1];
    }
    for (j=0; j< nargs; j+=2) {
      width = (int) ((pp[j+1]/tot_prob) * ff->flen +.5);
      for ( k=0; k < width; k++) {
        *fp++ = pp[j];
      }
    }
    *fp = pp[j-1];

    return OK;
}

static int gen42(FGDATA *ff, FUNC *ftp) /*gab d5*/
{
    MYFLT   *fp = ftp->ftable, inc;
    int     j, k, width;
    MYFLT    tot_prob = FL(0.0);
    int     nargs = ff->e.pcnt - 4;
    CSOUND  *csound = ff->csound;
    int nsw = 1;
    MYFLT   *valp = &ff->e.p[5];

    if (ff->e.pcnt>=PMAX)
      csound->Warning(csound, Str("using extended arguments\n"));
    for (j=0; j < nargs; j+=3) {
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      tot_prob += *valp++;
    }
    nsw = 1; valp = &ff->e.p[5];
    for (j=0; j< nargs; j+=3) {
      MYFLT p1, p2, p3;
      p1 = *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      p2 = *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      p3 = *valp++;
      if (UNLIKELY(nsw && valp>=&ff->e.p[PMAX-1]))
        nsw =0, valp = &(ff->e.c.extra[1]);
      width = (int) ((p3/tot_prob) * ff->flen +FL(0.5));
      inc = (p2-p1) / (MYFLT) (width-1);
      for ( k=0; k < width; k++) {
        *fp++ = p1+(inc*k);
      }
    }
    *fp = *(fp-1);

    return OK;
}

static CS_NOINLINE int fterror(const FGDATA *ff, const char *s, ...)
{
    CSOUND  *csound = ff->csound;
    char    buf[64];
    va_list args;

    snprintf(buf, 64, Str("ftable %d: "), ff->fno);
    va_start(args, s);
    csound->ErrMsgV(csound, buf, s, args);
    va_end(args);
    csoundMessage(csound, "f%3.0f %8.2f %8.2f ",
                            ff->e.p[1], ff->e.p2orig, ff->e.p3orig);
    if (ISSTRCOD(ff->e.p[4]))
      csoundMessage(csound,"%s", ff->e.strarg);
    else
      csoundMessage(csound, "%8.2f", ff->e.p[4]);
    if (ISSTRCOD(ff->e.p[5]))
      csoundMessage(csound, "  \"%s\" ...\n", ff->e.strarg);
    else
      csoundMessage(csound, "%8.2f ...\n", ff->e.p[5]);

    return -1;
}

/* set guardpt, rescale the function, and display it */

static CS_NOINLINE void ftresdisp(const FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp, *finp = &ftp->ftable[ff->flen];
    MYFLT   abs, maxval;
    WINDAT  dwindow;
    char    strmsg[64];

    if (!ff->guardreq)                      /* if no guardpt yet, do it */
      ftp->ftable[ff->flen] = ftp->ftable[0];
    if (ff->e.p[4] > FL(0.0)) {             /* if genum positve, rescale */
      for (fp=ftp->ftable, maxval = FL(0.0); fp<=finp; ) {
        if ((abs = *fp++) < FL(0.0))
          abs = -abs;
        if (abs > maxval)
          maxval = abs;
      }
      if (maxval != FL(0.0) && maxval != FL(1.0))
        for (fp=ftp->ftable; fp<=finp; fp++)
          *fp /= maxval;
    }
    if (!csound->oparms->displays)
      return;
    memset(&dwindow, 0, sizeof(WINDAT));
    snprintf(strmsg, 64, Str("ftable %d:"), (int) ff->fno);
    dispset(csound, &dwindow, ftp->ftable, (int32) (ff->flen),
                    strmsg, 0, "ftable");
    display(csound, &dwindow);
}

static void generate_sine_tab(CSOUND *csound)
{                               /* Assume power of 2 length */
    int flen = csound->sinelength;
    FUNC    *ftp = (FUNC*) csound->Calloc(csound, sizeof(FUNC));
    ftp->ftable = (MYFLT*) csound->Calloc(csound, sizeof(MYFLT)*(flen+1));
    double  tpdlen = TWOPI / (double) flen;
    MYFLT *ftable = ftp->ftable;
    unsigned int i;
    int ltest, lobits;
    for (ltest = flen, lobits = 0;
           (ltest & MAXLEN) == 0L;
           lobits++, ltest <<= 1)
        ;
    ftp->lobits   = lobits;
    i = (1 << lobits);
    ftp->lomask   = (int32) (i - 1);
    ftp->lodiv    = FL(1.0) / (MYFLT) i;        /*    & other useful vals   */
    ftp->flen = ftp->flenfrms = flen;
    ftp->fno = -1;
    ftp->lenmask = flen - 1;
    ftp->nchanls = 1;
    for (i = 1; i<ftp->flen; i++)
      ftable[i] = (MYFLT) sin(i*tpdlen);
    ftable[0] = ftable[ftp->flen] = FL(0.0);
    csound->sinetable = ftp;
    return;
}

/* alloc ftable space for fno (or replace one) */
/*  set ftp to point to that structure         */

static CS_NOINLINE FUNC *ftalloc(const FGDATA *ff)
{
    CSOUND  *csound = ff->csound;
    FUNC    *ftp = csound->flist[ff->fno];

    if (UNLIKELY(ftp != NULL)) {
      csound->Warning(csound, Str("replacing previous ftable %d"), ff->fno);
      if (ff->flen != (int32)ftp->flen) {       /* if redraw & diff len, */
        csound->Free(csound, ftp->ftable);
        csound->Free(csound, (void*) ftp);             /*   release old space   */
        csound->flist[ff->fno] = ftp = NULL;
        if (csound->actanchor.nxtact != NULL) { /*   & chk for danger    */
          csound->Warning(csound, Str("ftable %d relocating due to size change"
                                      "\n         currently active instruments "
                                      "may find this disturbing"), ff->fno);
        }
      }
      else {
                                    /* else clear it to zero */
        MYFLT *tmp = ftp->ftable;
        memset((void*) ftp->ftable, 0, sizeof(MYFLT)*(ff->flen+1));
        memset((void*) ftp, 0, sizeof(FUNC));
        ftp->ftable = tmp; /* restore table pointer */
      }
    }
    if (ftp == NULL) {                      /*   alloc space as reqd */
      csound->flist[ff->fno] = ftp = (FUNC*) csound->Calloc(csound, sizeof(FUNC));
      ftp->ftable = (MYFLT*) csound->Calloc(csound, (1+ff->flen) * sizeof(MYFLT));
    }
    ftp->fno = (int32) ff->fno;
    ftp->flen = ff->flen;
    return ftp;
}

/* find the ptr to an existing ftable structure */
/*   called by oscils, etc at init time         */

FUNC *csoundFTFind(CSOUND *csound, MYFLT *argp)
{
    FUNC    *ftp;
    int     fno;

    fno = MYFLT2LONG(*argp);
    if (UNLIKELY(fno == -1)) {
      if (UNLIKELY(csound->sinetable==NULL)) generate_sine_tab(csound);
      return csound->sinetable;
    }
    if (UNLIKELY(fno <= 0                 ||
        fno > csound->maxfnum       ||
                 (ftp = csound->flist[fno]) == NULL)) {
      csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    else if (UNLIKELY(ftp->lenmask == -1)) {
      csoundInitError(csound, Str("illegal table length"));
      return NULL;
    }
    else if (UNLIKELY(!ftp->lenmask)) {
      csoundInitError(csound,
                      Str("deferred-size ftable %f illegal here"), *argp);
      return NULL;
    }
    return ftp;
}

/* find the ptr to an existing ftable structure */
/*   called by oscils, etc at init time         */
/* does not throw an error when a non-pow of two size table is found */

FUNC *csoundFTFind2(CSOUND *csound, MYFLT *argp)
{
    FUNC    *ftp;
    int     fno;

    fno = MYFLT2LONG(*argp);
    if (UNLIKELY(fno == -1)) {
      if (UNLIKELY(csound->sinetable==NULL)) generate_sine_tab(csound);
      return csound->sinetable;
    }
    if (UNLIKELY(fno <= 0           ||
        fno > csound->maxfnum       ||
                 (ftp = csound->flist[fno]) == NULL)) {
      return NULL;
    }
    else if (UNLIKELY(ftp->lenmask == -1)) {
      return NULL;
    }
    else if (UNLIKELY(!ftp->lenmask)) {
      return NULL;
    }
    return ftp;
}

/* **** SOMETHING WRONG HERE __ NOT CALLED **** */
static CS_NOINLINE FUNC *gen01_defer_load(CSOUND *csound, int fno)
{
    FGDATA  ff;
    char    strarg[SSTRSIZ];
    FUNC    *ftp = csound->flist[fno];

    /* The soundfile hasn't been loaded yet, so call GEN01 */
    strcpy(strarg, ftp->gen01args.strarg);
    memset(&ff, 0, sizeof(FGDATA));
    ff.csound = csound;
    ff.fno = fno;
    ff.e.strarg = strarg;
    ff.e.opcod = 'f';
    ff.e.pcnt = 8;
    ff.e.p[1] = (MYFLT) fno;
    ff.e.p[4] = ftp->gen01args.gen01;
    ff.e.p[5] = ftp->gen01args.ifilno;
    ff.e.p[6] = ftp->gen01args.iskptim;
    ff.e.p[7] = ftp->gen01args.iformat;
    ff.e.p[8] = ftp->gen01args.channel;
    if (UNLIKELY(gen01raw(&ff, ftp) != 0)) {
      csoundErrorMsg(csound, Str("Deferred load of '%s' failed"), strarg);
      return NULL;
    }
    return csound->flist[fno];
}

PUBLIC int csoundGetTable(CSOUND *csound, MYFLT **tablePtr, int tableNum)
{
    FUNC    *ftp;

    if (UNLIKELY((unsigned int) (tableNum - 1) >= (unsigned int) csound->maxfnum))
      goto err_return;
    ftp = csound->flist[tableNum];
    if (UNLIKELY(ftp == NULL))
      goto err_return;
    if (!ftp->flen) {
      ftp = gen01_defer_load(csound, tableNum);
      if (UNLIKELY(ftp == NULL))
        goto err_return;
    }
    *tablePtr = ftp->ftable;
    return (int) ftp->flen;

 err_return:
    *tablePtr = (MYFLT*) NULL;
    return -1;
}

PUBLIC int csoundGetTableArgs(CSOUND *csound, MYFLT **argsPtr, int tableNum)
{
    FUNC    *ftp;
    if (UNLIKELY((unsigned int) (tableNum - 1) >= (unsigned int) csound->maxfnum))
      goto err_return;
    ftp = csound->flist[tableNum];
    if (UNLIKELY(ftp == NULL))
      goto err_return;
    *argsPtr = ftp->args;
    return (int) ftp->argcnt;

 err_return:
    *argsPtr = (MYFLT*) NULL;
    return -1;
}

/**************************************
 * csoundFTFindP()
 *
 * New function to find a function table at performance time.  Based
 * on csoundFTFind() which is intended to run at init time only.
 *
 * This function can be called from other modules - such as ugrw1.c.
 *
 * It returns a pointer to a FUNC data structure which contains all
 * the details of the desired table.  0 is returned if it cannot be
 * found.
 *
 * This does not handle deferred function table loads (gen01).
 *
 * Maybe this could be achieved, but some exploration would be
 * required to see that this is feasible at performance time.
 */
FUNC *csoundFTFindP(CSOUND *csound, MYFLT *argp)
{
    FUNC    *ftp;
    int     fno;

    /* Check limits, and then index  directly into the flist[] which
     * contains pointers to FUNC data structures for each table.
     */
    fno = MYFLT2LONG(*argp);
    if (UNLIKELY(fno == -1)) {
      if (UNLIKELY(csound->sinetable==NULL)) generate_sine_tab(csound);
      return csound->sinetable;
    }
    if (UNLIKELY(fno <= 0                 ||
                 fno > csound->maxfnum    ||
                 (ftp = csound->flist[fno]) == NULL)) {
      csound->ErrorMsg(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    else if (UNLIKELY(!ftp->lenmask)) {
      /* Now check that the table has a length > 0.  This should only
       * occur for tables which have not been loaded yet.  */
      csound->ErrorMsg(csound, Str("Deferred-size ftable %f load "
                                  "not available at perf time."), *argp);
      return NULL;
    }
    return ftp;
}

/* find ptr to a deferred-size ftable structure */
/*   called by loscil at init time, and ftlen   */

FUNC *csoundFTnp2Find(CSOUND *csound, MYFLT *argp)
{
    FUNC    *ftp;
    int     fno = MYFLT2LONG(*argp);

    if (UNLIKELY(fno == -1)) {
      if (UNLIKELY(csound->sinetable==NULL)) generate_sine_tab(csound);
      return csound->sinetable;
    }
    if (UNLIKELY(fno <= 0 ||
                 fno > csound->maxfnum    ||
                 (ftp = csound->flist[fno]) == NULL)) {
      csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    if (ftp->flen == 0) {
      if (LIKELY(csound->oparms->gen01defer))
       ftp = gen01_defer_load(csound, fno);
      else {
        csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
        return NULL;
      }
      if (UNLIKELY(ftp == NULL))
        csound->inerrcnt++;
    }
    return ftp;
}

/* read ftable values from a sound file */
/* stops reading when table is full     */

static int gen01(FGDATA *ff, FUNC *ftp)
{
    if (UNLIKELY(ff->e.pcnt < 8)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if (ff->csound->oparms->gen01defer) {
      /* We're deferring the soundfile load until performance time,
         so allocate the function table descriptor, save the arguments,
         and get out */
      ftp = ftalloc(ff);
      ftp->gen01args.gen01 = ff->e.p[4];
      ftp->gen01args.ifilno = ff->e.p[5];
      ftp->gen01args.iskptim = ff->e.p[6];
      ftp->gen01args.iformat = ff->e.p[7];
      ftp->gen01args.channel = ff->e.p[8];
      strncpy(ftp->gen01args.strarg, ff->e.strarg, SSTRSIZ-1);
      return OK;
    }
    return gen01raw(ff, ftp);
}

static void needsiz(CSOUND *csound, FGDATA *ff, int32 maxend)
{
    int32 nxtpow;
    maxend -= 1; nxtpow = 2;
    while (maxend >>= 1)
      nxtpow <<= 1;
    csoundMessage(csound, Str("non-deferred ftable %d needs size %d\n"),
                            (int) ff->fno, nxtpow);
}

static const int gen01_format_table[11] = {
    0,
    0,          AE_CHAR,    AE_ALAW,    AE_ULAW,    AE_SHORT,
    AE_LONG,    AE_FLOAT,   AE_UNCH,    AE_24INT,   AE_DOUBLE
};

/* read ftable values from a sound file */
/* stops reading when table is full     */

static int gen01raw(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    SOUNDIN *p;
    SOUNDIN tmpspace;
    SNDFILE *fd;
    int     truncmsg = 0;
    int32   inlocs = 0;
    int     def = 0, table_length = ff->flen + 1;

    p = &tmpspace;
    memset(p, 0, sizeof(SOUNDIN));
    {
      int32 filno = (int32) MYFLT2LRND(ff->e.p[5]);
      int   fmt = (int) MYFLT2LRND(ff->e.p[7]);
      if (ISSTRCOD(ff->e.p[5])) {
        if (ff->e.strarg[0] == '"') {
          int len = (int) strlen(ff->e.strarg) - 2;
          strncpy(p->sfname, ff->e.strarg + 1, 512);
          if (len >= 0 && p->sfname[len] == '"')
            p->sfname[len] = '\0';
        }
        else
          strncpy(p->sfname, ff->e.strarg, 512);
      }
      else if (filno >= 0 && filno <= csound->strsmax &&
               csound->strsets && csound->strsets[filno])
        strncpy(p->sfname, csound->strsets[filno], 512);
      else
        snprintf(p->sfname, 512, "soundin.%d", filno);   /* soundin.filno */
      if (!fmt)
        p->format = csound->oparms->outformat;
      else {
        if (UNLIKELY(fmt < -1 || fmt > 9))
          return fterror(ff, Str("invalid sample format: %d"), fmt);
        p->format = gen01_format_table[fmt + 1];
      }
    }
    p->skiptime = ff->e.p[6];
    p->channel  = (int) MYFLT2LRND(ff->e.p[8]);
    p->do_floatscaling = 0;
    if (UNLIKELY(p->channel < 0 /* || p->channel > ALLCHNLS-1 */)) {
      return fterror(ff, Str("channel %d illegal"), (int) p->channel);
    }
    if (p->channel == 0)                      /* snd is chan 1,2,..8 or all */
      p->channel = ALLCHNLS;
    p->analonly = 0;
    if (UNLIKELY(ff->flen == 0 && (csound->oparms->msglevel & 7))){
      csoundMessage(csound, Str("deferred alloc\n"));
    }
    if (UNLIKELY((fd = sndgetset(csound, p))==NULL)) {
      /* sndinset to open the file  */
      return fterror(ff, "Failed to open file");
    }
    if (ff->flen == 0) {                      /* deferred ftalloc requestd: */
      if (UNLIKELY((ff->flen = p->framesrem + 1) <= 0)) {
        /*   get minsize from soundin */
        return fterror(ff, Str("deferred size, but filesize unknown"));
      }
      if (csound->oparms->msglevel & 7)
        csoundMessage(csound, Str("  defer length %d\n"), ff->flen - 1);
       if (p->channel == ALLCHNLS)
         ff->flen *= p->nchanls;
      ff->guardreq  = 1;                      /* presum this includes guard */
/*ff->flen     -= 1;*/ /* VL: this was causing tables to exclude last point  */
      ftp           = ftalloc(ff);            /*   alloc now, and           */
      ftp->lenmask  = 0L;                     /*   mark hdr partly filled   */
      /*if (p->channel==ALLCHNLS) ftp->nchanls  = p->nchanls;
      else ftp->nchanls  = 1;
      ftp->flenfrms = ff->flen / p->nchanls; */ /* ?????????? */
      def           = 1;
      ff->flen -= 1;
      table_length = ff->flen;
    }
    if (p->channel==ALLCHNLS) {
    //ff->flen *= p->nchanls;
    ftp->nchanls  = p->nchanls;
    }
      else ftp->nchanls  = 1;
    ftp->flenfrms = ff->flen / p->nchanls;  /* ?????????? */
    ftp->gen01args.sample_rate = (MYFLT) p->sr;
    ftp->cvtbas = LOFACT * p->sr * csound->onedsr;
    {
      SF_INSTRUMENT lpd;
      int ans = sf_command(fd, SFC_GET_INSTRUMENT, &lpd, sizeof(SF_INSTRUMENT));
      if (ans) {
        double natcps;
#ifdef BETA
        if ((csound->oparms_.msglevel & 7) == 7) {
          csoundMessage(csound,
                  "Base Note : %u\tDetune    : %u\n"
                  "Low  Note : %u\tHigh Note : %u\n"
                  "Low  Vel. : %u\tHigh Vel. : %u\n"
                  "Gain      : %d\tCount     : %d\n"
                  "mode      : %d\n"
                  "start     : %d\tend       : %d\tcount  :%d\n"
                  "mode      : %d\n"
                  "start     : %d\tend       : %d\tcount  :%d\n\n",
                  lpd.basenote, 0U, lpd.key_lo, lpd.key_hi,
                  lpd.velocity_lo, lpd.velocity_hi, lpd.gain, lpd.loop_count,
                  lpd.loops[0].mode, lpd.loops[0].start, lpd.loops[0].end,
                  lpd.loops[0].count, lpd.loops[1].mode, lpd.loops[1].start,
                  lpd.loops[1].end, lpd.loops[1].count);
        }
#endif
        natcps = pow(2.0, ((double) ((int) lpd.basenote - 69)
                           + (double) lpd.detune * 0.01) / 12.0) * 440.0;
        /* As far as I can tell this gainfac value is never used! */
        //gainfac = exp((double) lpd.gain * LOG10D20);
     /* if (lpd.basenote == 0)
          lpd.basenote = ftp->cvtbas; */
        ftp->cpscvt = ftp->cvtbas / natcps;
        ftp->loopmode1 = (lpd.loops[0].mode == SF_LOOP_NONE ? 0 :
                          lpd.loops[0].mode == SF_LOOP_FORWARD ? 1 :
                          2);
        ftp->loopmode2 = (lpd.loops[1].mode == SF_LOOP_NONE ? 0 :
                          lpd.loops[1].mode == SF_LOOP_FORWARD ? 1 :
                          2);
        ftp->begin1 = lpd.loops[0].start;
        ftp->begin2 = lpd.loops[1].start;
        if (ftp->loopmode1)             /* Greg Sullivan */
          ftp->end1 = lpd.loops[0].end;
        else
          ftp->end1 = ftp->flenfrms;    /* Greg Sullivan */
        ftp->end2 = lpd.loops[1].end;
        if (ftp->end1 > ff->flen || ftp->end2 > ff->flen) {
          int32 maxend;
          csound->Warning(csound,
                          Str("GEN1: input file truncated by ftable size"));
          if ((maxend = ftp->end1) < ftp->end2)
            maxend = ftp->end2;
          csoundMessage(csound,
                          Str("\tlooping endpoint %d exceeds ftsize %d\n"),
                          maxend, ff->flen);
          needsiz(csound, ff, maxend);
          truncmsg = 1;
        }
      }
      else {
        ftp->cpscvt = FL(0.0);          /* else no looping possible   */
        ftp->loopmode1 = 0;
        ftp->loopmode2 = 0;
        ftp->end1 = ftp->flenfrms;      /* Greg Sullivan */
      }
    }
    /* read sound with opt gain */

    if (UNLIKELY((inlocs=getsndin(csound, fd, ftp->ftable, table_length, p)) < 0)) {
      return fterror(ff, Str("GEN1 read error"));
    }

    if (p->audrem > 0 && !truncmsg && p->framesrem > ff->flen) {
      /* Reduce msg */
      csound->Warning(csound, Str("GEN1: aiff file truncated by ftable size"));
      csound->Warning(csound, Str("\taudio samps %d exceeds ftsize %d"),
                              (int32) p->framesrem, (int32) ff->flen);
      needsiz(csound, ff, p->framesrem);     /* ????????????  */
    }
    ftp->soundend = inlocs / ftp->nchanls;   /* record end of sound samps */
    csound->FileClose(csound, p->fd);
    if (def) {
      MYFLT *tab = ftp->ftable;
      ftresdisp(ff, ftp);       /* VL: 11.01.05  for deferred alloc tables */
      tab[ff->flen] = tab[0];  /* guard point */
      ftp->flen -= 1;  /* exclude guard point */
    }
    /* save arguments */
    ftp->argcnt = ff->e.pcnt - 3;
    {  /* Note this does not handle extened args -- JPff */
      int size=ftp->argcnt;
      if (size>PMAX) size=PMAX;
      memcpy(ftp->args, &(ff->e.p[4]), sizeof(MYFLT)*size);
      /* for(k=0; k < size; k++)
         csound->Message(csound, "%f \n", ftp->args[k]);*/
    }
    return OK;
}

/* GEN 43 (c) Victor Lazzarini, 2004 */

typedef struct _pvstabledat {
    int32    fftsize;
    int32    overlap;
    int32    winsize;
    int     wintype;
    int     chans;
    int32    format;
    int32    blockalign;
    uint32 frames;
} PVSTABLEDAT;

static int gen43(FGDATA *ff, FUNC *ftp)
{
    CSOUND          *csound = ff->csound;
    MYFLT           *fp = ftp->ftable;
    MYFLT           *filno;
    int             nvals = ff->e.pcnt - 4;
    MYFLT           *channel;
    char            filename[MAXNAME];
    PVOCEX_MEMFILE  pp;
    PVSTABLEDAT     p;
    uint32          framesize, blockalign, bins;
    uint32          frames, i, j;
    float           *framep, *startp;
    double          accum = 0.0;

    if (UNLIKELY(nvals != 2)) {
      return fterror(ff, Str("wrong number of ftable arguments"));
    }

    filno = &ff->e.p[5];
    if (ISSTRCOD(ff->e.p[5]))
      strncpy(filename, (char *)(&ff->e.strarg[0]), MAXNAME-1);
    else
      csound->strarg2name(csound, filename, filno, "pvoc.", 0);

    if (UNLIKELY(PVOCEX_LoadFile(csound, filename, &pp) != 0))
      csoundDie(csound, Str("Failed to load PVOC-EX file"));
    p.fftsize  = pp.fftsize;
    p.overlap  = pp.overlap;
    p.winsize  = pp.winsize;
    p.wintype  = pp.wintype;
    p.chans    = pp.chans;
    p.format   = pp.format;
    p.frames   = pp.nframes;

    channel = &ff->e.p[6];
    if (UNLIKELY(*channel > p.chans))
      return fterror(ff, Str("illegal channel number"));

    framesize = p.fftsize+1;
    bins = framesize/2;
    frames = p.frames;

    if (*channel > 0) {
      startp = (float *) pp.data + (p.fftsize + 2) * ((int) *channel - 1);
      blockalign = (p.fftsize+2) * p.chans; /* only read one channel */
    }
    else {
      startp = (float *) pp.data;
      blockalign = (p.fftsize+2);           /* read all channels */
    }

    framep = startp;

    if (UNLIKELY(bins > (uint32) (ftp->flen+1))) {
      return fterror(ff, Str("ftable size too small"));
    }

    for (i=0; i<framesize; i+=2) {
      for (j=0; j < frames; j++, framep += blockalign) {
        accum += framep[i];
      }
      fp[i/2] = (MYFLT)accum/frames;
      framep = startp;
      accum = 0.0;
    }
    return OK;
}
#ifndef NACL
#include "mp3dec.h"

static int gen49raw(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound        = ff->csound;
    MYFLT   *fp           = ftp == NULL ? NULL: ftp->ftable;
    mp3dec_t mpa           = NULL;
    mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_AUTO,
                               MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                               MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                               0.0 };
    int     skip              = 0, chan = 0, r, fd;
    int p                     = 0;
    char    sfname[1024];
    mpadec_info_t mpainfo;
    uint32_t bufsize, bufused = 0;
    uint8_t *buffer;
    int size = 0x1000;
    int flen, nchanls, def = 0;

    if (UNLIKELY(ff->e.pcnt < 7)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    /* memset(&mpainfo, 0, sizeof(mpadec_info_t)); */ /* Is this necessary? */
    {
      int32 filno = (int32) MYFLT2LRND(ff->e.p[5]);
      if (ISSTRCOD(ff->e.p[5])) {
        if (ff->e.strarg[0] == '"') {
          int len = (int) strlen(ff->e.strarg) - 2;
          strncpy(sfname, ff->e.strarg + 1, 1023);
          if (len >= 0 && sfname[len] == '"')
            sfname[len] = '\0';
        }
        else
          strncpy(sfname, ff->e.strarg, 1023);
      }
      else if ((filno= (int32) MYFLT2LRND(ff->e.p[5])) >= 0 &&
               filno <= csound->strsmax &&
               csound->strsets && csound->strsets[filno])
        strncpy(sfname, csound->strsets[filno], 1023);
      else
        snprintf(sfname, 1024, "soundin.%d", filno);   /* soundin.filno */
    }
    chan  = (int) MYFLT2LRND(ff->e.p[7]);
    if (UNLIKELY(chan < 0)) {
      return fterror(ff, Str("channel %d illegal"), (int) chan);
    }
    switch (chan) {
    case 0:
      config.mode = MPADEC_CONFIG_AUTO; break;
    case 1:
      config.mode = MPADEC_CONFIG_MONO; break;
    case 2:
      config.mode = MPADEC_CONFIG_STEREO; break;
    case 3:
      config.mode = MPADEC_CONFIG_CHANNEL1; break;
    case 4:
      config.mode = MPADEC_CONFIG_CHANNEL2; break;
    }
    mpa = mp3dec_init();
    if (!mpa) {
      return fterror(ff, Str("Not enough memory\n"));
    }
    if ((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK) {
      mp3dec_uninit(mpa);
      return fterror(ff, mp3dec_error(r));
    }
    (void)csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                     sfname, NULL, "SFDIR;SSDIR",
                                     CSFTYPE_UNKNOWN_AUDIO, 0);
    //    fd = open(sfname, O_RDONLY); /* search paths */
    if (fd < 0) {
      mp3dec_uninit(mpa);
      fterror(ff, "sfname");
    }
    if ((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK) {
      mp3dec_uninit(mpa);
      return fterror(ff, mp3dec_error(r));
    }
    if ((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
        MP3DEC_RETCODE_OK) {
      mp3dec_uninit(mpa);
      return fterror(ff, mp3dec_error(r));
    }
    /* maxsize = mpainfo.decoded_sample_size */
    /*   *mpainfo.decoded_frame_samples */
    /*   *mpainfo.frames; */
    {
      char temp[80];
      if (mpainfo.frequency < 16000) strcpy(temp, "MPEG-2.5 ");
      else if (mpainfo.frequency < 32000) strcpy(temp, "MPEG-2 ");
      else strcpy(temp, "MPEG-1 ");
      if (mpainfo.layer == 1) strcat(temp, "Layer I");
      else if (mpainfo.layer == 2) strcat(temp, "Layer II");
      else strcat(temp, "Layer III");
      csound->DebugMsg(csound, "Input:  %s, %s, %d kbps, %d Hz  (%d:%02d)\n",
              temp, ((mpainfo.channels > 1) ? "stereo" : "mono"),
              mpainfo.bitrate, mpainfo.frequency, mpainfo.duration/60,
              mpainfo.duration%60);
    }
    buffer = (uint8_t *)malloc(size);
    bufsize = size/mpainfo.decoded_sample_size;
    skip = (int)(ff->e.p[6] * mpainfo.frequency);
    while (skip > 0) {
      uint32_t xx = skip;
      if ((uint32_t)xx > bufsize) xx = bufsize;
      //      printf("gen49: skipping xx\n", xx);
      skip -=xx;
      mp3dec_decode(mpa, buffer, mpainfo.decoded_sample_size*xx, &bufused);
    }
    //bufsize *= mpainfo.decoded_sample_size;
    r = mp3dec_decode(mpa, buffer, size, &bufused);
    nchanls = (chan == 2 && mpainfo.channels == 2 ? 2 : 1);
    if (ff->flen == 0) {    /* deferred ftalloc */
      int fsize, frames;
      frames = mpainfo.frames * mpainfo.decoded_frame_samples;
      fsize  = frames * nchanls;
      if (UNLIKELY((ff->flen = fsize) <= 0))
        return fterror(ff, Str("deferred size, but filesize unknown"));
      if (UNLIKELY(ff->flen > MAXLEN))
        return fterror(ff, Str("illegal table length"));
      if (csound->oparms->msglevel & 7)
        csoundMessage(csound, Str("  defer length %d\n"), ff->flen);
      ftp = ftalloc(ff);
      ftp->lenmask  = 0L;
      ftp->flenfrms = frames;
      ftp->nchanls  = nchanls;
      fp = ftp->ftable;
      def = 1;
    }
    ftp->gen01args.sample_rate = mpainfo.frequency;
    ftp->cvtbas = LOFACT * mpainfo.frequency * csound->onedsr;
    flen = ftp->flen;
    //printf("gen49: flen=%d size=%d bufsize=%d\n", flen, size, bufsize);
    while ((r == MP3DEC_RETCODE_OK) && bufused) {
      unsigned int i;
      short *bb = (short*)buffer;
      //printf("gen49: p=%d bufused=%d\n", p, bufused);
      for (i=0; i<bufused*nchanls/mpainfo.decoded_sample_size; i++)  {
        if (p>=flen) {
          free(buffer);
          //printf("gen49: i=%d p=%d exit as at end of table\n", i, p);
          return ((mp3dec_uninit(mpa) == MP3DEC_RETCODE_OK) ? OK : NOTOK);
        }
        fp[p] = ((MYFLT)bb[i]/(MYFLT)0x7fff) * csound->e0dbfs;
        //printf("%d: %f %d\n", p, fp[p], bb[i]);
        p++;
       }
      if (i <= 0) break;
      //printf("gen49: new buffer\n");
      r = mp3dec_decode(mpa, buffer, size, &bufused);
    }

    free(buffer);
    r |= mp3dec_uninit(mpa);
    if (def) ftresdisp(ff, ftp);
    return ((r == MP3DEC_RETCODE_OK) ? OK : NOTOK);
}

static int gen49(FGDATA *ff, FUNC *ftp)
{
    if (UNLIKELY(ff->e.pcnt < 7)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if (ff->csound->oparms->gen01defer) {
      /* We're deferring the soundfile load until performance time,
         so allocate the function table descriptor, save the arguments,
         and get out */
      ftp = ftalloc(ff);
      ftp->gen01args.gen01 = ff->e.p[4];
      ftp->gen01args.ifilno = ff->e.p[5];
      ftp->gen01args.iskptim = ff->e.p[6];
      ftp->gen01args.iformat = ff->e.p[7];
      ftp->gen01args.channel = ff->e.p[8];
      strncpy(ftp->gen01args.strarg, ff->e.strarg, SSTRSIZ-1);
      return OK;
    }
    return gen49raw(ff, ftp);
}
#endif

#if 0
static CS_NOINLINE FUNC *gen49_defer_load(CSOUND *csound, int fno)
{
    FGDATA  ff;
    char    strarg[SSTRSIZ];
    FUNC    *ftp = csound->flist[fno];

    /* The soundfile hasn't been loaded yet, so call GEN49 */
    strcpy(strarg, ftp->gen01args.strarg);
    memset(&ff, 0, sizeof(FGDATA));
    ff.fno = fno;
    ff.e.strarg = strarg;
    ff.e.opcod = 'f';
    ff.e.pcnt = 8;
    ff.e.p[1] = (MYFLT) fno;
    ff.e.p[4] = ftp->gen01args.gen01;
    ff.e.p[5] = ftp->gen01args.ifilno;
    ff.e.p[6] = ftp->gen01args.iskptim;
    ff.e.p[7] = ftp->gen01args.iformat;
    ff.e.p[8] = ftp->gen01args.channel;
    if (UNLIKELY(gen49raw(&ff, ftp) != 0)) {
      csoundErrorMsg(csound, Str("Deferred load of '%s' failed"), strarg);
      return NULL;
    }
    return csound->flist[fno];
}
#endif

static int gen51(FGDATA *ff, FUNC *ftp)    /* Gab 1/3/2005 */
{
    int     j, notenum, grade, numgrades, basekeymidi, nvals;
    MYFLT   basefreq, factor, interval;
    MYFLT   *fp = ftp->ftable, *pp;
    CSOUND  *csound = ff->csound;

    if (ff->e.pcnt>=PMAX) {
      csound->Warning
        (csound, Str("using extended arguments\n"));
    }
    nvals       = ff->flen;
    pp          = &(ff->e.p[5]);
    numgrades   = (int) *pp++;
    interval    = *pp++;
    basefreq    = *pp++;
    basekeymidi = (int) *pp++;
    if (UNLIKELY((ff->e.pcnt - 8) < numgrades)) { /* gab fixed */
      return fterror(ff,
                     Str("gen51: invalid number of p-fields (too few grades)"));
    }

    for (j = 0; j < nvals; j++) {
      MYFLT x;
      notenum = j;
      if (notenum < basekeymidi) {
        notenum = basekeymidi - notenum;
        grade  = (numgrades - (notenum % numgrades)) % numgrades;
        factor = -((MYFLT) ((int) ((notenum + numgrades - 1) / numgrades)));
      }
      else {
        notenum = notenum - basekeymidi;
        grade  = notenum % numgrades;
        factor = (MYFLT) ((int) (notenum / numgrades));
      }
      factor = POWER(interval, factor);
      if (LIKELY(grade<PMAX-10)) x = pp[grade];
      else x = ff->e.c.extra[grade-PMAX+11];
      fp[j] = x * factor * basefreq;
    }
    return OK;
}

static int gen52(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *src, *dst;
    FUNC    *f;
    int     nchn, len, len2, i, j, k, n;
    int     nargs = (int) ff->e.pcnt - 4;

    if (ff->e.pcnt>=PMAX) {
      csound->Warning(csound, Str("using extended arguments\n"));
    }
    if (UNLIKELY(nargs < 4)) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    nchn = MYFLT2LRND(ff->e.p[5]);
    if (UNLIKELY(((nchn * 3) + 1) != nargs)) {
      return fterror(ff, Str("number of channels "
                             "inconsistent with number of args"));
    }
    len = ((int) ftp->flen / nchn) * nchn;
    dst = ftp->ftable;
    memset(dst, 0, ftp->flen*sizeof(MYFLT));
    /* for (i = len; i <= (int) ftp->flen; i++) */
    /*   dst[i] = FL(0.0); */
    for (n = 0; n < nchn; n++) {
      MYFLT *pp;
      if (LIKELY((n * 3) + 6<PMAX-1)) pp = &(ff->e.p[(n * 3) + 6]);
      else pp = &(ff->e.c.extra[(n * 3) + 6-PMAX]);
      f = csoundFTFind(csound, pp);
      if (UNLIKELY(f == NULL))
        return NOTOK;
      len2 = (int) f->flen;
      src = f->ftable;
      i = n;
      if (LIKELY((n * 3) + 7<PMAX-1)) j = MYFLT2LRND(ff->e.p[(n * 3) + 7]);
      else j = MYFLT2LRND(ff->e.c.extra[(n * 3) + 7-PMAX]);
      if (LIKELY((n * 3) + 8<PMAX-1)) k = MYFLT2LRND(ff->e.p[(n * 3) + 8]);
      else k = MYFLT2LRND(ff->e.c.extra[(n * 3) + 8-PMAX]);
      while (i < len) {
        if (j >= 0 && j < len2)
          dst[i] = src[j];
        else
          dst[i] = FL(0.0);
        i += nchn;
        j += k;
      }
    }
    return OK;
}

static void gen53_apply_window(MYFLT *buf, MYFLT *w,
                               int npts, int wpts, int minphase)
{
    int64_t ph, ph_inc;
    int     i, j;
    MYFLT   pfrac;

    for (i = 1, j = 0; i < npts; i <<= 1, j++)
      ;
    if (!minphase) {
      ph = (int64_t) 0;
      ph_inc = ((int64_t) wpts << 32) >> j;
    }
    else {
      ph = (int64_t) wpts << 31;
      ph_inc = ((int64_t) wpts << 31) >> j;
    }
    for (i = 0; i <= npts; i++) {
      j = (int) (ph >> 32);
      pfrac = (MYFLT) ((int) (((uint32_t) ph) >> 1));
      if (j >= wpts) {
        buf[i] *= w[wpts];
      }
      else {
        pfrac *= (FL(0.5) / (MYFLT) 0x40000000);
        buf[i] *= (w[j] + ((w[j + 1] - w[j]) * pfrac));
      }
      ph += ph_inc;
    }
}

static void gen53_freq_response_to_ir(CSOUND *csound,
                                      MYFLT *obuf, MYFLT *ibuf, MYFLT *wbuf,
                                      int npts, int wpts, int mode)
{
    MYFLT   *buf1, *buf2;
    double  tmp;
    MYFLT   scaleFac;
    int     i, j, npts2 = (npts << 1);

    scaleFac = csound->GetInverseRealFFTScale(csound, npts);
    /* ---- linear phase impulse response ---- */
    i = j = 0;
    do {
      obuf[i++] = (FABS(ibuf[j])) * scaleFac; j++;
      obuf[i++] = FL(0.0);
      obuf[i++] = -(FABS(ibuf[j]) * scaleFac); j++;
      obuf[i++] = FL(0.0);
    } while (i < npts);
    obuf[1] = ibuf[j] * scaleFac;
    csound->InverseRealFFT(csound, obuf, npts);
    obuf[npts] = FL(0.0);               /* clear guard point */
    if (wbuf != NULL && !(mode & 4))    /* apply window if requested */
      gen53_apply_window(obuf, wbuf, npts, wpts, 0);
    if (!(mode & 1))
      return;
    /* ---- minimum phase impulse response ---- */
    scaleFac = csound->GetInverseRealFFTScale(csound, npts2);
    buf1 = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (size_t) npts2);
    buf2 = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (size_t) npts2);
    /* upsample magnitude response by a factor of 2, */
    /* and store result in obuf[0]...obuf[npts]      */
    for (j = 0; j < (npts >> 1); j++)
      buf1[j] = FL(0.0);
    for (i = 0; i < npts; i++, j++)
      buf1[j] = obuf[i];
    for ( ; j < npts2; j++)
      buf1[j] = FL(0.0);
    csound->RealFFT(csound, buf1, npts2);
    for (i = j = 0; i < npts; i++, j += 2) {
      tmp = (double) buf1[j];
      tmp = sqrt(tmp * tmp + 1.0e-20);
      obuf[i] = (MYFLT) tmp;
    }
    tmp = (double) buf1[1];
    tmp = sqrt(tmp * tmp + 1.0e-20);
    obuf[i] = (MYFLT) tmp;
    /* calculate logarithm of magnitude response, */
    for (i = 0; i <= npts; i++) {
      buf1[i] = LOG(obuf[i]);
    }
    for (j = i - 2; i < npts2; i++, j--)    /* need full spectrum,     */
      buf1[i] = buf1[j];                    /* not just the lower half */
    csound->RealFFT(csound, buf1, npts2);
    /* and convolve with 1/tan(x) impulse response */
    buf2[0] = FL(0.0);
    buf2[1] = FL(0.0);
    for (i = 2; i < npts2; i += 2) {
      buf2[i] = FL(0.0);
      buf2[i + 1] = (MYFLT) (npts2 - i) / (MYFLT) npts2;
    }
    csound->RealFFTMult(csound, buf1, buf1, buf2, npts2, scaleFac);
    /* store unwrapped phase response in buf1 */
    csound->InverseRealFFT(csound, buf1, npts2);
    /* convert from magnitude/phase format to real/imaginary */
    for (i = 2; i < npts2; i += 2) {
      double  ph;
      ph = (double) buf1[i >> 1] / TWOPI;
      ph = TWOPI * modf(ph, &tmp);
      ph = (ph < 0.0 ? ph + PI : ph - PI);
      tmp = -((double) scaleFac * (double) obuf[i >> 1]);
      buf2[i] = (MYFLT) (tmp * cos(ph));
      buf2[i + 1] = (MYFLT) (tmp * sin(ph));
    }
    buf2[0] = scaleFac * obuf[0];
    buf2[1] = scaleFac * obuf[npts];
    /* perform inverse FFT to get impulse response */
    csound->InverseRealFFT(csound, buf2, npts2);
    /* copy output, truncating to table length + guard point */
    for (i = 0; i <= npts; i++)
      obuf[i] = buf2[i];
    csound->Free(csound, buf2);
    csound->Free(csound, buf1);
    if (wbuf != NULL && !(mode & 8))    /* apply window if requested */
      gen53_apply_window(obuf, wbuf, npts, wpts, 1);
}

static int gen53(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *srcftp, *dstftp, *winftp = NULL;
    int     nargs = ff->e.pcnt - 4;
    int     mode = 0, srcftno, winftno = 0, srcflen, dstflen, winflen = 0;

    if (UNLIKELY(nargs < 1 || nargs > 3)) {
      return fterror(ff, Str("GEN53: invalid number of gen arguments"));
    }
    srcftno = (int) MYFLT2LRND(ff->e.p[5]);
    if (nargs > 1)
      mode = (int) MYFLT2LRND(ff->e.p[6]);
    if (nargs > 2)
      winftno = (int) MYFLT2LRND(ff->e.p[7]);

    dstftp = ftp->ftable; dstflen = (int) ftp->flen;
    if (UNLIKELY(dstflen < 8 || (dstflen & (dstflen - 1)))) {
      return fterror(ff, Str("GEN53: invalid table length"));
    }
    srcflen = csoundGetTable(csound, &srcftp, srcftno);
    if (UNLIKELY(srcflen < 0)) {
      return fterror(ff, Str("GEN53: invalid source table number"));
    }
    if (UNLIKELY(mode & (~15))) {
      return fterror(ff, Str("GEN53: mode must be in the range 0 to 15"));
    }
    if (UNLIKELY((!(mode & 2) && srcflen != (dstflen >> 1)) ||
                 ((mode & 2) && srcflen != dstflen))) {
      return fterror(ff, Str("GEN53: invalid source table length"));
    }
    if (winftno) {
      winflen = csoundGetTable(csound, &winftp, winftno);
      if (UNLIKELY(winflen <= 0 || (winflen & (winflen - 1)))) {
        return fterror(ff, Str("GEN53: invalid window table"));
      }
    }
    if (mode & 2) {     /* if input data is impulse response: */
      MYFLT *tmpft;
      int   i, j;
      tmpft = (MYFLT*) csound->Calloc(csound, sizeof(MYFLT)
                                              * (size_t) ((dstflen >> 1) + 1));
      memcpy(dstftp, srcftp, sizeof(MYFLT) * (size_t) dstflen);
      csound->RealFFT(csound, dstftp, dstflen);
      tmpft[0] = dstftp[0];
      for (i = 2, j = 1; i < dstflen; i += 2, j++)
        tmpft[j] = SQRT(((dstftp[i] * dstftp[i])
                         + (dstftp[i + 1] * dstftp[i + 1])));
      tmpft[j] = dstftp[1];
      gen53_freq_response_to_ir(csound, dstftp, tmpft, winftp,
                                        dstflen, winflen, mode);
      csound->Free(csound, tmpft);
    }
    else                /* input is frequency response: */
      gen53_freq_response_to_ir(csound, dstftp, srcftp, winftp,
                                        dstflen, winflen, mode);
    return OK;
}

int allocgen(CSOUND *csound, char *s, GEN fn)
{
    NAMEDGEN *n = (NAMEDGEN*) csound->namedgen;

    while (n != NULL) {
      if (strcmp(s, n->name) == 0)
        return n->genum;
      n = n->next;
    }
    /* Need to allocate */
    n = (NAMEDGEN*) csound->Malloc(csound, sizeof(NAMEDGEN));
    n->genum = csound->genmax++;
    n->next = (NAMEDGEN*) csound->namedgen;
    n->name = csound->Malloc(csound, strlen(s) + 1);
    strcpy(n->name, s);
    csound->namedgen = (void*) n;
    if (csound->gensub == NULL) {
      csound->gensub = (GEN*) csound->Malloc(csound, csound->genmax * sizeof(GEN));
      memcpy(csound->gensub, or_sub, sizeof(or_sub));
    }
    else
      csound->gensub = (GEN*) csound->ReAlloc(csound, csound->gensub,
                                               csound->genmax * sizeof(GEN));
    csound->gensub[csound->genmax-1] = fn;
    return csound->genmax-1;
}

#include "resize.h"

 static int warned = 0;
int resize_table(CSOUND *csound, RESIZE *p)
{
    unsigned int fsize  = (unsigned int) MYFLT2LRND(*p->nsize);
    int fno  = (int) MYFLT2LRND(*p->fn);
    FUNC *ftp;

    if (warned==0) {
      printf("WARNING: EXPERIMENTAL CODE\n");
      warned = 1;
    }
    if ((ftp = csound->FTFind(csound, p->fn)) == NULL)
      return NOTOK;
    if (ftp->flen<fsize)
      ftp->ftable = (MYFLT *) csound->ReAlloc(csound, ftp->ftable,
                                              sizeof(MYFLT)*(fsize+1));
    ftp->flen = fsize+1;
    csound->flist[fno] = ftp;
    return OK;
}
