/*
    fgens.c:

    Copyright (C) 1991, 1994, 1995, 1998, 2000
                  Barry Vercoe, John ffitch, Paris Smaragdis, Gabriel Maldonado,
                  Richard Karpen, Greg Sullivan, Pete Moss, Istvan Varga

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

#include "cs.h"             /*                              FGENS.C         */
#include <ctype.h>
#include <stdarg.h>
#include "soundio.h"
#include "cwindow.h"
#include <math.h>
#include "cmath.h"
#include "ftgen.h"

/* New function in FILOPEN.C to look around for (text) files */
extern FILE *fopenin(ENVIRON *csound, char *filnam);
extern double besseli(double);

/* Start of moving static data into a single structure */

static void gen01raw(FUNC*,ENVIRON*);
static void gen01(FUNC*,ENVIRON*), gen02(FUNC*,ENVIRON*), gen03(FUNC*,ENVIRON*);
static void gen04(FUNC*,ENVIRON*), gen05(FUNC*,ENVIRON*), gen06(FUNC*,ENVIRON*);
static void gen07(FUNC*,ENVIRON*), gen08(FUNC*,ENVIRON*), gen09(FUNC*,ENVIRON*);
static void gen10(FUNC*,ENVIRON*), gen11(FUNC*,ENVIRON*), gen12(FUNC*,ENVIRON*);
static void gen13(FUNC*,ENVIRON*), gen14(FUNC*,ENVIRON*), gen15(FUNC*,ENVIRON*);
static void gen17(FUNC*,ENVIRON*), gen18(FUNC*,ENVIRON*);
static void gen19(FUNC*,ENVIRON*), gen20(FUNC*,ENVIRON*), gen21(FUNC*,ENVIRON*);
static void gen23(FUNC*,ENVIRON*), gen24(FUNC*,ENVIRON*), gen16(FUNC*,ENVIRON*);
static void gen25(FUNC*,ENVIRON*), gen27(FUNC*,ENVIRON*), gen28(FUNC*,ENVIRON*);
static void gen30(FUNC*,ENVIRON*), gen31(FUNC*,ENVIRON*), gen32(FUNC*,ENVIRON*);
static void gen33(FUNC*,ENVIRON*), gen34(FUNC*,ENVIRON*), gen40(FUNC*,ENVIRON*);
static void gen41(FUNC*,ENVIRON*), gen42(FUNC*,ENVIRON*), gen43(FUNC*,ENVIRON*);
static void gn1314(FUNC*,ENVIRON*, MYFLT, MYFLT);
static void gen51(FUNC*,ENVIRON*), gen52(FUNC*,ENVIRON*);
static void GENUL(FUNC*,ENVIRON*);

static GEN or_sub[GENMAX + 1] = {
    GENUL,
    gen01, gen02, gen03, gen04, gen05, gen06, gen07, gen08, gen09, gen10,
    gen11, gen12, gen13, gen14, gen15, gen16, gen17, gen18, gen19, gen20,
    gen21, GENUL, gen23, gen24, gen25, GENUL, gen27, gen28, GENUL, gen30,
    gen31, gen32, gen33, gen34, GENUL, GENUL, GENUL, GENUL, GENUL, gen40,
    gen41, gen42, gen43, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL,
    gen51, gen52, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL, GENUL
};

typedef struct namedgen {
        char            *name;
        int              genum;
        struct namedgen *next;
} NAMEDGEN;

static  NAMEDGEN *namedgen = NULL;

#define tpd360  (0.017453293)

static  void    fterror(ENVIRON *, FGDATA *, char *, ...);
static  void    ftresdisp(ENVIRON *, FGDATA *, FUNC*);
static  FUNC    *ftalloc(ENVIRON *);

#define FTPMAX  (150)

#define RNDINT(x) ((int) ((double) (x) + ((double) (x) < 0.0 ? -0.5 : 0.5)))

void ftRESET(ENVIRON *csound)
{
  int i;
  if (csound->flist) {
    for (i = 1; i <= csound->maxfnum; i++)
      mfree(csound, csound->flist[i]);   /* Check this */
    mfree(csound, csound->flist);
    csound->flist   = NULL;
  }
  csound->maxfnum = 0;
  if (csound->gensub) {
    mfree(csound, csound->gensub);
    csound->gensub = NULL;
  }
  while (namedgen) {
    NAMEDGEN *next = namedgen->next;
    mfree(csound, namedgen);
    namedgen = next;
  }
}

static void GENUL(FUNC *ftp, ENVIRON *csound)
{
    fterror(csound, &(csound->ff), Str("unknown GEN number"));
    return;
}

void fgens(ENVIRON *csound, EVTBLK *evtblkp)
{                                       /* create ftable using evtblk data */
    long        ltest, lobits, lomod, genum;
    FUNC        *ftp = NULL;
    int         nargs;
    FGDATA *ff = &csound->ff;

    if (csound->gensub==NULL) {
      csound->gensub = (GEN*) mmalloc(csound, sizeof(GEN)*(GENMAX+1));
      memcpy(csound->gensub, or_sub, sizeof(GEN)*(GENMAX+1));
      csound->genmax = GENMAX+1;
    }
    ff->e = *evtblkp;
    ff->fterrcnt = 0;
    if ((ff->fno = (int)ff->e.p[1]) < 0) {   /* fno < 0: remove */
      if ((ff->fno = -ff->fno) > csound->maxfnum) {
        fterror(csound, ff, Str("illegal ftable number")); return;
      }
      if ((ftp = csound->flist[ff->fno]) == NULL) {
        fterror(csound, ff, Str("ftable does not exist"));
        return;
      }
      csound->flist[ff->fno] = NULL;
      mfree(csound, (char *)ftp);
      csound->Message(csound, Str("ftable %d now deleted\n"), ff->fno);
      return;
    }
    if (!ff->fno)                           /* fno = 0, return      */
      return;
    if (ff->fno > csound->maxfnum) {
      int size = csound->maxfnum;
      FUNC **nn;
      int i;
      while (ff->fno >= size)
        size += MAXFNUM;
      size++;
      nn = (FUNC**)mrealloc(csound, csound->flist, size*sizeof(FUNC*));
      csound->flist = nn;
      for (i=csound->maxfnum+1; i<size; i++)
        csound->flist[i] = NULL;             /* Clear new section */
      csound->maxfnum = size-1;
    }
    if ((nargs = ff->e.pcnt - 4) <= 0) {     /* chk minimum arg count */
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
    }
    if ((genum = (long)ff->e.p[4])==SSTRCOD) {
      /* A named gen given so search the list of extra gens */
      NAMEDGEN *n = namedgen;
/*       csound->DebugMsg(csound, "*** Named fgen %s", ff->e.strarg); */
      while (n) {
        if (strcmp(n->name, ff->e.strarg)==0) {    /* Look up by name */
          genum = n->genum;
          break;
        }
        n = n->next;                          /* and round again */
      }
      if (n == NULL) {
        fterror(csound, ff, Str("Named gen %s not defined"), ff->e.strarg);
        return;
      }
    }
    else {
      if (genum < 0)
        genum = -genum;
      if (!genum || genum > GENMAX) {          /*   & legal gen number */
        fterror(csound, ff, Str("illegal gen number"));
        return;
      }
    }
    if ((ff->flen = (long)(ff->e.p[3]+FL(0.5)))) { /* if user flen given */
      if (ff->flen < 0 ) { /* gab for non-pow-of-two-length */
        ff->flen = labs(ff->flen-1);
        ff->lenmask = 0xFFFFFFFF;
        lobits = 0;    /* Hope this is not needed! */
      }
      else {
        ff->guardreq = ff->flen & 01;           /*   set guard request flg  */
        ff->flen &= -2;                         /*   flen now w/o guardpt   */
        ff->flenp1 = ff->flen + 1;              /*   & flenp1 with guardpt  */
        if (ff->flen <= 0 || ff->flen > MAXLEN) {
          fterror(csound, ff, Str("illegal table length"));
          return;
        }
        for (ltest=ff->flen,lobits=0; (ltest & MAXLEN)==0; lobits++,ltest<<=1);
        if (ltest != MAXLEN) {                  /*   flen must be power-of-2 */
          fterror(csound, ff, Str("illegal table length"));
          return;
        }
        ff->lenmask = ff->flen-1;
      }
      ftp = ftalloc(csound);                    /*   alloc ftable space now */
      ftp->flen     = ff->flen;
      ftp->lenmask  = ff->lenmask;              /*   init hdr w powof2 data */
      ftp->lobits   = lobits;
      lomod         = MAXLEN / ff->flen;
      ftp->lomask   = lomod - 1;
      ftp->lodiv    = FL(1.0)/((MYFLT)lomod);   /*    & other useful vals    */
      ff->tpdlen    = TWOPI_F / ff->flen;
      ftp->nchanls  = 1;                        /*    presume mono for now   */
      ftp->flenfrms = ff->flen;
    }
    else if (genum != 1 && genum != 23 && genum != 28) {
      /* else defer alloc to gen01|gen23|gen28 */
      fterror(csound, ff, Str("deferred size for GEN1 only"));
      return;
    }
    csound->Message(csound, Str("ftable %d:\n"), ff->fno);
    (*csound->gensub[genum])(ftp, csound);

    if (!ff->fterrcnt && ftp){
      /* VL 11.01.05 for deferred GEN01, it's called in gen01raw */
      ftresdisp(csound, ff, ftp);               /* rescale and display */
    }
}

static void gen02(FUNC *ftp, ENVIRON *csound)
{                               /* read ftable values directly from p-args */
    FGDATA  *ff = &(csound->ff);
    MYFLT   *fp = ftp->ftable, *pp = &(ff->e.p[5]);
    int     nvals = ff->e.pcnt-1;

    if (nvals > ff->flenp1)
      nvals = ff->flenp1;                      /* for all vals up to flen+1 */
    do  {
      *fp++ = *pp++;                           /*   copy into ftable   */
    } while (--nvals);
}

static void gen03(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int         ncoefs, nargs = ff->e.pcnt-4;
    MYFLT       xintvl, xscale;
    int         xloc, nlocs;
    MYFLT       *fp = ftp->ftable, x, sum, *coefp, *coef0, *coeflim;

    if ((ncoefs = nargs - 2) <= 0) {
      fterror(csound, ff, Str("no coefs present"));
      return;
    }
    coef0 = &ff->e.p[7];
    coeflim = coef0 + ncoefs;
    if ((xintvl = ff->e.p[6] - ff->e.p[5]) <= 0) {
      fterror(csound, ff, Str("illegal x interval"));
      return;
    }
    xscale = xintvl / (MYFLT)ff->flen;
    xloc = (int)(ff->e.p[5] / xscale);         /* initial xloc */
    nlocs = ff->flenp1;
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
}

static void gen04(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   *valp, *rvalp, *fp = ftp->ftable;
    int     n, r;
    FUNC    *srcftp;
    MYFLT   val, max, maxinv;
    int     srcno, srcpts, ptratio;

    if (ff->e.pcnt < 6) {
      fterror(csound, ff, Str("insufficient arguments"));
      return;
    }
    if ((srcno = (int)ff->e.p[5]) <= 0 || srcno > csound->maxfnum ||
        (srcftp = csound->flist[srcno]) == NULL) {
      fterror(csound, ff, Str("unknown srctable number")); return;
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
      fterror(csound, ff, Str("table size too large")); return;
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
}

static void gen05(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int     nsegs, seglen;
    MYFLT   *valp, *fp, *finp;
    MYFLT   amp1, mult;

    if ((nsegs = (ff->e.pcnt-5) >> 1) <= 0)    /* nsegs = nargs-1 /2 */
      return;
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
        if (fp > finp) return;
      }
    } while (--nsegs);
    if (fp == finp)                 /* if 2**n pnts, add guardpt */
      *fp = amp1;
    return;

 gn5er1:
    fterror(csound, ff, Str("gen call has negative segment size:"));
    return;
 gn5er2:
    fterror(csound, ff, Str("illegal input vals for gen call, beginning:"));
    return;
}

static void gen07(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int     nsegs, seglen;
    MYFLT   *valp, *fp, *finp;
    MYFLT   amp1, incr;

    if ((nsegs = (ff->e.pcnt-5) >> 1) <= 0)         /* nsegs = nargs-1 /2 */
      return;
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
        if (fp > finp) return;
      }
    } while (--nsegs);
    if (fp == finp)                 /* if 2**n pnts, add guardpt */
      *fp = amp1;
    return;

 gn7err:
    fterror(csound, ff, Str("gen call has negative segment size:"));
    return;
}

static void gen06(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   *segp, *extremp, *inflexp, *segptsp, *fp, *finp;
    MYFLT   y, diff2;
    int     pntno, pntinc, nsegs, npts;

    if ((nsegs = ((ff->e.pcnt-5) >>1)) < 1) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    fp = ftp->ftable;
    finp = fp + ff->flen;
    pntinc = 1;
    for (segp = &ff->e.p[3]; nsegs > 0; nsegs--) {
      segp += 2;
      segptsp = segp + 1;
      if ((npts = (int)*segptsp) < 0) {
        fterror(csound, ff, Str("negative segsiz")); return;
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
}

static void gen08(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   R, x, c3, c2, c1, c0, *fp, *fplim, *valp;
    MYFLT   f2 = FL(0.0), f1, f0, df1, df0, dx01, dx12 = FL(0.0), curx;
    MYFLT   slope, resd1, resd0;
    int     nsegs, npts;

    if ((nsegs = (ff->e.pcnt-5) >>1) <= 0) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    valp = &ff->e.p[5];
    fp = ftp->ftable;
    fplim = fp + ff->flen;
    f0 = *valp++;                    /* 1st 3 params give vals at x0, x1 */
    if ((dx01 = *valp++) <= FL(0.0)) {      /*      and dist between     */
     fterror(csound, ff, Str("illegal x interval")); return;
    }
    f1 = *valp++;
    curx = df0 = FL(0.0);           /* init x to origin; slope at x0 = 0 */
    do {                            /* for each spline segmnt (x0 to x1) */
      if (nsegs > 1) {                      /* if another seg to follow  */
        MYFLT dx02;
        if ((dx12 = *valp++) <= FL(0.0)) {  /*    read its distance      */
         fterror(csound, ff, Str("illegal x interval")); return;
        }
        f2 = *valp++;                       /*    and the value at x2    */
        dx02 = dx01 + dx12;
        df1 = ( f2*dx01*dx01 + f1*(dx12-dx01)*dx02 - f0*dx12*dx12 )
          / (dx01*dx02*dx12);
      }                                /* df1 is slope of parabola at x1 */
      else df1 = FL(0.0);
      if ((npts = (int)(dx01 - curx)) > fplim - fp)
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
}

static void gen09(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int     hcnt;
    MYFLT   *valp, *fp, *finp;
    double  phs, inc, amp;

    if ((hcnt = (ff->e.pcnt-4) / 3) <= 0)            /* hcnt = nargs / 3 */
      return;
    valp = &ff->e.p[5];
    finp = &ftp->ftable[ff->flen];
    do {
      for (inc=(*valp++)*ff->tpdlen, amp=(*valp++),
             phs=(*valp++)*tpd360, fp=ftp->ftable; fp<=finp; fp++) {
        *fp += (MYFLT)(sin(phs) * amp);
        if ((phs += inc) >= TWOPI)
          phs -= TWOPI;
      }
    } while (--hcnt);
}

static void gen10(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    long    phs, hcnt;
    MYFLT   amp, *fp, *finp;

    if ((hcnt = ff->e.pcnt-4) <= 0)                  /* hcnt is nargs   */
      return;
    finp = &ftp->ftable[ff->flen];
    do {
      if ((amp = ff->e.p[hcnt+4]) != 0)               /* for non-0 amps,  */
      for (phs=0, fp=ftp->ftable; fp<=finp; fp++) {
        *fp += (MYFLT)sin(phs*ff->tpdlen) * amp;        /* accum sin pts  */
        phs += hcnt;                                   /* phsinc is hno   */
        phs %= ff->flen;                           /* phs &= ff->lenmask; */
      }
    }
    while (--hcnt);
}

static void gen11(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT  *fp, *finp;
    long   phs;
    double  x;
    MYFLT   denom, r, scale;
    int     n, k;
    int nargs = ff->e.pcnt - 4;

    if (ff->e.pcnt < 5) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    if ((n = (int)ff->e.p[5]) < 1) {
      fterror(csound, ff, Str("nh partials < 1")); return;
    }
    k = 1;
    r = FL(1.0);
    if (ff->e.pcnt > 5)
      k = (int)ff->e.p[6];
    if (nargs > 2)
      r = ff->e.p[7];
    fp = ftp->ftable;
    finp = fp + ff->flen;
    if (ff->e.pcnt == 5 || (k == 1 && r == FL(1.0))) {  /* simple "buzz" case */
      int tnp1;
      MYFLT pdlen;

      tnp1  = (n << 1) + 1;
      scale = FL(0.5) / n;
      pdlen = (MYFLT)ff->tpdlen * FL(0.5);
      for (phs = 0; fp <= finp; phs++) {
        x = phs * pdlen;
        if (!(denom = (MYFLT)sin(x)))
          *fp++ = FL(1.0);
        else *fp++ = ((MYFLT)sin(tnp1 * x) / denom - FL(1.0)) * scale;
      }
    }
    else {                                   /* complex "gbuzz" case */
      MYFLT numer, twor, rsqp1, rtn, rtnp1, absr;
      int   km1, kpn, kpnm1;

      km1   = k - 1;
      kpn   = k + n;
      kpnm1 = kpn - 1;
      twor  = r * FL(2.0);
      rsqp1 = r * r + FL(1.0);
      rtn   = intpow(r, (long) n);
      rtnp1 = rtn * r;
      if ((absr = (MYFLT)fabs(r)) > FL(0.999) && absr < FL(1.001))
        scale = FL(1.0) / n;
      else scale = (FL(1.0) - absr) / (FL(1.0) - (MYFLT)fabs(rtn));
      for (phs=0; fp <= finp; phs++) {
        x = phs * ff->tpdlen;
        numer = (MYFLT)cos(x*k) - r * (MYFLT)cos(x*km1) - rtn*(MYFLT)cos(x*kpn)
                + rtnp1 * (MYFLT)cos(x*kpnm1);
        if ((denom = rsqp1 - twor*(MYFLT)cos(x)) > FL(0.0001)
            || denom < -FL(0.0001))
          *fp++ = numer / denom * scale;
        else *fp++ = FL(1.0);
      }
    }
}

static void gen12(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    static const double coefs[] = { 3.5156229, 3.0899424, 1.2067492,
                                     0.2659732, 0.0360768, 0.0045813 };
    double *coefp, sum, tsquare, evenpowr, *cplim = (double*) coefs + 6;
    int    n;
    MYFLT   *fp;
    double xscale;

    if (ff->e.pcnt < 5) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    xscale = (double) ff->e.p[5] / ff->flen / 3.75;
    for (n=0,fp=ftp->ftable; n<=ff->flen; n++) {
      tsquare  = (double) n * xscale;
      tsquare *= tsquare;
      for (sum=evenpowr=1.0, coefp = (double*) coefs; coefp<cplim; coefp++) {
        evenpowr *= tsquare;
        sum += *coefp * evenpowr;
      }
      *fp++ = (MYFLT) log(sum);
    }
}

static void gen13(FUNC *ftp, ENVIRON *csound)
{
    gn1314(ftp, csound, FL(2.0), FL(0.5));
}

static void gen14(FUNC *ftp, ENVIRON *csound)
{
    gn1314(ftp, csound, FL(1.0), FL(1.0));
}

static void gn1314(FUNC *ftp, ENVIRON *csound, MYFLT mxval, MYFLT mxscal)
{
    long        nh, nn;
    MYFLT       *mp, *mspace, *hp, *oddhp;
    MYFLT       xamp, xintvl, scalfac, sum, prvm;
    FGDATA      *ff = &(csound->ff);

    if ((nh = ff->e.pcnt - 6) <= 0) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    if ((xintvl = ff->e.p[5]) <= 0) {
      fterror(csound, ff, Str("illegal xint value")); return;
    }
    if ((xamp = ff->e.p[6]) <= 0) {
      fterror(csound, ff, Str("illegal xamp value")); return;
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
    gen03(ftp, csound);                         /* then call gen03 to write */
}

static void gen15(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT       xint, xamp, hsin[PMAX/2], h, angle;
    MYFLT       *fp, *cosp, *sinp;
    int n, nh;
    long        *lp, *lp13;
    int         nargs = ff->e.pcnt -4;

    if (nargs & 01) {
      fterror(csound, ff, Str("uneven number of args")); return;
    }
    nh = (nargs - 2) >>1;
    fp   = &ff->e.p[5];                                /* save p5, p6  */
    xint = *fp++;
    xamp = *fp++;
    for (n = nh, cosp = fp, sinp = hsin; n > 0; n--) {
      h = *fp++;                                  /* rpl h,angle pairs */
      angle = (MYFLT)(*fp++ * tpd360);
      *cosp++ = h * (MYFLT)cos((double)angle);    /* with h cos angle */
      *sinp++ = h * (MYFLT)sin((double)angle);    /* and save the sine */
    }
    nargs -= nh;
    gen13(ftp, csound);                         /* call gen13   */
    if (ff->fterrcnt) return;
    ftresdisp(csound, ff, ftp);                 /* and display fno   */
    lp13 = (long *)ftp;
    ff->fno++;                                  /* alloc eq. space for fno+1 */
    ftp = ftalloc(csound);
    for (lp = (long *)ftp; lp < (long *)ftp->ftable; )  /* & copy header */
      *lp++ = *lp13++;
    fp    = &ff->e.p[5];
    *fp++ = xint;                                 /* restore p5, p6,   */
    *fp++ = xamp;
    for (n = nh-1, sinp = hsin+1; n > 0; n--)     /* then skip h0*sin  */
      *fp++ = *sinp++;                            /* & copy rem hn*sin */
    nargs--;
    gen14(ftp, csound);                           /* now draw ftable   */
}

static void gen16(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT *fp, *valp, val;
    int nargs = ff->e.pcnt-4;
    int nseg = nargs/3;

    fp = ftp->ftable;
    valp = &ff->e.p[5];
    *fp++ = val = *valp++;
    while (nseg-- > 0) {
      MYFLT dur    = *valp++;
      MYFLT alpha  = *valp++;
      MYFLT nxtval = *valp++;
      long cnt = (long)(dur + FL(0.5));
      if (alpha == FL(0.0)) {
        MYFLT c1 = (nxtval-val)/dur;
        while (cnt-->0) {
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
}

static void gen17(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int     nsegs, ndx, nxtndx;
    MYFLT   *valp, *fp, *finp;
    MYFLT   val;
    int nargs = ff->e.pcnt -4;

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
        if (fp > finp) return;
      } while (++ndx < nxtndx);
    }
    val = *valp;
    while (fp <= finp)                    /* include 2**n + 1 guardpt */
      *fp++ = val;
    return;

 gn17err:
    fterror(csound, ff, Str("gen call has illegal x-ordinate values:"));
}

static void gen18(FUNC *ftp, ENVIRON *csound)
{  /* by pete moss (petemoss@petemoss.org), jan 2002 */
    FGDATA  *ff = &(csound->ff);
    int cnt, start, finish, fnlen, j;
    MYFLT *pp = &ff->e.p[5], fn, amp, *fp, *fp18 = ftp->ftable, range, f;
    double i;
    FUNC *fnp;
    int nargs = ff->e.pcnt -4;

    if ((cnt = nargs >> 2) <= 0) {
      fterror(csound, ff, Str("wrong number of args"));
      return;
    }
    while (cnt--) {
      fn=*pp++, amp=*pp++, start=(int)*pp++, finish=(int)*pp++;

      if ((start>ff->flen) || (finish>ff->flen)) {
        /* make sure start and finish < flen */
        fterror(csound, ff, Str("a range given exceeds table length"));
        return;
      }

      if ((fnp = csoundFTFind(csound,&fn)) != NULL) { /* make sure fn exists */
        fp = fnp->ftable, fnlen = fnp->flen-1;        /* and set it up */
      }
      else {
        fterror(csound, ff, Str("an input function does not exist"));
        return;
      }

      range = (MYFLT)(finish - start), j = start;
      while (j++ <= finish) {                      /* write the table */
        f = (MYFLT)modf((fnlen*(j-start)/range), &i);
        *(fp18 + j) += amp * ((f * (*(fp + (int)(i+1)) -
                                    *(fp + (int)i))) +
                              *(fp + (int)i));
      }
    }
}

static void gen19(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int     hcnt;
    MYFLT   *valp, *fp, *finp;
    double  phs, inc, amp, dc;
    int nargs = ff->e.pcnt -4;

    if ((hcnt = nargs / 4) <= 0)            /* hcnt = nargs / 4 */
      return;
    valp = &ff->e.p[5];
    finp = &ftp->ftable[ff->flen];
    do {
      for (inc=(*valp++)*ff->tpdlen, amp=(*valp++),
             phs=(*valp++)*tpd360, dc=(*valp++),
             fp=ftp->ftable; fp<=finp; fp++) {
        *fp += (MYFLT)(sin(phs) * amp + dc);   /* dc after str scale */
        if ((phs += inc) >= TWOPI)
          phs -= TWOPI;
      }
    } while (--hcnt);
}

/*  GEN20 and GEN21 by Paris Smaragdis 1994 B.C.M. Csound development team  */
static void gen20(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT cf[4], *ft;
    double arg, i, xarg, beta = 0.0;
    int nargs = ff->e.pcnt -4;

    ft = ftp->ftable;
    xarg = 1.0;

    if (ff->e.p[4] < FL(0.0)) {
      xarg = ff->e.p[6];
      if ( nargs < 2 ) xarg = 1.0;
    }

    if (nargs > 2)
      beta = (double)ff->e.p[7];

    switch ((int)ff->e.p[5])  {
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
        for (i = 0.0 ; i < ff->flen/2.0 ; i++)
            *ft++ = (MYFLT)(i*arg*xarg);
        for (; i < ff->flen ; i++)
            *ft++ = (MYFLT)((2.0 - i*arg)*xarg);
        return;
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
        arg = 12.0/ff->flen;
        for (i = -6.0 ; i < 0.0 ; i += arg)
          *ft++ = (MYFLT)(xarg * (pow( 2.71828, -(i*i)/2.0)));
        for (i = arg ; i < 6.0 ; i += arg)
          *ft++ = (MYFLT)(xarg * (pow( 2.71828, -(i*i)/2.0)));
        return;
    case 7:                     /* Kaiser */
      {
        double flen2 = (double)ff->flen/2.0;
        double flenm12 = (double)(ff->flen-1)*(ff->flen-1);
        double besbeta = besseli( beta);
        for (i = -flen2 + 0.1 ; i < flen2 ; i++)
          *ft++ = (MYFLT)(xarg * besseli((beta*sqrt(1.0-i*i/flenm12)))/besbeta);
        return;
      }
    case 8:                     /* Rectangular */
        for (i = 0 ; i < ff->flen ; i++)
          *ft++ = FL(1.0);
        return;
    case 9:                     /* Sinc */
        arg = TWOPI / ff->flen;
        for (i = -PI ; i < 0 ; i += arg)
          *ft++ = (MYFLT)(xarg * sin(i) / i);
        *ft++ = (MYFLT)xarg;
        for (i = arg ; i < PI ; i += arg)
          *ft++ = (MYFLT)(xarg * sin(i) / i);
        return;
    default:
        fterror(csound, ff, Str("No such window!"));
        return;
    }

    arg = TWOPI/ff->flen;

    for (i = 0.0 ; i < TWOPI ; i += arg)
      *ft++ = (MYFLT)(xarg * (cf[0] - cf[1]*cos(i) +
                              cf[2]*cos(2.0*i) - cf[3]*cos(3.0*i)));
}

static void gen21(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    long i;
    MYFLT *ft;
    MYFLT scale;
    int nargs = ff->e.pcnt -4;

    ft = ftp->ftable;

    if (nargs < 1)  {           /* All need at least 1 argument */
      fterror(csound, ff, Str("Wrong number of input arguments\n"));
      return;
    }
    if (nargs ==1) scale = FL(1.0);
    else scale = ff->e.p[6];

    switch ((int)ff->e.p[5])  {
    case 1:                     /* Uniform distribution */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = unifrand(scale);
      break;
    case 2:                     /* Linear distribution */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = linrand(  scale);
      break;
    case 3:                     /* Triangular about 0.5 */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = trirand(  scale);
      break;
    case 4:                     /* Exponential */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = exprand(  scale);
      break;
    case 5:                     /* Bilateral exponential */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = biexprand(  scale);
      break;
    case 6:                     /* Gaussian distribution */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = gaussrand(  scale);
      break;
    case 7:                     /* Cauchy distribution */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = cauchrand(  scale);
      break;
    case 8:                     /* Positive Cauchy */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = pcauchrand(  scale);
      break;
    case 9:                     /* Beta distribution */
      if (nargs < 3)  {
        fterror(csound, ff, Str("Wrong number of input arguments\n"));
        return;
      }
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = betarand(scale,(MYFLT)ff->e.p[7],(MYFLT)ff->e.p[8]);
      break;
    case 10:                    /* Weibull Distribution */
      if (nargs < 2)  {
        fterror(csound, ff, Str("Wrong number of input arguments\n"));
        return;
      }
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = weibrand(  scale, (MYFLT) ff->e.p[7]);
      break;
    case 11:                    /* Poisson Distribution */
      for (i = 0 ; i < ff->flen ; i++)
        *ft++ = poissrand(  scale);
      break;
    default:
      fterror(csound, ff, Str("unknown distribution\n"));
    }
}

static void gen23(FUNC *ftp, ENVIRON *csound)
                                /* ASCII file table read Gab 17-feb-98*/
                                /* Modified after Paris Smaragdis by JPff */
{
    FGDATA  *ff = &(csound->ff);
    int         c = 0, j = 0;
    char        buf[512], *p;
    MYFLT       *fp;
    FILE        *infile;

    if (!(infile = fopenin(csound, ff->e.strarg))) {
      fterror(csound, ff, Str("error opening ASCII file")); return;
    }
    p = buf;
    if (ftp==NULL) {
        /* Start counting elements */
      ff->flen = 0;
      while ((c = getc(infile)) != EOF) {
        if (!isspace(c)) {
          if (c == ';') {
            while ((c = getc(infile)) != '\n') ;
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
          while (isspace(c=getc(infile))) ;
          ungetc(c,infile);
          p = buf;
        }
      }
      csound->Message(csound, Str("%ld elements in %s\n"),
                              ff->flen, ff->e.strarg);
      rewind(infile);
      /* Allocate memory and read them in now */
      ff->flen    = ff->flen+2;
      ff->lenmask = ff->flen;
      ff->flenp1  = ff->flen+2;
      ftp = (FUNC *) mcalloc(csound, sizeof(FUNC) + ff->flen * sizeof(MYFLT));
      csound->flist[ff->fno]   = ftp;
      ftp->flen    = ff->flen;
      ftp->lenmask = ff->flen;
    }
    fp = ftp->ftable;
    p = buf;
    while ((c= getc(infile)) != EOF && j < ff->flen) {
      if (!isspace(c)) {
        if (c == ';') {
          while ((c= getc(infile)) != '\n') ;
        }
        else *p++ = c;
      }
      else {
        char pp;                /* To save value */
        *p = '\0';
        for (p = buf; (pp = *p) != '\0'; p++) {
          if (!isdigit(pp) && pp != '-' && pp != '.' && pp != '\0')
            goto next ;
        }
        *fp++ = (MYFLT) atof (buf);
        j++;
      next:
        while (isspace(c=getc(infile))) ;
        ungetc(c,infile);
        p = buf;
      }
    }
    fclose(infile);
}


static void gen24(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT       *fp = ftp->ftable, *fp_source;
    FUNC        *srcftp;
    int srcno, srcpts, j;
    MYFLT max, min, new_max, new_min, source_amp, target_amp, amp_ratio;
    int nargs = ff->e.pcnt -4;

    if (nargs < 3) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    if ((srcno = (int)ff->e.p[5]) <= 0 ||
        srcno > csound->maxfnum        ||
        (srcftp = csound->flist[srcno]) == NULL) {
      fterror(csound, ff, Str("unknown srctable number")); return;
    }
    fp_source = srcftp->ftable;

    new_min = ff->e.p[6];
    new_max = ff->e.p[7];
    srcpts = srcftp->flen;
    if (srcpts!= ff->flen) {
      fterror(csound, ff, Str("table size must be the same of source table"));
      return;
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
}


static void gen25(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int nsegs,  seglen;
    MYFLT       *valp, *fp, *finp;
    MYFLT       x1, x2, y1, y2, mult;
    int nargs = ff->e.pcnt -4;

    if ((nsegs = ((nargs / 2) - 1) )<= 0) return;
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
        if (fp > finp) return;
      }
      valp -= 2;
    } while (--nsegs);
    if (fp == finp)                     /* if 2**n pnts, add guardpt */
      *fp = y1;
    return;

 gn25err:
    fterror(csound, ff, Str("x coordindates must all be in increasing order:"));
    return;

 gn25err2:
    fterror(csound, ff, Str("x coordindate greater than function size:"));
    return;

 gn25err3:
    fterror(csound, ff,
            Str("illegal input val (y <= 0) for gen call, beginning:"));
    return;
}

static void gen27(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    int nsegs;
    MYFLT       *valp, *fp, *finp;
    MYFLT       x1, x2, y1, y2, seglen, incr;
    int nargs = ff->e.pcnt -4;

    if ((nsegs = ((nargs / 2) - 1) )<= 0) return;
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
        if (fp > finp) return;
      }
      valp -= 2;
    } while (--nsegs);
    if (fp == finp)                     /* if 2**n pnts, add guardpt */
      *fp = y1;
    return;

 gn27err:
    fterror(csound, ff, Str("x coordindates must all be in increasing order:"));
    return;
 gn27err2:
    fterror(csound, ff, Str("x coordindate greater than function size:"));
    return;
}

static void gen28(FUNC *ftp, ENVIRON *csound)
{                       /* read X Y values directly from ascii file */
    FGDATA  *ff = &(csound->ff);
    MYFLT       *fp = ftp->ftable, *finp;
    int         seglen, resolution = 100;
    FILE        *filp;
    char        filename[256];
    int         i=0, j=0;
    MYFLT       *x, *y, *z;
    int         arraysize = 1000;
    MYFLT       x1, y1, z1, x2, y2, z2, incrx, incry;

    finp = fp + ff->flen;
    strcpy(filename,ff->e.strarg);
    if ((filp = fopenin(csound, filename)) == NULL) goto gen28err1;

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

    ff->flen = (long)(z[i]*resolution*2);
    ff->flen = ff->flen+2;
    ff->lenmask=ff->flen;
    ff->flenp1=ff->flen+2;
    ftp=NULL;
    mfree(csound, (char *)ftp);         /*   release old space   */
    ftp = (FUNC *) mcalloc(csound, (long)sizeof(FUNC) + ff->flen*sizeof(MYFLT));
    csound->flist[ff->fno] = ftp;
    ftp->flen = ff->flen;
    ftp->lenmask=ff->flen;
    fp = ftp->ftable;
    finp = fp + ff->flen;

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
    fclose(filp);

    return;

 gen28err1:
    fterror(csound, ff, Str("could not open space file"));
    return;
 gen28err2:
    fterror(csound, ff, Str("Time values must be in increasing order"));
    return;
}

/* gen30: extract a range of harmonic partials from source table */

static void gen30(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   *x, *f1, *f2;
    int     l1, l2, minh = 0, maxh = 0, i;
    MYFLT   xsr, minfrac, maxfrac;
    int     nargs = ff->e.pcnt - 4;

    if (nargs < 3) {
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
    }
    xsr = FL(1.0);
    if ((nargs > 3) && (ff->e.p[8] > FL(0.0)))
      xsr = csound->esr / ff->e.p[8];
    f2 = csound->GetTable(csound, (int) ff->e.p[5], &l2);
    if (f2 == NULL) {
      fterror(csound, ff, Str("GEN30: source ftable not found"));
      return;
    }
    f1 = &(ftp->ftable[0]);
    l1 = (int) ftp->flen;
    minfrac = ff->e.p[6];           /* lowest harmonic partial number */
    maxfrac = ff->e.p[7] * xsr;     /* highest harmonic partial number */
    i = (l1 < l2 ? l1 : l2) >> 1;   /* sr/2 limit */
    /* limit to 0 - sr/2 range */
    if ((maxfrac < FL(0.0)) || (minfrac > (MYFLT) i))
      return;
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
      return;
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
}

/* gen31: transpose, phase shift, and mix source table */

static void gen31 (FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   *x, *y, *f1, *f2;
    MYFLT   a, p;
    double  d_re, d_im, p_re, p_im, ptmp;
    int     i, j, k, n, l1, l2;
    int     nargs = ff->e.pcnt - 4;

    if (nargs < 4) {
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
    }
    f2 = csound->GetTable(csound, (int) ff->e.p[5], &l2);
    if (f2 == NULL) {
      fterror(csound, ff, Str("GEN31: source ftable not found"));
      return;
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
      p = ff->e.p[j++];                                     /* phase */
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
}

/* gen32: transpose, phase shift, and mix source tables */

static void gen32(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   *x, *y, *f1, *f2;
    MYFLT   a, p;
    double  d_re, d_im, p_re, p_im, ptmp;
    int     i, j, k, n, l1, l2, ntabl, *pnum, ft;
    int     nargs = ff->e.pcnt - 4;

    if (nargs < 4) {
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
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
        fterror(csound, ff, Str("GEN32: source ftable %d not found"), abs(i));
        mfree(csound, pnum);
        if (x != NULL) mfree(csound, x);
        if (y != NULL) mfree(csound, x);
        return;
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
}

/* GEN33 by Istvan Varga */

static void gen33 (FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   fmode, *ft, *srcft, scl, amp, phs;
    MYFLT   *x;
    int     nh, flen, srclen, i, pnum, maxp;
    int     nargs = ff->e.pcnt - 4;

    if (nargs < 3) {
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
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
      fterror(csound, ff, Str("GEN33: source ftable not found"));
      return;
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
}

/* GEN34 by Istvan Varga */

static void gen34 (FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   fmode, *ft, *srcft, scl;
    double  y0, y1, x, c, v, *xn, *cn, *vn, *tmp, amp, frq, phs;
    long    nh, flen, srclen, i, j, k, l, bs;
    FUNC    *src;
    int nargs = ff->e.pcnt -4;

    if (nargs < 3) {
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
    }
    if (nargs > 3)      /* check optional argument */
      fmode = ff->e.p[8];
    else
      fmode = FL(0.0);
    /* table length and data */
    ft = ftp->ftable; flen = (long) ftp->flen;
    /* source table */
    if ((src = csoundFTFind(csound,&(ff->e.p[5]))) == NULL) return;
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
}



static void gen40(FUNC *ftp, ENVIRON *csound)              /*gab d5*/
{
    FGDATA  *ff = &(csound->ff);
    MYFLT       *fp = ftp->ftable, *fp_source, *fp_temp;
    FUNC        *srcftp;
    int         srcno, srcpts, j,k;
    MYFLT       last_value = FL(0.0), lenratio;

    if ((srcno = (int)ff->e.p[5]) <= 0 ||
        srcno > csound->maxfnum        ||
        (srcftp = csound->flist[srcno]) == NULL) {
      fterror(csound, ff, Str("unknown source table number")); return;
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
}

static void gen41(FUNC *ftp, ENVIRON *csound)  /*gab d5*/
{
    FGDATA  *ff = &(csound->ff);
    MYFLT       *fp = ftp->ftable, *pp = &ff->e.p[5];
    int         j, k, width;
    long        tot_prob=0;
    int nargs = ff->e.pcnt -4;

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
}


static void gen42(FUNC *ftp, ENVIRON *csound) /*gab d5*/
{
    FGDATA  *ff = &(csound->ff);
    MYFLT       *fp = ftp->ftable, *pp = &ff->e.p[5], inc;
    int         j, k, width;
    long        tot_prob=0;
    int nargs = ff->e.pcnt -4;

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
}

static void fterror(ENVIRON *csound, FGDATA *ff, char *s, ...)
{
    va_list args;

    csound->Message(csound, Str("ftable %d: "), ff->fno);
    va_start(args, s);
    csound->MessageV(csound, s, args);
    va_end(args);
    csound->Message(csound, "\n");
    csound->Message(csound, "f%3.0f %8.2f %8.2f ",
                            ff->e.p[1], ff->e.p2orig, ff->e.p3orig);
    if (ff->e.p[4] == SSTRCOD)
      csound->Message(csound, "%s", ff->e.strarg);
    else
      csound->Message(csound, "%8.2f", ff->e.p[4]);
    if (ff->e.p[5] == SSTRCOD)
      csound->Message(csound, "  \"%s\" ...\n",ff->e.strarg);
    else
      csound->Message(csound, "%8.2f ...\n",ff->e.p[5]);
    ff->fterrcnt++;
}

static void ftresdisp(ENVIRON *csound, FGDATA *ff, FUNC *ftp)
{                       /* set guardpt, rescale the function, and display it */
    MYFLT       *fp, *finp = &ftp->ftable[ff->flen];
    MYFLT       abs, maxval;
    static      WINDAT  dwindow;        /* Why is this static? */

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
    sprintf(csound->strmsg, Str("ftable %d:"), ff->fno);
    dispset(&dwindow, ftp->ftable, (long) (ff->flen + ff->guardreq),
            csound->strmsg, 0, "ftable");
    display(&dwindow);
}

static FUNC *ftalloc(ENVIRON *csound)
                            /* alloc ftable space for fno (or replace one) */
{                           /*  set ftp to point to that structure      */
    FGDATA  *ff = &(csound->ff);
    FUNC *ftp;
    if ((ftp = csound->flist[ff->fno]) != NULL) {
      if (csound->oparms->msglevel & WARNMSG)
        csound->Message(csound, Str("replacing previous ftable %d\n"), ff->fno);
      if (ff->flen != ftp->flen) {          /* if redraw & diff len, */
        mfree(csound, (char *)ftp);         /*   release old space   */
        csound->flist[ff->fno] = NULL;
        if (csound->actanchor.nxtact != NULL) { /*   & chk for danger    */
          csound->Warning(csound, Str("ftable %d relocating due to size "
                                      "change\ncurrently active instruments "
                                      "may find this disturbing"), ff->fno);
        }
      }
      else {                            /* else clear it to zero */
        MYFLT   *fp = ftp->ftable;
        MYFLT   *finp = &ftp->ftable[ff->flen];
        while (fp <= finp)
          *fp++ = FL(0.0);
      }
    }
    if ((ftp = csound->flist[ff->fno]) == NULL) {   /*   alloc space as reqd */
      size_t  nBytes = sizeof(FUNC) + (size_t) ff->flen * sizeof(MYFLT);
      csound->DebugMsg(csound, Str("Allocating %ld bytes"), (long) nBytes);
      ftp = (FUNC*) mcalloc(csound, nBytes);
      csound->flist[ff->fno] = ftp;
    }
    return ftp;
}

/* find the ptr to an existing ftable structure */
/*   called by oscils, etc at init time         */

FUNC *csoundFTFind(void *csound, MYFLT *argp)
{
    ENVIRON *cs = (ENVIRON*) csound;
    FUNC    *ftp;
    int     fno;

    if ((fno = (int)*argp) <= 0 ||
        fno > cs->maxfnum       ||
        (ftp = cs->flist[fno]) == NULL) {
      csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    else if (!ftp->lenmask) {
      csoundInitError(csound,
                      Str("deferred-size ftable %f illegal here"), *argp);
      return NULL;
    }
    return(ftp);
}

MYFLT *csoundGetTable(void *csound_, int tableNum, int *tableLength)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    FUNC    *ftp;

    if (tableLength != NULL)
      *tableLength = -1;
    if (tableNum <= 0 || tableNum > csound->maxfnum)
      return NULL;
    ftp = csound->flist[tableNum];
    if (ftp == NULL)
      return NULL;
    if (!ftp->lenmask)
      return NULL;
    if (tableLength != NULL)
      *tableLength = (int) ftp->flen;
    return &(ftp->ftable[0]);
}

/*************************************/
/* csoundFTFindP()
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
FUNC *csoundFTFindP(void *csound_, MYFLT *argp)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    FUNC    *ftp;
    int     fno = csound->ff.fno;

    /* Check limits, and then index  directly into the flist[] which
     * contains pointers to FUNC data structures for each table.
     */
    if ((fno = (int)*argp) <= 0 ||
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
    return(ftp);
}

FUNC *csoundFTnp2Find(void *csound_, MYFLT *argp)
   /* find ptr to a deferred-size ftable structure */
   /*   called by loscil at init time, and ftlen   */
{
    ENVIRON *csound = (ENVIRON*) csound_;
    FUNC    *ftp;
    char    strarg[SSTRSIZ];
    EVTBLK  evt;
    FGDATA  *ff = &(csound->ff);

    if ((ff->fno = (int)*argp) <= 0 ||
        ff->fno > csound->maxfnum   ||
        (ftp = csound->flist[ff->fno]) == NULL) {
      csoundInitError(csound, Str("Invalid ftable no. %f"), *argp);
      return NULL;
    }
    if (ftp->flen == 0) {
      /* The soundfile hasn't been loaded yet, so call GEN01 */
      ff->flen = 0;
      ff->e = evt;
      ff->e.p[4] = ftp->gen01args.gen01;
      ff->e.p[5] = ftp->gen01args.ifilno;
      ff->e.p[6] = ftp->gen01args.iskptim;
      ff->e.p[7] = ftp->gen01args.iformat;
      ff->e.p[8] = ftp->gen01args.channel;
      strcpy(strarg,ftp->gen01args.strarg);
      ff->e.strarg = strarg;
      gen01raw(ftp, csound);
    }
    return (ftp);
}

static void gen01(FUNC *ftp, ENVIRON *csound)
                                /* read ftable values from a sound file */
{                               /* stops reading when table is full     */
    FGDATA  *ff = &(csound->ff);
    int nargs = ff->e.pcnt -4;
    if (nargs < 4) {
      fterror(csound, ff, Str("insufficient arguments")); return;
    }
    if (csound->oparms->gen01defer) {
      /* We're deferring the soundfile load until performance time,
         so allocate the function table descriptor, save the arguments,
         and get out */
      ftp = ftalloc(csound);
      ftp->gen01args.gen01 = ff->e.p[4];
      ftp->gen01args.ifilno = ff->e.p[5];
      ftp->gen01args.iskptim = ff->e.p[6];
      ftp->gen01args.iformat = ff->e.p[7];
      ftp->gen01args.channel = ff->e.p[8];
      strcpy(ftp->gen01args.strarg,ff->e.strarg);
      ftp->flen = ff->flen;
      return;
    }
    gen01raw(ftp, csound);
}

static void needsiz(ENVIRON *csound, FGDATA *ff, long maxend)
{
    long nxtpow;
    maxend -= 1; nxtpow = 2;
    while (maxend >>= 1)
      nxtpow <<= 1;
    csound->Message(csound, Str("non-deferred ftable %d needs size %ld\n"),
                            (int) ff->fno, nxtpow);
}

static void gen01raw(FUNC *ftp, ENVIRON *csound)
                                /* read ftable values from a sound file */
{                               /* stops reading when table is full     */
    FGDATA  *ff = &(csound->ff);
    SOUNDIN *p;                     /*   for sndgetset      */
    AIFFDAT *adp;
    SOUNDIN tmpspace;               /* create temporary opds */
    SNDFILE *fd;
    int     truncmsg = 0;
    long    inlocs = 0;
    int def=0;

    p = &tmpspace;
    memset(p, 0, sizeof(SOUNDIN));
    {
      long  filno = (long) RNDINT(ff->e.p[5]);
      int   fmt = (int) RNDINT(ff->e.p[7]);
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
          fterror(csound, ff, Str("invalid sample format: %d"), fmt);
          return;
      }
    }
    p->skiptime = ff->e.p[6];
    p->channel  = (int) RNDINT(ff->e.p[8]);
    p->do_floatscaling = 0;
    if (p->channel < 0 /* || p->channel > ALLCHNLS-1 */) {
      fterror(csound, ff, Str("channel %d illegal"), (int) p->channel);
      return;
    }
    if (p->channel == 0)                      /* snd is chan 1,2,..8 or all */
      p->channel = ALLCHNLS;
    p->analonly = 0;
    if (ff->flen == 0)
      csound->Message(csound, Str("deferred alloc\n"));
    if ((fd = sndgetset(csound, p))==NULL) {  /* sndinset to open the file */
      fterror(csound, ff, "Failed to open file"); return;
    }
    if (ff->flen==0) {                      /* deferred ftalloc requestd: */
      if ((ff->flen = p->framesrem) <= 0) { /*   get minsize from soundin */
        fterror(csound, ff, Str("deferred size, but filesize unknown")); return;
      }
      csound->Message(csound, Str("**** defer length %ld\n"), (long) ff->flen);
      if (p->channel == ALLCHNLS)
        ff->flen *= p->nchanls;
      ff->guardreq  = 1;
      ff->flenp1    = ff->flen;              /* presum this includes guard */
      ff->flen     -= 1;
      ftp           = ftalloc(csound);       /*   alloc now, and           */
      ftp->flen     = ff->flen;
      ftp->lenmask  = 0;                     /*   mark hdr partly filled   */
      ftp->nchanls  = p->nchanls;
      ftp->flenfrms = ff->flen / p->nchanls; /* ?????????? */
      def           = 1;
    }
    ftp->gen01args.sample_rate = csound->curr_func_sr;
    ftp->cvtbas = LOFACT * p->sr * csound->onedsr;
    if ((adp = p->aiffdata) != NULL) {       /* if file was aiff,    */
      csound->Message(csound, Str("AIFF case\n"));
      /* set up some necessary header stuff if not in aiff file */
      if (adp->natcps == 0)                  /* from Jeff Fried      */
        adp->natcps = ftp->cvtbas;
      if (adp->gainfac == 0)
        adp->gainfac = FL(1.0);
      ftp->cpscvt = ftp->cvtbas / adp->natcps;  /*    copy data to FUNC */
      ftp->loopmode1 = adp->loopmode1;          /* (getsndin does gain) */
      ftp->loopmode2 = adp->loopmode2;
      ftp->begin1 = adp->begin1;
      ftp->begin2 = adp->begin2;
      if (ftp->loopmode1)                    /* Greg Sullivan */
        ftp->end1 = adp->end1;
      else
        ftp->end1 = ftp->flenfrms;
      ftp->end1 = adp->end1;
      ftp->end2 = adp->end2;
      if (ftp->end1 > ff->flen || ftp->end2 > ff->flen) {
        long maxend;
        csound->Warning(csound,
                        Str("GEN1: input file truncated by ftable size"));
        if ((maxend = ftp->end1) < ftp->end2)
          maxend = ftp->end2;
        csound->Message(csound,
                        Str("\tlooping endpoint %ld exceeds ftsize %ld\n"),
                        (long) maxend, (long) ff->flen);
        needsiz(csound, ff, maxend);
        truncmsg = 1;
      }
    }
    else {
      ftp->cpscvt = FL(0.0);      /* else no looping possible   */
      ftp->loopmode1 = 0;
      ftp->loopmode2 = 0;
      ftp->end1 = ftp->flenfrms;  /* Greg Sullivan */
    }
    /* read sound with opt gain */
    if ((inlocs = getsndin(csound, fd, ftp->ftable, ff->flenp1, p)) < 0) {
      fterror(csound, ff, Str("GEN1 read error"));
      return;
    }
    if (p->audrem > 0 && !truncmsg && p->framesrem > ff->flen) {
      /* Reduce msg */
      csound->Warning(csound, Str("GEN1: aiff file truncated by ftable size"));
      csound->Warning(csound, Str("\taudio samps %ld exceeds ftsize %ld"),
                              (long) p->framesrem, (long) ff->flen);
      needsiz(csound, ff, p->framesrem);     /* ????????????  */
    }
    ftp->soundend = inlocs / ftp->nchanls;   /* record end of sound samps */
    sf_close(fd);
    if (def)
      ftresdisp(csound, ff, ftp); /* VL: 11.01.05  for deferred alloc tables */
}

#define FTPLERR(s)     {fterror(csound, ff, s); \
                        csound->Die(csound, Str("ftable load error"));\
                        return(NULL);}

/* create ftable using evtblk data */

FUNC *hfgens(ENVIRON *csound, EVTBLK *evtblkp)
{
    long    ltest, lobits, lomod, genum;
    FUNC    *ftp = NULL;
    FGDATA *ff = &(csound->ff);

    ff->e = *evtblkp;
    ff->fterrcnt = 0;
    if ((ff->fno = (int)ff->e.p[1]) < 0) {         /* fno < 0: remove */
      if ((ff->fno = -ff->fno) > csound->maxfnum) {
        int size = csound->maxfnum+1;
        FUNC **nn;
        int i;
        while (ff->fno >= size) size += MAXFNUM;
        nn = (FUNC**)mrealloc(csound, csound->flist, size*sizeof(FUNC*));
        csound->flist = nn;
        for (i=csound->maxfnum+1; i<size; i++)
          csound->flist[i] = NULL; /* Clear new section */
        csound->maxfnum = size-1;
      }
      if ((ftp = csound->flist[ff->fno]) == NULL)
        FTPLERR(Str("ftable does not exist"))
      csound->flist[ff->fno] = NULL;
      mfree(csound, (char *)ftp);
      csound->Message(csound, Str("ftable %d now deleted\n"), ff->fno);
      return(NULL);                       /**************/
    }
    if (!ff->fno) {                 /* fno = 0, automatic number */
      do {
        ff->fno = ++csound->ftldno;
      } while (ff->fno<csound->maxfnum && csound->flist[ff->fno]!=NULL);
      if (ff->fno==csound->maxfnum) {
        int size = csound->maxfnum+1;
        FUNC **nn;
        int i;
        size += MAXFNUM;
        nn = (FUNC**)mrealloc(csound, csound->flist, size*sizeof(FUNC*));
        csound->flist = nn;
        for (i=csound->maxfnum+1; i<size; i++)
          csound->flist[i] = NULL; /* Clear new section */
        csound->maxfnum = size-1;
      }
      ff->e.p[1] = (MYFLT)(ff->fno);
    }
    if (ff->fno > csound->maxfnum) {
        int size = csound->maxfnum+1;
        FUNC **nn;
        int i;
        while (ff->fno >= size) size += MAXFNUM;
        nn = (FUNC**)mrealloc(csound, csound->flist, size*sizeof(FUNC*));
        csound->flist = nn;
        for (i=csound->maxfnum+1; i<size; i++)
          csound->flist[i] = NULL; /* Clear new section */
        csound->maxfnum = size-1;
    }
    if (ff->e.pcnt - 4 <= 0)                    /* chk minimum arg count    */
      FTPLERR(Str("insufficient gen arguments"))
    if ((genum = (long)ff->e.p[4]) < 0)
      genum = -genum;
    if (!genum || genum > GENMAX)               /*   & legal gen number     */
      FTPLERR(Str("illegal gen number"))
    if ((ff->flen = (long)ff->e.p[3]) != 0) {   /* if user flen given       */
      ff->guardreq = ff->flen & 01;             /*   set guard request flg  */
      ff->flen &= -2;                           /*   flen now w/o guardpt   */
      ff->flenp1 = ff->flen + 1;                /*   & flenp1 with guardpt  */
      if (ff->flen <= 0 || ff->flen > MAXLEN)
        FTPLERR(Str("illegal table length"))
      for (ltest=ff->flen,lobits=0; (ltest & MAXLEN) == 0; lobits++,ltest<<=1);
      if (ltest != MAXLEN)                      /* flen must be power-of-2  */
        FTPLERR(Str("illegal table length"))
      ff->lenmask = ff->flen-1;
      ftp = ftalloc(csound);                    /*   alloc ftable space now */
      ftp->fno = ff->fno;
      ftp->flen = ff->flen;
      ftp->lenmask = ff->lenmask;               /*   init hdr w powof2 data */
      ftp->lobits = lobits;
      lomod = MAXLEN / ff->flen;
      ftp->lomask = lomod - 1;
      ftp->lodiv = FL(1.0)/((MYFLT)lomod);      /*    & other useful vals   */
      ff->tpdlen = TWOPI / ff->flen;
      ftp->nchanls = 1;                         /*    presume mono for now  */
      ftp->flenfrms = ff->flen;         /* Is this necessary?? */
    }
    else if (genum != 1 && genum != 23 && genum != 28)
      /* else defer alloc to gen01|gen23|gen28 */
      FTPLERR(Str("deferred size for GEN1 only"))
    csound->Message(csound, Str("ftable %d:\n"), ff->fno);
    if (csound->gensub==NULL) {
      csound->gensub = (GEN*)mmalloc(csound, csound->genmax*sizeof(GEN));
      memcpy(csound->gensub, or_sub, sizeof(or_sub));
    }
    (*csound->gensub[genum])(ftp, csound);      /* call gen subroutine  */

    if (!ff->fterrcnt)
      ftresdisp(csound, ff, ftp);               /* rescale and display */
    return(ftp);
}

int ftgen(ENVIRON *csound, FTGEN *p) /* set up and call any GEN routine */
{
    int nargs;
    MYFLT *fp;
    FUNC *ftp;
    EVTBLK *ftevt;

    ftevt = (EVTBLK *)mcalloc(csound, sizeof(EVTBLK) + FTPMAX * sizeof(MYFLT));
    ftevt->opcod = 'f';
    ftevt->strarg = NULL;
    fp = &ftevt->p[1];
    *fp++ = *p->p1;                             /* copy p1 - p5 */
    *fp++ = ftevt->p2orig = FL(0.0);            /* force time 0 */
    *fp++ = ftevt->p3orig = *p->p3;
    *fp++ = *p->p4;
    if (p->XSTRCODE) {                        /* string argument: */
      int n = (int) ftevt->p[4];
      *fp++ = SSTRCOD;
      if (n < 0) n = -n;
      if (n == 1 || n == 23 || n == 28) {       /*   must be Gen01, 23 or 28 */
        ftevt->strarg = (char*) p->p5;
      }
      else {
        mfree(csound, ftevt);
        return csoundInitError(csound, Str("ftgen string arg not allowed"));
      }
    }
    else {
      *fp++ = *p->p5;
      ftevt->strarg = NULL;                     /* else no string */
    }
    if ((nargs = p->INOCOUNT - 5) > 0) {
      MYFLT **argp = p->argums;
      while (nargs--)                           /* copy rem arglist */
        *fp++ = **argp++;
    }
    ftevt->pcnt = p->INOCOUNT;
    if ((ftp = hfgens(csound, ftevt)) != NULL)  /* call the fgen */
      *p->ifno = (MYFLT)ftp->fno;               /* record the fno */
    else if (ftevt->p[1] >=0) {
      mfree(csound, ftevt);
      return csoundInitError(csound, Str("ftgen error"));
    }
    mfree(csound, ftevt);
    return OK;
}

int ftload(ENVIRON *csound, FTLOAD *p)
{
    MYFLT **argp = p->argums;
    FUNC  *ftp;
    char filename[MAXNAME];
    int nargs;
    FILE *file;

    if ((nargs = p->INOCOUNT - 2) <= 0) goto err2;

    if (p->XSTRCODE)                      /* if char string name given */
      strcpy(filename, (char*) p->ifilno);  /* FIXME: and what if not ? */
    if (*p->iflag <= 0) {
      if (!(file = fopen(filename, "rb"))) goto err3;
      while (nargs--)  {
        FUNC header;
        FGDATA *ff = &(p->h.insdshead->csound->ff);

        ff->fno = (int) **argp;
        fread(&header, sizeof(FUNC)-sizeof(MYFLT)-SSTRSIZ, 1, file);
        /* ***** Need to do byte order here ***** */
        ff->flen = header.flen;
        header.fno = ff->fno;
        if ((ftp = csoundFTFind(csound,*argp)) != NULL) {
          MYFLT *table = ftp->ftable;
          memcpy(ftp, &header, sizeof(FUNC)-sizeof(MYFLT)-SSTRSIZ);
          ftp = ftalloc(csound);
          fread(table, sizeof(float), ff->flen, file);
          /* ***** Need to do byte order here ***** */
        }
        else goto err;
        argp++;
      }
    }
    else {
      if (!(file = fopen(filename, "r"))) goto err3;
      while (nargs--) {
        FUNC header;
        char s[64], *s1;
        FGDATA *ff = &(csound->ff);

        ff->fno = (int) **argp;
        /* IMPORTANT!! If FUNC structure and/or GEN01ARGS structure
           will be modified, the following code has to be modified too */
        fgets(s, 64, file);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.flen = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lenmask = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lobits = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lomask = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lodiv = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.cvtbas = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.cpscvt = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.loopmode1 = (short) atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.loopmode2 = (short) atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.begin1 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.end1 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.begin2 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.end2 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.soundend = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.flenfrms = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.nchanls = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.fno = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.gen01 = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.ifilno = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.iskptim = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.iformat = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.channel = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.sample_rate = (MYFLT)atof(s1);
        fgets(s, 64, file);
        /* WARNING! skips header.gen01args.strarg from saving/loading
           in text format */
        ff->flen = header.flen;
        header.fno = ff->fno;
        if ((ftp = csoundFTFind(csound,*argp)) != NULL) {
          long j;
          MYFLT *table = ftp->ftable;
          memcpy(ftp, &header, sizeof(FUNC)-sizeof(MYFLT));
          ftp = ftalloc(csound);
          for (j=0; j < ff->flen; j++) {
            fgets(s, 64, file);
            table[j] = (MYFLT)atof(s);
          }
          fgets(s, 64, file);
        }
        else goto err;
        argp++;
      }
    }
    fclose(file);
    return OK;
 err:
    return csoundInitError(csound,
                           Str("ftload: Bad table number. Loading is possible "
                               "only into existing tables."));
 err2:
    return csoundInitError(csound, Str("ftload: no table numbers"));
 err3:
    return csoundInitError(csound, Str("ftload: unable to open file"));
}

int ftload_k(ENVIRON *csound, FTLOAD_K *p)
{
    FTLOAD *pp = &(p->p);
    if (*p->ktrig)
      ftload(csound, pp);
    return OK;
}


int ftsave(ENVIRON *csound, FTLOAD *p)
{
    MYFLT **argp = p->argums;
    char filename[MAXNAME];
    int nargs;
    FILE *file;

    if ((nargs = p->INOCOUNT - 2) <= 0) goto err2;

    if (p->XSTRCODE)                      /* if char string name given */
      strcpy(filename, (char*) p->ifilno);  /* FIXME: and what if not ? */
    if (*p->iflag <= 0) {
      if (!(file = fopen(filename, "wb"))) goto err3;
      while (nargs--) {
        FUNC *ftp;

        if ((ftp = csoundFTFind(csound,*argp)) != NULL) {
          MYFLT *table = ftp->ftable;
          long flen = ftp->flen;
          fwrite(ftp, sizeof(FUNC)-sizeof(MYFLT)-SSTRSIZ, 1, file);
          fwrite(table, sizeof(MYFLT), flen, file);
        }
        else
          goto err;
        argp++;
      }
    }
    else {
      if (!(file = fopen(filename, "w"))) goto err3;
      while (nargs--)  {
        FUNC *ftp;

        if ((ftp = csoundFTFind(csound,*argp)) != NULL) {
          long flen = ftp->flen;
          long j;
          MYFLT *table = ftp->ftable;
          /* IMPORTANT!! If FUNC structure and/or GEN01ARGS structure
             will be modified, the following code has to be modified too */
          fprintf(file,"======= TABLE %ld size: %ld values ======\n",
                  ftp->fno, ftp->flen);
          fprintf(file,"flen: %ld\n", ftp->flen);
          fprintf(file,"lenmask: %ld\n", ftp->lenmask);
          fprintf(file,"lobits: %ld\n",ftp->lobits);
          fprintf(file,"lomask: %ld\n",ftp->lomask);
          fprintf(file,"lodiv: %f\n",ftp->lodiv);
          fprintf(file,"cvtbas: %f\n",ftp->cvtbas);
          fprintf(file,"cpscvt: %f\n",ftp->cpscvt);
          fprintf(file,"loopmode1: %d\n", (int) ftp->loopmode1);
          fprintf(file,"loopmode2: %d\n", (int) ftp->loopmode2);
          fprintf(file,"begin1: %ld\n",ftp->begin1);
          fprintf(file,"end1: %ld\n",ftp->end1);
          fprintf(file,"begin2: %ld\n",ftp->begin2);
          fprintf(file,"end2: %ld\n",ftp->end2);
          fprintf(file,"soundend: %ld\n",ftp->soundend);
          fprintf(file,"flenfrms: %ld\n",ftp->flenfrms);
          fprintf(file,"nchnls: %ld\n",ftp->nchanls);
          fprintf(file,"fno: %ld\n",ftp->fno);

          fprintf(file,"gen01args.gen01: %f\n",ftp->gen01args.gen01);
          fprintf(file,"gen01args.ifilno: %f\n",ftp->gen01args.ifilno);
          fprintf(file,"gen01args.iskptim: %f\n",ftp->gen01args.iskptim);
          fprintf(file,"gen01args.iformat: %f\n",ftp->gen01args.iformat);
          fprintf(file,"gen01args.channel: %f\n",ftp->gen01args.channel);
          fprintf(file,"gen01args.sample_rate: %f\n",
                       ftp->gen01args.sample_rate);
          /* WARNING! skips ftp->gen01args.strarg from saving/loading in
             text format */
          fprintf(file,"---------END OF HEADER--------------\n");

          for (j=0; j < flen; j++) {
            MYFLT val = table[j];
            fprintf(file,"%f\n",val);
          }
          fprintf(file,"---------END OF TABLE---------------\n");
        }
        else goto err;
        argp++;
      }
    }
    fclose(file);
    return OK;
 err:
    return csoundInitError(csound,
                           Str("ftsave: Bad table number. Saving is possible "
                               "only for existing tables."));
 err2:
    csoundInitError(csound, Str("ftsave: no table numbers"));
    return NOTOK;
 err3:
    csoundInitError(csound, Str("ftsave: unable to open file"));
    return NOTOK;
}

int ftsave_k_set(ENVIRON *csound, FTLOAD_K *p)
{
    memcpy(&(p->p.h), &(p->h), sizeof(OPDS));
    p->p.INOCOUNT = p->INOCOUNT -1;
    p->p.ifilno = p->ifilno;
    p->p.iflag = p->iflag;
    memcpy( p->p.argums, p->argums, sizeof(MYFLT*)*p->INOCOUNT - 3);
    return OK;
}

int ftsave_k(ENVIRON *csound, FTLOAD_K *p)
{
    FTLOAD *pp = &(p->p);
    if (*p->ktrig)
      ftsave(csound, pp);
    return OK;
}

/* GEN 43 (c) Victor Lazzarini, 2004 */

#include "pstream.h"
#include "pvfileio.h"

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

/* lifted almost straight from Richard Dobson's code */

static int pvx_loadfile_mem(ENVIRON *csound,
                            const char *fname,PVSTABLEDAT *p, MEMFIL **mfp)
{
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MEMFIL *mfil = NULL;
    int i,j,rc = 0,pvx_id = -1;
    long pvx_fftsize,pvx_winsize;
    long mem_wanted = 0;
    long totalframes,framelen;
    float *pFrame;
    float *memblock = NULL;
    pv_wtype wtype;

    pvx_id = pvoc_openfile(fname,&pvdata,&fmt);
    if (pvx_id < 0) {
      sprintf(csound->errmsg, Str("unable to open pvocex file %s.\n"), fname);
      return 0;
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    pvx_fftsize = 2 * (pvdata.nAnalysisBins-1);
    framelen = 2 * pvdata.nAnalysisBins;
    /* no need to impose Csound limit on fftsize here */
    pvx_winsize = pvdata.dwWinlen;

    /* also, accept only 32bit floats for now */
    if (pvdata.wWordFormat != PVOC_IEEE_FLOAT){
      sprintf(csound->errmsg, Str("pvoc-ex file %s is not 32bit floats\n"),
                              fname);
      return 0;
    }

    /* FOR NOW, accept only PVOC_AMP_FREQ : later, we can convert */
    /* NB Csound knows no other: frameFormat is not read anywhere! */
    if (pvdata.wAnalFormat != PVOC_AMP_FREQ){
      sprintf(csound->errmsg, Str("pvoc-ex file %s not in AMP_FREQ format\n"),
                              fname);
      return 0;
    }

    /* ignore the window spec until we can use it! */
    totalframes = pvoc_framecount(pvx_id);
    if (totalframes == 0){
      sprintf(csound->errmsg, Str("pvoc-ex file %s is empty!\n"), fname);
      return 0;
    }

    if (!find_memfile(csound, fname, &mfil)){
      mem_wanted = totalframes * 2 * pvdata.nAnalysisBins * sizeof(float);
      /* try for the big block first! */

      memblock = (float *) mmalloc(csound, mem_wanted);

      pFrame = memblock;
      /* despite using pvocex infile, and pvocex-style resynth, we ~still~
         have to rescale to Csound's internal range! This is because all pvocex
         calculations assume +-1 floatsam i/o.
         It seems preferable to do this here, rather than force the user
         to do so. Csound might change one day... */

      for (i=0;i < totalframes;i++){
        rc = pvoc_getframes(pvx_id,pFrame,1);
        if (rc != 1)
          break;          /* read error, but may still have something to use */
        /* scale amps to Csound range, to fit fsig */
        for (j=0;j < framelen; j+=2) {
          pFrame[j] *= (float) csound->e0dbfs;
        }
        pFrame += framelen;
      }
      if (rc <0){
        sprintf(csound->errmsg, Str("error reading pvoc-ex file %s\n"), fname);
        mfree(csound, memblock);
        return 0;
      }
      if (i < totalframes){
        sprintf(csound->errmsg,
                Str("error reading pvoc-ex file %s after %d frames\n"),
                fname, i);
        mfree(csound, memblock);
        return 0;
      }
    }
    else
      memblock = (float *) mfil->beginp;

    pvoc_closefile(pvx_id);

    p->fftsize  = pvx_fftsize;
    p->winsize  = pvx_winsize;
    p->overlap  = pvdata.dwOverlap;
    p->chans    = fmt.nChannels;
    p->frames = (unsigned) totalframes;
    wtype = (pv_wtype) pvdata.wWindowType;
    switch (wtype) {
    case PVOC_DEFAULT:
    case PVOC_HAMMING:
      p->wintype = PVS_WIN_HAMMING;
      break;
    case PVOC_HANN:
      p->wintype = PVS_WIN_HANN;
      break;
    default:
      /* deal with all other possibilities later! */
      p->wintype = PVS_WIN_HAMMING;
      break;
    }

    /* Need to assign an MEMFIL to mfp */
    if (mfil==NULL){
      mfil = (MEMFIL *)  mmalloc(csound, sizeof(MEMFIL));
      /* just hope the filename is short enough...! */
      mfil->next = NULL;
      mfil->filename[0] = '\0';
      strcpy(mfil->filename,fname);
      mfil->beginp = (char *) memblock;
      mfil->endp = mfil->beginp + mem_wanted;
      mfil->length = mem_wanted;
      /*from memfiles.c */
      csoundMessage(csound, Str("file %s (%ld bytes) loaded into memory\n"),
                            fname, mem_wanted);
      add_memfil(csound, mfil);
    }

    *mfp = mfil;
    return 1;
}

void gen43(FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);

    MYFLT *fp = ftp->ftable;
    MYFLT *filno;
    int nvals = ff->e.pcnt - 4;
    MYFLT *channel;
    char     filename[MAXNAME];
    MEMFIL  *mfp;
    PVSTABLEDAT  p;
    unsigned long framesize, blockalign, bins;
    unsigned long frames, i, j;
    float* framep,* startp;
    double accum = 0.0;

    if (nvals != 2) {
      fterror(csound, ff, Str("wrong number of ftable arguments"));
      return;
    }

    filno = &ff->e.p[5];
    if (*filno == SSTRCOD) {
      strcpy(filename, (char *)(&ff->e.strarg[0]));
    }
    else if ((long) *filno < csound->strsmax && csound->strsets != NULL &&
             csound->strsets[(long) *filno])
      strcpy(filename, csound->strsets[(long)*filno]);
    else sprintf(filename,"pvoc.%d", (int)*filno); /* pvoc.filnum   */
    if (!pvx_loadfile_mem(csound, filename, &p, &mfp))
      csoundDie(csound, csound->errmsg);

    channel = &ff->e.p[6];

    if (*channel > p.chans) fterror(csound, ff, Str("illegal channel number"));

    framesize = p.fftsize+1;
    bins = framesize/2;
    frames = p.frames;

    if (*channel > 0 ) {
      startp = (float *) (mfp->beginp + (p.fftsize+2) * ((int)*channel-1));
      blockalign = (p.fftsize+2) * p.chans; /* only read one channel */
    }
    else {
      startp = (float *) mfp->beginp;
      blockalign = (p.fftsize+2);  /* read all channels */
    }

    framep = startp;

    if (bins > (unsigned long) (ftp->flen+1)) {
      fterror(csound, ff, Str("ftable size too small"));
      return;
    }

    for (i=0; i<framesize; i+=2) {
      for (j=0; j < frames; j++, framep += blockalign) {
        accum += framep[i];
      }
      fp[i/2] = (MYFLT)accum/frames;
      framep = startp;
      accum = 0.0;
    }
}

static void gen51(FUNC *ftp, ENVIRON *csound)   /* Gab 1/3/2005 */
{
    int   j, notenum, grade, numgrades, basekeymidi, nvals;
    MYFLT basefreq, factor, interval;
    MYFLT *fp = ftp->ftable, *pp;

    nvals       = csound->ff.flenp1 - 1;
    pp          = &(csound->ff.e.p[5]);
    numgrades   = (int) *pp++;
    interval    = *pp++;
    basefreq    = *pp++;
    basekeymidi = (int) *pp++;

    if ((csound->ff.e.pcnt - 8) != numgrades) {
      fterror(csound, &(csound->ff), Str("gen51: invalid number of p-fields"));
      return;
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
}

static void gen52 (FUNC *ftp, ENVIRON *csound)
{
    FGDATA  *ff = &(csound->ff);
    MYFLT   *src, *dst;
    FUNC    *f;
    int     nchn, len, len2, i, j, k, n;
    int     nargs = (int) ff->e.pcnt - 4;

    if (nargs < 4) {
      fterror(csound, ff, Str("insufficient gen arguments"));
      return;
    }
    nchn = RNDINT(ff->e.p[5]);
    if (((nchn * 3) + 1) != nargs) {
      fterror(csound, ff,
              Str("number of channels inconsistent with number of args"));
      return;
    }
    len = ((int) ftp->flen / nchn) * nchn;
    dst = &(ftp->ftable[0]);
    for (i = len; i <= (int) ftp->flen; i++)
      dst[i] = FL(0.0);
    for (n = 0; n < nchn; n++) {
      f = csoundFTFind(csound, &(ff->e.p[(n * 3) + 6]));
      if (f == NULL)
        return;
      len2 = (int) f->flen;
      src = &(f->ftable[0]);
      i = n;
      j = RNDINT(ff->e.p[(n * 3) + 7]);
      k = RNDINT(ff->e.p[(n * 3) + 8]);
      while (i < len) {
        if (j >= 0 && j < len2)
          dst[i] = src[j];
        else
          dst[i] = FL(0.0);
        i += nchn;
        j += k;
      }
    }
}

int allocgen(ENVIRON *csound, char *s, GEN fn)
{
    NAMEDGEN *n = namedgen;
/*     csound->DebugMsg(csound, Str("**** allocgen %s to %p"), s, fn); */
    while (n!=NULL) {
      if (strcmp(s, n->name)==0) return n->genum;
      n = n->next;
    }
    /* Need to allocate */
    n = (NAMEDGEN*) mmalloc(csound, sizeof(NAMEDGEN));
    n->genum = csound->genmax++;
    n->next = namedgen;
    n->name = mmalloc(csound, strlen(s)+1);
    strcpy(n->name, s);
    namedgen = n;
    if (csound->gensub==NULL) {
      csound->gensub = (GEN*)mmalloc(csound, csound->genmax*sizeof(GEN));
      memcpy(csound->gensub, or_sub, sizeof(or_sub));
    }
    else csound->gensub = (GEN*)mrealloc(csound, csound->gensub,
                                         csound->genmax*sizeof(GEN));
    csound->gensub[csound->genmax-1] = fn;
/*     csound->DebugMsg(csound, Str("**** allocated %d"), csound->genmax-1); */
    return csound->genmax-1;
}

