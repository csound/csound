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

extern double besseli(double);

/* Start of moving static data into a single structure */

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

static const GEN or_sub[GENMAX + 1] = {
    GENUL,
    gen01, gen02, gen03, gen04, gen05, gen06, gen07, gen08, gen09, gen10,
    gen11, gen12, gen13, gen14, gen15, gen16, gen17, gen18, gen19, gen20,
    gen21, GENUL, gen23, gen24, gen25, GENUL, gen27, gen28, GENUL, gen30,
    gen31, gen32, gen33, gen34, GENUL, GENUL, GENUL, GENUL, GENUL, gen40,
    gen41, gen42, gen43, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL,
    gen51, gen52, gen53, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL
};

typedef struct namedgen {
    char    *name;
    int     genum;
    struct namedgen *next;
} NAMEDGEN;

#define tpd360  (0.0174532925199433)

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
    long    genum, ltest;
    int     lobits, msg_enabled, i;
    FUNC    *ftp;
    FGDATA  ff;

    *ftpp = NULL;
    if (csound->gensub == NULL) {
      csound->gensub = (GEN*) mmalloc(csound, sizeof(GEN) * (GENMAX + 1));
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
      do {                                      /*      or automatic number */
        ff.fno = ++csound->ftldno;
      } while (ff.fno <= csound->maxfnum && csound->flist[ff.fno] != NULL);
      ff.e.p[1] = (MYFLT) (ff.fno);
    }
    else if (ff.fno < 0) {                      /*  fno < 0: remove         */
      ff.fno = -(ff.fno);
      if (ff.fno > csound->maxfnum ||
          (ftp = csound->flist[ff.fno]) == NULL) {
        return fterror(&ff, Str("ftable does not exist"));
      }
      csound->flist[ff.fno] = NULL;
      mfree(csound, (void*) ftp);
      if (msg_enabled)
        csound->Message(csound, Str("ftable %d now deleted\n"), ff.fno);
      return 0;
    }
    if (ff.fno > csound->maxfnum) {             /* extend list if necessary */
      FUNC  **nn;
      int   size;
      for (size = csound->maxfnum; size < ff.fno; size += MAXFNUM)
        ;
      nn = (FUNC**) mrealloc(csound, csound->flist, (size + 1) * sizeof(FUNC*));
      csound->flist = nn;
      for (i = csound->maxfnum + 1; i <= size; i++)
        csound->flist[i] = NULL;                /*  Clear new section       */
      csound->maxfnum = size;
    }
    if (ff.e.pcnt <= 4) {                       /*  chk minimum arg count   */
      return fterror(&ff, Str("insufficient gen arguments"));
    }
    memcpy(&(ff.e.p[2]), &(evtblkp->p[2]),
           sizeof(MYFLT) * ((int) ff.e.pcnt - 1));
    if ((genum = (long) MYFLT2LRND(ff.e.p[4])) == SSTRCOD) {
      /* A named gen given so search the list of extra gens */
      NAMEDGEN *n = (NAMEDGEN*) csound->namedgen;
      while (n) {
        if (strcmp(n->name, ff.e.strarg) == 0) {    /* Look up by name */
          genum = n->genum;
          break;
        }
        n = n->next;                            /*  and round again         */
      }
      if (n == NULL) {
        return fterror(&ff, Str("Named gen \"%s\" not defined"), ff.e.strarg);
      }
    }
    else {
      if (genum < 0)
        genum = -genum;
      if (!genum || genum > GENMAX) {           /*   & legal gen number     */
        return fterror(&ff, Str("illegal gen number"));
      }
    }
    ff.flen = (long) MYFLT2LRND(ff.e.p[3]);
    if (!ff.flen) {
      /* defer alloc to gen01|gen23|gen28 */
      ff.guardreq = 1;
      if (genum != 1 && genum != 23 && genum != 28) {
        return fterror(&ff, Str("deferred size for GEN1 only"));
      }
      if (msg_enabled)
        csound->Message(csound, Str("ftable %d:\n"), ff.fno);
      i = (*csound->gensub[genum])(&ff, NULL);
      ftp = csound->flist[ff.fno];
      if (i != 0) {
        csound->flist[ff.fno] = NULL;
        mfree(csound, ftp);
        return -1;
      }
      *ftpp = ftp;
      return 0;
    }
    /* if user flen given */
    if (ff.flen < 0L) {                 /* gab for non-pow-of-two-length    */
      ff.guardreq = 1;
      ff.flen = -(ff.flen) - 1L;
      if (!(ff.flen & (ff.flen - 1L)) || ff.flen > MAXLEN)
        goto powOfTwoLen;
      lobits = 0;                               /* Hope this is not needed! */
    }
    else {
      ff.guardreq = ff.flen & 01;               /*  set guard request flg   */
      ff.flen &= -2L;                           /*  flen now w/o guardpt    */
 powOfTwoLen:
      if (ff.flen <= 0L || ff.flen > MAXLEN) {
        return fterror(&ff, Str("illegal table length"));
      }
      for (ltest = ff.flen, lobits = 0;
           (ltest & MAXLEN) == 0L;
           lobits++, ltest <<= 1)
        ;
      if (ltest != MAXLEN) {                    /*  flen must be power-of-2 */
        return fterror(&ff, Str("illegal table length"));
      }
    }
    ftp = ftalloc(&ff);                         /*  alloc ftable space now  */
    ftp->lenmask  = ((ff.flen & (ff.flen - 1L)) ?
                     0L : (ff.flen - 1L));      /*  init hdr w powof2 data  */
    ftp->lobits   = lobits;
    i = (1 << lobits);
    ftp->lomask   = (long) (i - 1);
    ftp->lodiv    = FL(1.0) / (MYFLT) i;        /*    & other useful vals   */
    ftp->nchanls  = 1;                          /*    presume mono for now  */
    ftp->flenfrms = ff.flen;

    if (msg_enabled)
      csound->Message(csound, Str("ftable %d:\n"), ff.fno);
    if ((*csound->gensub[genum])(&ff, ftp) != 0) {
      csound->flist[ff.fno] = NULL;
      mfree(csound, ftp);
      return -1;
    }

    /* VL 11.01.05 for deferred GEN01, it's called in gen01raw */
    ftresdisp(&ff, ftp);                        /* rescale and display      */
    *ftpp = ftp;

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

    if (tableNum <= 0 || len <= 0 || len > (int) MAXLEN)
      return -1;
    if (tableNum > csound->maxfnum) {       /* extend list if necessary     */
      for (size = csound->maxfnum; size < tableNum; size += MAXFNUM)
        ;
      nn = (FUNC**) mrealloc(csound, csound->flist, (size + 1) * sizeof(FUNC*));
      csound->flist = nn;
      for (i = csound->maxfnum + 1; i <= size; i++)
        csound->flist[i] = NULL;            /* Clear new section            */
      csound->maxfnum = size;
    }
    /* allocate space for table */
    size = (int) sizeof(FUNC) + (len * (int) sizeof(MYFLT));
    ftp = csound->flist[tableNum];
    if (ftp == NULL)
      csound->flist[tableNum] = (FUNC*) csound->Malloc(csound, (size_t) size);
    else if (len != (int) ftp->flen) {
      if (csound->actanchor.nxtact != NULL) { /*   & chk for danger    */
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
    memset((void*) ftp, 0, (size_t) ((char*) &(ftp->ftable[0]) - (char*) ftp));
    ftp->flen = (long) len;
    if (!(len & (len - 1))) {
      /* for power of two length: */
      ftp->lenmask = (long) (len - 1);
      for (i = len, ftp->lobits = 0L; i < (int) MAXLEN; ftp->lobits++, i <<= 1)
        ;
      i = (int) MAXLEN / len;
      ftp->lomask = (long) (i - 1);
      ftp->lodiv = FL(1.0) / (MYFLT) i;
    }
    ftp->flenfrms = (long) len;
    ftp->nchanls = 1L;
    ftp->fno = (long) tableNum;

    return 0;
}

/**
 * Deletes a function table.
 * Return value is zero on success.
 */

int csoundFTDelete(CSOUND *csound, int tableNum)
{
    FUNC  *ftp;

    if ((unsigned int) (tableNum - 1) >= (unsigned int) csound->maxfnum)
      return -1;
    ftp = csound->flist[tableNum];
    if (ftp == NULL)
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

    if (nvals >= (int) ff->flen)
      nvals = (int) ff->flen + 1;               /* for all vals up to flen+1 */
    while (nvals--)
      *fp++ = *pp++;                            /*   copy into ftable   */

    return OK;
}

static int gen03(FGDATA *ff, FUNC *ftp)
{
    int     ncoefs, nargs = ff->e.pcnt - 4;
    MYFLT   xintvl, xscale;
    int     xloc, nlocs;
    MYFLT   *fp = ftp->ftable, x, sum, *coefp, *coef0, *coeflim;

    if ((ncoefs = nargs - 2) <= 0) {
      return fterror(ff, Str("no coefs present"));
    }
    coef0 = &ff->e.p[7];
    coeflim = coef0 + ncoefs;
    if ((xintvl = ff->e.p[6] - ff->e.p[5]) <= 0) {
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

    if (ff->e.pcnt < 6) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if ((srcno = (int)ff->e.p[5]) <= 0 || srcno > csound->maxfnum ||
        (srcftp = csound->flist[srcno]) == NULL) {
      return fterror(ff, Str("unknown srctable number"));
    }
    if (!ff->e.p[6]) {
      srcpts = srcftp->flen;
      valp   = &srcftp->ftable[0];
      rvalp  = NULL;
    }
    else {
      srcpts = srcftp->flen >>1;
      valp   = &srcftp->ftable[srcpts];
      rvalp  = valp - 1;
    }
    if ((ptratio = srcpts / ff->flen) < 1) {
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

    if ((nsegs = (ff->e.pcnt-5) >> 1) <= 0)    /* nsegs = nargs-1 /2 */
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    if (*valp == 0) goto gn5er2;
    do {
      amp1 = *valp++;
      if (!(seglen = (int)*valp++)) continue;
      if (seglen < 0) goto gn5er1;
      if ((mult = *valp/amp1) <= 0) goto gn5er2;
      mult = (MYFLT)pow( (double)mult, 1.0/(double)seglen );
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
      if (seglen < 0) goto gn7err;
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

    if ((nsegs = ((ff->e.pcnt - 5) >> 1)) < 1) {
      return fterror(ff, Str("insufficient arguments"));
    }
    fp = ftp->ftable;
    finp = fp + ff->flen;
    pntinc = 1;
    for (segp = &ff->e.p[3]; nsegs > 0; nsegs--) {
      segp += 2;
      segptsp = segp + 1;
      if ((npts = (int)*segptsp) < 0) {
        return fterror(ff, Str("negative segsiz"));
      }
      if (pntinc > 0) {
        pntno   = 0;
        inflexp = segp + 2;
        extremp = segp;
      }
      else {
        pntno   = npts;
        inflexp = segp;
        extremp = segp + 2;
      }
      diff2 = (*inflexp - *extremp) * FL(0.5);
      for ( ; npts > 0 && fp < finp; pntno += pntinc, npts--) {
        y = (MYFLT)pntno / *segptsp;
        *fp++ = (FL(3.0)-y) * y * y * diff2 + *extremp;
      }
      pntinc = -pntinc;
    }
    *fp = *(segp + 2);                      /* write last target point */

    return OK;
}

static int gen08(FGDATA *ff, FUNC *ftp)
{
    MYFLT   R, x, c3, c2, c1, c0, *fp, *fplim, *valp;
    MYFLT   f2 = FL(0.0), f1, f0, df1, df0, dx01, dx12 = FL(0.0), curx;
    MYFLT   slope, resd1, resd0;
    int     nsegs, npts;

    if ((nsegs = (ff->e.pcnt - 5) >> 1) <= 0) {
      return fterror(ff, Str("insufficient arguments"));
    }
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    fplim = fp + ff->flen;
    f0 = *valp++;                    /* 1st 3 params give vals at x0, x1 */
    if ((dx01 = *valp++) <= FL(0.0)) {      /*      and dist between     */
      return fterror(ff, Str("illegal x interval"));
    }
    f1 = *valp++;
    curx = df0 = FL(0.0);           /* init x to origin; slope at x0 = 0 */
    do {                            /* for each spline segmnt (x0 to x1) */
      if (nsegs > 1) {                      /* if another seg to follow  */
        MYFLT dx02;
        if ((dx12 = *valp++) <= FL(0.0)) {  /*    read its distance      */
          return fterror(ff, Str("illegal x interval"));
        }
        f2 = *valp++;                       /*    and the value at x2    */
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
    }
    while (--nsegs && fp<fplim);        /* loop for remaining segments  */
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

    if ((hcnt = (ff->e.pcnt - 4) / 3) <= 0)         /* hcnt = nargs / 3 */
      return OK;
    valp = &ff->e.p[5];
    finp = &ftp->ftable[ff->flen];
    do {
      for (inc = *(valp++) * tpdlen, amp = *(valp++),
           phs = *(valp++) * tpd360, fp = ftp->ftable; fp <= finp; fp++) {
        *fp += (MYFLT) (sin(phs) * amp);
        if ((phs += inc) >= TWOPI)
          phs -= TWOPI;
      }
    } while (--hcnt);

    return OK;
}

static int gen10(FGDATA *ff, FUNC *ftp)
{
    long    phs, hcnt;
    MYFLT   amp, *fp, *finp;
    double  tpdlen = TWOPI / (double) ff->flen;

    hcnt = ff->e.pcnt - 4;                              /* hcnt is nargs    */
    finp = &ftp->ftable[ff->flen];
    do {
      if ((amp = ff->e.p[hcnt + 4]) != FL(0.0))         /* for non-0 amps,  */
      for (phs = 0, fp = ftp->ftable; fp <= finp; fp++) {
        *fp += (MYFLT) sin(phs * tpdlen) * amp;         /* accum sin pts    */
        phs += hcnt;                                    /* phsinc is hno    */
        phs %= ff->flen;
      }
    } while (--hcnt);

    return OK;
}

static int gen11(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *fp, *finp;
    long    phs;
    double  x;
    MYFLT   denom, r, scale;
    int     n, k;
    int     nargs = ff->e.pcnt - 4;

    if ((n = (int) ff->e.p[5]) < 1) {
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
      rtn   = intpow(r, (long) n);
      rtnp1 = rtn * r;
      if ((absr = (MYFLT) fabs(r)) > FL(0.999) && absr < FL(1.001))
        scale = FL(1.0) / n;
      else scale = (FL(1.0) - absr) / (FL(1.0) - (MYFLT) fabs(rtn));
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
    long    nh, nn;
    MYFLT   *mp, *mspace, *hp, *oddhp;
    MYFLT   xamp, xintvl, scalfac, sum, prvm;

    if ((nh = ff->e.pcnt - 6) <= 0) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if ((xintvl = ff->e.p[5]) <= 0) {
      return fterror(ff, Str("illegal xint value"));
    }
    if ((xamp = ff->e.p[6]) <= 0) {
      return fterror(ff, Str("illegal xamp value"));
    }
    ff->e.p[5] = -xintvl;
    ff->e.p[6] = xintvl;
    nn = nh * sizeof(MYFLT) / 2;              /* alloc spc for terms 3,5,7,..*/
    mp = mspace = (MYFLT *)mcalloc(csound, nn); /* of 1st row of matrix, and */
    for (nn = (nh + 1) >>1; --nn; )             /* form array of non-0 terms */
      *mp++ = mxval = -mxval;                   /*  -val, val, -val, val ... */
    scalfac = 2 / xamp;
    hp = &ff->e.p[7];                           /* beginning with given h0,  */
    do {
      mp = mspace;
      oddhp = hp;
      sum = *oddhp++;                           /* sum = diag(=1) * this h   */
      for (nn = (nh+1) >>1; --nn; ) {
        oddhp++;                                /*  + odd terms * h+2,h+4,.. */
        sum += *mp++ * *oddhp++;
      }
      *hp++ = sum * mxscal;                     /* repl this h w. coef (sum) */
      mp    = mspace;
      prvm  = FL(1.0);
      for (nn = nh>>1; --nn > 0; mp++)          /* calc nxt row matrix terms */
        *mp = prvm = *mp - prvm;
      mxscal *= scalfac;
    } while (--nh);                             /* loop til all h's replaced */
    mfree(csound, (char *)mspace);
    return gen03(ff, ftp);                      /* then call gen03 to write */
}

static int gen15(FGDATA *ff, FUNC *ftp)
{
    MYFLT   xint, xamp, hsin[PMAX/2], h, angle;
    MYFLT   *fp, *cosp, *sinp;
    int     n, nh;
    void    *lp13;
    int     nargs = ff->e.pcnt - 4;

    if (nargs & 01) {
      return fterror(ff, Str("uneven number of args"));
    }
    nh = (nargs - 2) >>1;
    fp   = &ff->e.p[5];                         /* save p5, p6  */
    xint = *fp++;
    xamp = *fp++;
    for (n = nh, cosp = fp, sinp = hsin; n > 0; n--) {
      h = *fp++;                                /* rpl h,angle pairs */
      angle = (MYFLT) (*fp++ * tpd360);
      *cosp++ = h * (MYFLT)cos((double)angle);  /* with h cos angle */
      *sinp++ = h * (MYFLT)sin((double)angle);  /* and save the sine */
    }
    nargs -= nh;
    if (gen13(ff, ftp) != OK)                   /* call gen13   */
      return NOTOK;
    ftresdisp(ff, ftp);                         /* and display fno   */
    lp13 = (void*) ftp;
    ff->fno++;                                  /* alloc eq. space for fno+1 */
    ftp = ftalloc(ff);                          /* & copy header */
    memcpy((void*) ftp, lp13, (size_t) ((char*) ftp->ftable - (char*) ftp));
    ftp->fno = (long) ff->fno;
    fp    = &ff->e.p[5];
    *fp++ = xint;                               /* restore p5, p6,   */
    *fp++ = xamp;
    for (n = nh-1, sinp = hsin+1; n > 0; n--)   /* then skip h0*sin  */
      *fp++ = *sinp++;                          /* & copy rem hn*sin */
    nargs--;
    return gen14(ff, ftp);                      /* now draw ftable   */
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
      long cnt = (long) (dur + FL(0.5));
      if (alpha == FL(0.0)) {
        MYFLT c1 = (nxtval-val)/dur;
        while (cnt-- > 0) {
          *fp++ = val = val + c1;
        }
      }
      else {
        MYFLT c1 = (nxtval - val)/(FL(1.0) - (MYFLT)exp((double)alpha));
        MYFLT x;
        alpha /= dur;
        x = alpha;
        while (cnt-->0) {
          *fp++ = val + c1 * (FL(1.0) - (MYFLT)exp((double)(x)));
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

    if ((nsegs = nargs >> 1) <= 0)       /* nsegs = nargs /2 */
      goto gn17err;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    if ((ndx = (int)*valp++) != 0)
      goto gn17err;
    while (--nsegs) {
      val = *valp++;
      if ((nxtndx = (int)*valp++) <= ndx)
        goto gn17err;
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

    if ((cnt = nargs >> 2) <= 0) {
      return fterror(ff, Str("wrong number of args"));
    }
    while (cnt--) {
      fn=*pp++, amp=*pp++, start=(int)*pp++, finish=(int)*pp++;

      if ((start>ff->flen) || (finish>ff->flen)) {
        /* make sure start and finish < flen */
        return fterror(ff, Str("a range given exceeds table length"));
      }

      if ((fnp = csoundFTFind(csound,&fn)) != NULL) { /* make sure fn exists */
        fp = fnp->ftable, fnlen = fnp->flen-1;        /* and set it up */
      }
      else {
        return fterror(ff, Str("an input function does not exist"));
      }

      range = (MYFLT) (finish - start), j = start;
      while (j++ <= finish) {                      /* write the table */
        f = (MYFLT)modf((fnlen*(j-start)/range), &i);
        *(fp18 + j) += amp * ((f * (*(fp + (int)(i+1)) -
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

    if ((hcnt = nargs / 4) <= 0)                /* hcnt = nargs / 4 */
      return OK;
    valp = &ff->e.p[5];
    finp = &ftp->ftable[ff->flen];
    do {
      for (inc = *(valp++) * tpdlen, amp = *(valp++),
           phs = *(valp++) * tpd360, dc = *(valp++),
           fp = ftp->ftable; fp <= finp; fp++) {
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
    double  arg, x, xarg, beta = 0.0;
    int     i, nargs = ff->e.pcnt - 4;

    ft = ftp->ftable;
    xarg = 1.0;

    if (ff->e.p[4] < FL(0.0)) {
      xarg = ff->e.p[6];
      if ( nargs < 2 ) xarg = 1.0;
    }

    if (nargs > 2)
      beta = (double) ff->e.p[7];

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
          ft[i] = (MYFLT) (xarg * (pow(2.718281828459, -(x*x) / 2.0)));
        for (x = 0.0 ; i <= (int) ff->flen ; i++, x += arg)
          ft[i] = (MYFLT) (xarg * (pow(2.718281828459, -(x*x) / 2.0)));
        return OK;
    case 7:                     /* Kaiser */
      {
        double flen2 = (double) ff->flen / 2.0;
        double flenm12 = (double) (ff->flen-1) * (ff->flen-1);
        double besbeta = besseli(beta);
        for (i = 0, x = -flen2 + 0.1 ; i <= (int) ff->flen ; i++, x++)
          ft[i] = (MYFLT) (xarg * besseli((beta * sqrt(1.0 - x * x / flenm12)))
                                / besbeta);
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

static int gen23(FGDATA *ff, FUNC *ftp)
                                /* ASCII file table read Gab 17-feb-98*/
                                /* Modified after Paris Smaragdis by JPff */
{
    CSOUND  *csound = ff->csound;
    int     c = 0, j = 0;
    char    buf[512], *p;
    MYFLT   *fp;
    FILE    *infile;
    void    *fd;

    fd = csound->FileOpen(csound, &infile, CSFILE_STD, ff->e.strarg, "r",
                                  "SFDIR;SSDIR;INCDIR");
    if (fd == NULL) {
      return fterror(ff, Str("error opening ASCII file"));
    }
    p = buf;
    if (ftp == NULL) {
      /* Start counting elements */
      ff->flen = 0;
      while ((c = getc(infile)) != EOF) {
        if (!isspace(c)) {
          if (c == ';') {
            while ((c = getc(infile)) != '\n')
              ;
          }
          else *p++ = c;
        }
        else {
          char pp;
          *p = '\0';
          for (p = buf; (pp = *p) != '\0'; p++) {
            if (!isdigit(pp) && pp != '-' && pp != '.' && pp != '\0')
              goto nxt;
          }
          ff->flen++;
        nxt:
          while (isspace(c=getc(infile)))
            ;
          ungetc(c, infile);
          p = buf;
        }
      }
      csound->Message(csound, Str("%ld elements in %s\n"),
                              ff->flen, ff->e.strarg);
      rewind(infile);
      /* Allocate memory and read them in now */
  /*  ff->flen      = ff->flen + 2;        ??? */
      ftp           = ftalloc(ff);
    }
    fp = ftp->ftable;
    p = buf;
    while ((c = getc(infile)) != EOF && j < ff->flen) {
      if (!isspace(c)) {
        if (c == ';') {
          while ((c = getc(infile)) != '\n')
            ;
        }
        else *p++ = c;
      }
      else {
        char pp;                /* To save value */
        *p = '\0';
        for (p = buf; (pp = *p) != '\0'; p++) {
          if (!isdigit(pp) && pp != '-' && pp != '.' && pp != '\0')
            goto next;
        }
        *fp++ = (MYFLT) atof (buf);
        j++;
      next:
        while (isspace(c = getc(infile)))
          ;
        ungetc(c, infile);
        p = buf;
      }
    }
    csound->FileClose(csound, fd);

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

    if (nargs < 3) {
      return fterror(ff, Str("insufficient arguments"));
    }
    if ((srcno = (int) ff->e.p[5]) <= 0 ||
        srcno > csound->maxfnum         ||
        (srcftp = csound->flist[srcno]) == NULL) {
      return fterror(ff, Str("unknown srctable number"));
    }
    fp_source = srcftp->ftable;

    new_min = ff->e.p[6];
    new_max = ff->e.p[7];
    srcpts = srcftp->flen;
    if (srcpts!= ff->flen) {
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

    if ((nsegs = ((nargs / 2) - 1)) <= 0)
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    do {
      x1 = *valp++;
      y1 =  *valp++;
      x2 = *valp++;
      if (nsegs > 1)
        y2 =  *valp++;
      else
        y2 = *valp;
      if (x2 < x1) goto gn25err;
      if (x1 > ff->flen || x2 > ff->flen) goto gn25err2;
      seglen = (int)(x2-x1);
      if (y1 <= 0 || y2 <= 0) goto gn25err3;
      mult = y2/y1;
      mult = (MYFLT)pow((double)mult, 1.0/(double)seglen);
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
    return fterror(ff, Str("x coordindates must all be in increasing order:"));

 gn25err2:
    return fterror(ff, Str("x coordindate greater than function size:"));

 gn25err3:
    return fterror(ff,
                   Str("illegal input val (y <= 0) for gen call, beginning:"));
}

static int gen27(FGDATA *ff, FUNC *ftp)
{
    int     nsegs;
    MYFLT   *valp, *fp, *finp;
    MYFLT   x1, x2, y1, y2, seglen, incr;
    int     nargs = ff->e.pcnt - 4;

    if ((nsegs = ((nargs / 2) - 1)) <= 0)
      return OK;
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    do {
      x1 = *valp++;
      y1 = *valp++;
      x2 = *valp++;
      if (nsegs > 1)
        y2 =  *valp++;
      else
        y2 = *valp;
      if (x2 < x1) goto gn27err;
      if (x1 > ff->flen || x2 > ff->flen) goto gn27err2;
      seglen = x2-x1;
      incr = (y2 - y1) / seglen;
      while (seglen--) {
        *fp++ = y1;
        y1 += incr;
        if (fp > finp)
          return OK;
      }
      valp -= 2;
    } while (--nsegs);
    if (fp == finp)                     /* if 2**n pnts, add guardpt */
      *fp = y1;
    return OK;

 gn27err:
    return fterror(ff, Str("x coordindates must all be in increasing order:"));
 gn27err2:
    return fterror(ff, Str("x coordindate greater than function size:"));
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

    if (ff->flen)
      return fterror(ff, Str("GEN28 requires zero table length"));
    fd = csound->FileOpen(csound, &filp, CSFILE_STD, ff->e.strarg, "r",
                                  "SFDIR;SSDIR;INCDIR");
    if (fd == NULL)
      goto gen28err1;

    x = (MYFLT*)mmalloc(csound, arraysize*sizeof(MYFLT));
    y = (MYFLT*)mmalloc(csound, arraysize*sizeof(MYFLT));
    z = (MYFLT*)mmalloc(csound, arraysize*sizeof(MYFLT));
#if defined(USE_DOUBLE)
    while (fscanf( filp, "%lf%lf%lf", &z[i], &x[i], &y[i])!= EOF) {
#else
    while (fscanf( filp, "%f%f%f", &z[i], &x[i], &y[i])!= EOF) {
#endif
      i++;
      if (i>=arraysize) {
        arraysize += 1000;
        x = (MYFLT*)mrealloc(csound, x, arraysize*sizeof(MYFLT));
        y = (MYFLT*)mrealloc(csound, y, arraysize*sizeof(MYFLT));
        z = (MYFLT*)mrealloc(csound, z, arraysize*sizeof(MYFLT));
      }
    }
    --i;

    ff->flen      = (long) (z[i] * resolution * 2);
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

      if (z2 < z1) goto gen28err2;
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

    mfree(csound, x); mfree(csound, y); mfree(csound, z);
    csound->FileClose(csound, fd);

    return OK;

 gen28err1:
    return fterror(ff, Str("could not open space file"));
 gen28err2:
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

    if (nargs < 3) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    xsr = FL(1.0);
    if ((nargs > 3) && (ff->e.p[8] > FL(0.0)))
      xsr = csound->esr / ff->e.p[8];
    f2 = csound->GetTable(csound, (int) ff->e.p[5], &l2);
    if (f2 == NULL) {
      return fterror(ff, Str("GEN30: source ftable not found"));
    }
    f1 = &(ftp->ftable[0]);
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
    x = (MYFLT*) mmalloc(csound, sizeof(MYFLT) * i);
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
    for (i = 0; i < l1; i++)
      f1[i] = x[i];
    f1[l1] = f1[0];     /* write guard point */
    mfree(csound, x);

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

    if (nargs < 4) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    f2 = csound->GetTable(csound, (int) ff->e.p[5], &l2);
    if (f2 == NULL) {
      return fterror(ff, Str("GEN31: source ftable not found"));
    }
    f1 = &(ftp->ftable[0]);
    l1 = (int) ftp->flen;

    x = (MYFLT*) mcalloc(csound, sizeof(MYFLT) * (l2 + 2));
    y = (MYFLT*) mcalloc(csound, sizeof(MYFLT) * (l1 + 2));
    /* read and analyze src table, apply amplitude scale */
    a = csound->GetInverseRealFFTScale(csound, l1) * (MYFLT) l1 / (MYFLT) l2;
    for (i = 0; i < l2; i++)
      x[i] = a * f2[i];
    csound->RealFFT(csound, x, l2);
    x[l2] = x[1];
    x[1] = x[l2 + 1] = FL(0.0);

    for (j = 6; j < (nargs + 3); j++) {
      n = (int) (FL(0.5) + ff->e.p[j++]); if (n < 1) n = 1; /* frequency */
      a = ff->e.p[j++];                                     /* amplitude */
      p = ff->e.p[j];                                       /* phase     */
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
    for (i = 0; i < l1; i++)
      f1[i] = y[i];
    f1[l1] = f1[0];     /* write guard point */

    mfree(csound, x);
    mfree(csound, y);

    return OK;
}

/* gen32: transpose, phase shift, and mix source tables */

static int gen32(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *x, *y, *f1, *f2;
    MYFLT   a, p;
    double  d_re, d_im, p_re, p_im, ptmp;
    int     i, j, k, n, l1, l2, ntabl, *pnum, ft;
    int     nargs = ff->e.pcnt - 4;

    if (nargs < 4) {
      return fterror(ff, Str("insufficient gen arguments"));
    }

    ntabl = nargs >> 2;         /* number of waves to mix */
    pnum  = (int*) mmalloc(csound, sizeof(int) * ntabl);
    for (i = 0; i < ntabl; i++)
      pnum[i] = (i << 2) + 5;   /* p-field numbers */
    do {
      i = k = 0;                        /* sort by table number */
      while (i < (ntabl - 1)) {
        if (ff->e.p[pnum[i]] > ff->e.p[pnum[i + 1]]) {
          j = pnum[i]; pnum[i] = pnum[i + 1]; pnum[i + 1] = j;
          k = 1;
        }
        i++;
      }
    } while (k);

    f1 = &(ftp->ftable[0]);
    l1 = (int) ftp->flen;
    for (i = 0; i <= l1; i++)
      f1[i] = FL(0.0);
    x = y = NULL;

    ft = 0x7FFFFFFF;            /* last table number */
    j  = -1;                    /* current wave number */

    while (++j < ntabl) {
      p = ff->e.p[pnum[j]];                /* table number */
      i = (int) (p + (p < FL(0.0) ? FL(-0.5) : FL(0.5)));
      f2 = csound->GetTable(csound, abs(i), &l2);
      if (f2 == NULL) {
        fterror(ff, Str("GEN32: source ftable %d not found"), abs(i));
        mfree(csound, pnum);
        if (x != NULL) mfree(csound, x);
        if (y != NULL) mfree(csound, x);
        return NOTOK;
      }
      if (i < 0) {              /* use linear interpolation */
        ft = i;
        p_re  = (double) ff->e.p[pnum[j] + 3];     /* start phase */
        p_re -= (double) ((int) p_re); if (p_re < 0.0) p_re++;
        p_re *= (double) l2;
        d_re  = (double) ff->e.p[pnum[j] + 1];     /* frequency */
        d_re *= (double) l2 / (double) l1;
        a     = ff->e.p[pnum[j] + 2];              /* amplitude */
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
            y = (MYFLT*) mcalloc(csound, sizeof (MYFLT) * (l1 + 2));
          if (x != NULL) mfree(csound, x);
          x = (MYFLT*) mcalloc(csound, sizeof (MYFLT) * (l2 + 2));
          /* read and analyze src table */
          for (i = 0; i < l2; i++)
            x[i] = f2[i];
          csound->RealFFT(csound, x, l2);
          x[l2] = x[1];
          x[1] = x[l2 + 1] = FL(0.0);
        }
        n = (int) (FL(0.5) + ff->e.p[pnum[j] + 1]);         /* frequency */
        if (n < 1) n = 1;
        a = ff->e.p[pnum[j] + 2] * (MYFLT) l1 / (MYFLT) l2; /* amplitude */
        a *= csound->GetInverseRealFFTScale(csound, (int) l1);
        p = ff->e.p[pnum[j] + 3];                           /* phase */
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
      mfree(csound, x);         /* free tmp memory */
      mfree(csound, y);
    }
    mfree(csound, pnum);

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

    if (nargs < 3) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    if (nargs > 3)      /* check optional argument */
      fmode = ff->e.p[8];
    else
      fmode = FL(0.0);
    /* table length and data */
    ft = ftp->ftable; flen = (int) ftp->flen;
    /* source table */
    srcft = csound->GetTable(csound, (int) ff->e.p[5], &srclen);
    if (srcft == NULL) {
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
    x = (MYFLT*) mcalloc(csound, sizeof(MYFLT) * (flen + 2));

    maxp = flen >> 1;           /* max. partial number */
    i = nh;
    while (i--) {
      /* amplitude */
      amp = scl * *(srcft++);
      /* partial number */
      pnum = (int) (fmode * *srcft + (*srcft < FL(0.0) ? FL(-0.5) : FL(0.5)));
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
      x[pnum << 1] += amp * (MYFLT) sin((double) phs);
      x[(pnum << 1) + 1] -= amp * (MYFLT) cos((double) phs);
    }

    csound->InverseRealFFT(csound, x, flen);    /* iFFT */

    for (i = 0; i < flen; i++)  /* copy to output table */
      ft[i] = x[i];
    ft[flen] = x[0];            /* write guard point */

    /* free tmp memory */
    mfree(csound, x);

    return OK;
}

/* GEN34 by Istvan Varga */

static int gen34(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   fmode, *ft, *srcft, scl;
    double  y0, y1, x, c, v, *xn, *cn, *vn, *tmp, amp, frq, phs;
    long    nh, flen, srclen, i, j, k, l, bs;
    FUNC    *src;
    int     nargs = ff->e.pcnt - 4;

    if (nargs < 3) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    if (nargs > 3)      /* check optional argument */
      fmode = ff->e.p[8];
    else
      fmode = FL(0.0);
    /* table length and data */
    ft = ftp->ftable; flen = (long) ftp->flen;
    /* source table */
    if ((src = csoundFTFind(csound, &(ff->e.p[5]))) == NULL)
      return NOTOK;
    srcft = src->ftable; srclen = (long) src->flen;
    /* number of partials */
    nh = (long) (ff->e.p[6] + FL(0.5));
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
    tmp = (double*) mmalloc(csound, sizeof (double) * bs);
    xn  = (double*) mmalloc(csound, sizeof (double) * (nh + 1L));
    cn  = (double*) mmalloc(csound, sizeof (double) * (nh + 1L));
    vn  = (double*) mmalloc(csound, sizeof (double) * (nh + 1L));
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
      for (i = 0L; i < k; i++) tmp[i] = 0.0;
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
    mfree(csound, tmp); mfree(csound, xn); mfree(csound, cn); mfree(csound, vn);

    return OK;
}

static int gen40(FGDATA *ff, FUNC *ftp)               /*gab d5*/
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp = ftp->ftable, *fp_source, *fp_temp;
    FUNC    *srcftp;
    int     srcno, srcpts, j, k;
    MYFLT   last_value = FL(0.0), lenratio;

    if ((srcno = (int) ff->e.p[5]) <= 0 ||
        srcno > csound->maxfnum         ||
        (srcftp = csound->flist[srcno]) == NULL) {
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
    long    tot_prob = 0;
    int     nargs = ff->e.pcnt - 4;

    for (j=0; j < nargs; j+=2) {
      tot_prob += (long) pp[j+1];
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
    MYFLT   *fp = ftp->ftable, *pp = &ff->e.p[5], inc;
    int     j, k, width;
    long    tot_prob = 0;
    int     nargs = ff->e.pcnt - 4;

    for (j=0; j < nargs; j+=3) {
      tot_prob += (long) pp[j+2];
    }
    for (j=0; j< nargs; j+=3) {
      width = (int) ((pp[j+2]/tot_prob) * ff->flen +FL(0.5));
      inc = (pp[j+1]-pp[j]) / (MYFLT) (width-1);
      for ( k=0; k < width; k++) {
        *fp++ = pp[j]+(inc*k);
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

    sprintf(buf, Str("ftable %d: "), ff->fno);
    va_start(args, s);
    csound->ErrMsgV(csound, buf, s, args);
    va_end(args);
    csound->Message(csound, "f%3.0f %8.2f %8.2f ",
                            ff->e.p[1], ff->e.p2orig, ff->e.p3orig);
    if (ff->e.p[4] == SSTRCOD)
      csound->Message(csound, "%s", ff->e.strarg);
    else
      csound->Message(csound, "%8.2f", ff->e.p[4]);
    if (ff->e.p[5] == SSTRCOD)
      csound->Message(csound, "  \"%s\" ...\n", ff->e.strarg);
    else
      csound->Message(csound, "%8.2f ...\n", ff->e.p[5]);

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
    sprintf(strmsg, Str("ftable %d:"), (int) ff->fno);
    dispset(csound, &dwindow, ftp->ftable, (long) (ff->flen + ff->guardreq),
                    strmsg, 0, "ftable");
    display(csound, &dwindow);
}

/* alloc ftable space for fno (or replace one) */
/*  set ftp to point to that structure         */

static CS_NOINLINE FUNC *ftalloc(const FGDATA *ff)
{
    CSOUND  *csound = ff->csound;
    FUNC    *ftp = csound->flist[ff->fno];

    if (ftp != NULL) {
      if (csound->oparms->msglevel & WARNMSG)
        csound->Warning(csound, Str("replacing previous ftable %d"), ff->fno);
      if (ff->flen != ftp->flen) {          /* if redraw & diff len, */
        mfree(csound, (void*) ftp);         /*   release old space   */
        csound->flist[ff->fno] = ftp = NULL;
        if (csound->actanchor.nxtact != NULL) { /*   & chk for danger    */
          csound->Warning(csound, Str("ftable %d relocating due to size change"
                                      "\n         currently active instruments "
                                      "may find this disturbing"), ff->fno);
        }
      }
      else {                                /* else clear it to zero */
        size_t  nBytes = sizeof(FUNC) + (size_t) ff->flen * sizeof(MYFLT);
        memset((void*) ftp, 0, nBytes);
      }
    }
    if (ftp == NULL) {                      /*   alloc space as reqd */
      size_t  nBytes = sizeof(FUNC) + (size_t) ff->flen * sizeof(MYFLT);
      csound->flist[ff->fno] = ftp = (FUNC*) mcalloc(csound, nBytes);
    }
    ftp->fno = (long) ff->fno;
    ftp->flen = ff->flen;

    return ftp;
}

/* find the ptr to an existing ftable structure */
/*   called by oscils, etc at init time         */

FUNC *csoundFTFind(CSOUND *csound, MYFLT *argp)
{
    FUNC    *ftp;
    int     fno;

    if ((fno = (int) *argp) <= 0 ||
        fno > csound->maxfnum       ||
        (ftp = csound->flist[fno]) == NULL) {
      csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    else if (!ftp->lenmask) {
      csoundInitError(csound,
                      Str("deferred-size ftable %f illegal here"), *argp);
      return NULL;
    }
    return ftp;
}

static CS_NOINLINE FUNC *gen01_defer_load(CSOUND *csound, int fno)
{
    FGDATA  ff;
    char    strarg[SSTRSIZ];
    FUNC    *ftp = csound->flist[fno];

    /* The soundfile hasn't been loaded yet, so call GEN01 */
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
    if (gen01raw(&ff, ftp) != 0) {
      csoundErrorMsg(csound, Str("Deferred load of '%s' failed"), strarg);
      return NULL;
    }
    return csound->flist[fno];
}

PUBLIC MYFLT *csoundGetTable(CSOUND *csound, int tableNum, int *tableLength)
{
    FUNC    *ftp;

    if ((unsigned int) (tableNum - 1) >= (unsigned int) csound->maxfnum)
      goto err_return;
    ftp = csound->flist[tableNum];
    if (ftp == NULL)
      goto err_return;
    if (!ftp->flen) {
      ftp = gen01_defer_load(csound, tableNum);
      if (ftp == NULL)
        goto err_return;
    }
    *tableLength = (int) ftp->flen;
    return &(ftp->ftable[0]);

 err_return:
    *tableLength = -1;
    return NULL;
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
    if ((fno = (int) *argp) <= 0 ||
        fno > csound->maxfnum           ||
        (ftp = csound->flist[fno]) == NULL) {
      csoundPerfError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    else if (!ftp->lenmask) {
      /* Now check that the table has a length > 0.  This should only
       * occur for tables which have not been loaded yet.  */
      csoundPerfError(csound, Str("Deferred-size ftable %f load "
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
    int     fno;

    if ((fno = (int) *argp) <= 0 ||
        fno > csound->maxfnum    ||
        (ftp = csound->flist[fno]) == NULL) {
      csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    if (ftp->flen == 0) {
      ftp = gen01_defer_load(csound, fno);
      if (ftp == NULL)
        csound->inerrcnt++;
    }
    return ftp;
}

/* read ftable values from a sound file */
/* stops reading when table is full     */

static int gen01(FGDATA *ff, FUNC *ftp)
{
    if (ff->e.pcnt < 8) {
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
      strcpy(ftp->gen01args.strarg, ff->e.strarg);
      return OK;
    }
    return gen01raw(ff, ftp);
}

static void needsiz(CSOUND *csound, FGDATA *ff, long maxend)
{
    long nxtpow;
    maxend -= 1; nxtpow = 2;
    while (maxend >>= 1)
      nxtpow <<= 1;
    csound->Message(csound, Str("non-deferred ftable %d needs size %ld\n"),
                            (int) ff->fno, nxtpow);
}

/* read ftable values from a sound file */
/* stops reading when table is full     */

static int gen01raw(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    SOUNDIN *p;
    SOUNDIN tmpspace;
    SNDFILE *fd;
    int     truncmsg = 0;
    long    inlocs = 0;
    int     def = 0;

    p = &tmpspace;
    memset(p, 0, sizeof(SOUNDIN));
    {
      long  filno = (long) MYFLT2LRND(ff->e.p[5]);
      int   fmt = (int) MYFLT2LRND(ff->e.p[7]);
      if (filno == (long) SSTRCOD) {
        if (ff->e.strarg[0] == '"') {
          int len = (int) strlen(ff->e.strarg) - 2;
          strcpy(p->sfname, ff->e.strarg + 1);
          if (len >= 0 && p->sfname[len] == '"')
            p->sfname[len] = '\0';
        }
        else
          strcpy(p->sfname, ff->e.strarg);
      }
      else if (filno >= 0 && filno <= csound->strsmax &&
               csound->strsets && csound->strsets[filno])
        strcpy(p->sfname, csound->strsets[filno]);
      else
        sprintf(p->sfname, "soundin.%ld", filno);   /* soundin.filno */
      switch (fmt) {
        case 0: p->format = AE_SHORT; break;
        case 1: p->format = AE_CHAR;  break;
        case 2: p->format = AE_ALAW;  break;
        case 3: p->format = AE_ULAW;  break;
        case 4: p->format = AE_SHORT; break;
        case 5: p->format = AE_LONG;  break;
        case 6: p->format = AE_FLOAT; break;
        default:
          return fterror(ff, Str("invalid sample format: %d"), fmt);
      }
    }
    p->skiptime = ff->e.p[6];
    p->channel  = (int) MYFLT2LRND(ff->e.p[8]);
    p->do_floatscaling = 0;
    if (p->channel < 0 /* || p->channel > ALLCHNLS-1 */) {
      return fterror(ff, Str("channel %d illegal"), (int) p->channel);
    }
    if (p->channel == 0)                      /* snd is chan 1,2,..8 or all */
      p->channel = ALLCHNLS;
    p->analonly = 0;
    if (ff->flen == 0 && (csound->oparms->msglevel & 7))
      csound->Message(csound, Str("deferred alloc\n"));
    if ((fd = sndgetset(csound, p))==NULL) {  /* sndinset to open the file  */
      return fterror(ff, "Failed to open file");
    }
    if (ff->flen == 0) {                      /* deferred ftalloc requestd: */
      if ((ff->flen = p->framesrem) <= 0) {   /*   get minsize from soundin */
        return fterror(ff, Str("deferred size, but filesize unknown"));
      }
      if (csound->oparms->msglevel & 7)
        csound->Message(csound, Str("  defer length %ld\n"), ff->flen);
      if (p->channel == ALLCHNLS)
        ff->flen *= p->nchanls;
      ff->guardreq  = 1;                      /* presum this includes guard */
      ff->flen     -= 1;
      ftp           = ftalloc(ff);            /*   alloc now, and           */
      ftp->lenmask  = 0L;                     /*   mark hdr partly filled   */
      ftp->nchanls  = p->nchanls;
      ftp->flenfrms = ff->flen / p->nchanls;  /* ?????????? */
      def           = 1;
    }
    ftp->gen01args.sample_rate = (MYFLT) p->sr;
    ftp->cvtbas = LOFACT * p->sr * csound->onedsr;
    /* FIXME: no looping possible yet */
    ftp->cpscvt = FL(0.0);
    ftp->loopmode1 = 0;
    ftp->loopmode2 = 0;
    ftp->end1 = ftp->flenfrms;  /* Greg Sullivan */
    /* read sound with opt gain */
    if ((inlocs = getsndin(csound, fd, ftp->ftable, ff->flen + 1, p)) < 0) {
      return fterror(ff, Str("GEN1 read error"));
    }
    if (p->audrem > 0 && !truncmsg && p->framesrem > ff->flen) {
      /* Reduce msg */
      csound->Warning(csound, Str("GEN1: aiff file truncated by ftable size"));
      csound->Warning(csound, Str("\taudio samps %ld exceeds ftsize %ld"),
                              (long) p->framesrem, (long) ff->flen);
      needsiz(csound, ff, p->framesrem);     /* ????????????  */
    }
    ftp->soundend = inlocs / ftp->nchanls;   /* record end of sound samps */
    csound->FileClose(csound, p->fd);
    if (def)
      ftresdisp(ff, ftp);       /* VL: 11.01.05  for deferred alloc tables */
    return OK;
}

/* GEN 43 (c) Victor Lazzarini, 2004 */

typedef struct _pvstabledat {
    long    fftsize;
    long    overlap;
    long    winsize;
    int     wintype;
    int     chans;
    long    format;
    long    blockalign;
    unsigned long frames;
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
    unsigned long   framesize, blockalign, bins;
    unsigned long   frames, i, j;
    float           *framep, *startp;
    double          accum = 0.0;

    if (nvals != 2) {
      return fterror(ff, Str("wrong number of ftable arguments"));
    }

    filno = &ff->e.p[5];
    if (*filno == SSTRCOD)
      strcpy(filename, (char *)(&ff->e.strarg[0]));
    else
      csound->strarg2name(csound, filename, filno, "pvoc.", 0);

    if (PVOCEX_LoadFile(csound, filename, &pp) != 0)
      csoundDie(csound, Str("Failed to load PVOC-EX file"));
    p.fftsize  = pp.fftsize;
    p.overlap  = pp.overlap;
    p.winsize  = pp.winsize;
    p.wintype  = pp.wintype;
    p.chans    = pp.chans;
    p.format   = pp.format;
    p.frames   = pp.nframes;

    channel = &ff->e.p[6];
    if (*channel > p.chans)
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

    if (bins > (unsigned long) (ftp->flen+1)) {
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

static int gen51(FGDATA *ff, FUNC *ftp)    /* Gab 1/3/2005 */
{
    int     j, notenum, grade, numgrades, basekeymidi, nvals;
    MYFLT   basefreq, factor, interval;
    MYFLT   *fp = ftp->ftable, *pp;

    nvals       = ff->flen;
    pp          = &(ff->e.p[5]);
    numgrades   = (int) *pp++;
    interval    = *pp++;
    basefreq    = *pp++;
    basekeymidi = (int) *pp++;

    if ((ff->e.pcnt - 8) != numgrades) {
      return fterror(ff, Str("gen51: invalid number of p-fields"));
    }

    for (j = 0; j < nvals; j++) {
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
      factor = (MYFLT) pow((double) interval, (double) factor);
      fp[j] = pp[grade] * factor * basefreq;
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

    if (nargs < 4) {
      return fterror(ff, Str("insufficient gen arguments"));
    }
    nchn = MYFLT2LRND(ff->e.p[5]);
    if (((nchn * 3) + 1) != nargs) {
      return fterror(ff, Str("number of channels "
                             "inconsistent with number of args"));
    }
    len = ((int) ftp->flen / nchn) * nchn;
    dst = &(ftp->ftable[0]);
    for (i = len; i <= (int) ftp->flen; i++)
      dst[i] = FL(0.0);
    for (n = 0; n < nchn; n++) {
      f = csoundFTFind(csound, &(ff->e.p[(n * 3) + 6]));
      if (f == NULL)
        return NOTOK;
      len2 = (int) f->flen;
      src = &(f->ftable[0]);
      i = n;
      j = MYFLT2LRND(ff->e.p[(n * 3) + 7]);
      k = MYFLT2LRND(ff->e.p[(n * 3) + 8]);
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
      obuf[i++] = (MYFLT) (fabs((double) ibuf[j])) * scaleFac; j++;
      obuf[i++] = FL(0.0);
      obuf[i++] = -((MYFLT) (fabs((double) ibuf[j])) * scaleFac); j++;
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
      buf1[i] = (MYFLT) log((double) obuf[i]);
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

    if (nargs < 1 || nargs > 3) {
      return fterror(ff, Str("GEN53: invalid number of gen arguments"));
    }
    srcftno = (int) MYFLT2LRND(ff->e.p[5]);
    if (nargs > 1)
      mode = (int) MYFLT2LRND(ff->e.p[6]);
    if (nargs > 2)
      winftno = (int) MYFLT2LRND(ff->e.p[7]);

    dstftp = ftp->ftable; dstflen = (int) ftp->flen;
    if (dstflen < 8 || (dstflen & (dstflen - 1))) {
      return fterror(ff, Str("GEN53: invalid table length"));
    }
    srcftp = csound->GetTable(csound, srcftno, &srcflen);
    if (srcftp == NULL) {
      return fterror(ff, Str("GEN53: invalid source table number"));
    }
    if (mode & (~15)) {
      return fterror(ff, Str("GEN53: mode must be in the range 0 to 15"));
    }
    if ((!(mode & 2) && srcflen != (dstflen >> 1)) ||
        ((mode & 2) && srcflen != dstflen)) {
      return fterror(ff, Str("GEN53: invalid source table length"));
    }
    if (winftno) {
      winftp = csound->GetTable(csound, winftno, &winflen);
      if (winftp == NULL || winflen < 1 || (winflen & (winflen - 1))) {
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
        tmpft[j] = (MYFLT) sqrt((double) ((dstftp[i] * dstftp[i])
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
    n = (NAMEDGEN*) mmalloc(csound, sizeof(NAMEDGEN));
    n->genum = csound->genmax++;
    n->next = (NAMEDGEN*) csound->namedgen;
    n->name = mmalloc(csound, strlen(s) + 1);
    strcpy(n->name, s);
    csound->namedgen = (void*) n;
    if (csound->gensub == NULL) {
      csound->gensub = (GEN*) mmalloc(csound, csound->genmax * sizeof(GEN));
      memcpy(csound->gensub, or_sub, sizeof(or_sub));
    }
    else
      csound->gensub = (GEN*) mrealloc(csound, csound->gensub,
                                               csound->genmax * sizeof(GEN));
    csound->gensub[csound->genmax-1] = fn;
    return csound->genmax-1;
}

