/*
    ugens4.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"         /*                      UGENS4.C        */
#include "ugens4.h"
#include <math.h>
#include <inttypes.h>

/* The branch prediction slows it down!! */

int32_t bzzset(CSOUND *csound, BUZZ *p)
{
    FUNC        *ftp;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (*p->iphs >= 0)
        p->lphs = (int32_t)(*p->iphs * FL(0.5) * FMAXLEN);
      p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;
      p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
      p->reported = 0;          /* No errors yet */
      return OK;
    }
    return NOTOK;
}

int32_t buzz(CSOUND *csound, BUZZ *p)
{
    FUNC        *ftp;
    MYFLT       *ar, *ampp, *cpsp, *ftbl;
    int32_t       phs, inc, lobits, dwnphs, tnp1, lenmask;
    MYFLT       sicvt2, over2n, scal, num, denom;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    int32_t       nn;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1; /* RWD fix */
    ftbl = ftp->ftable;
    sicvt2 = csound->sicvt * FL(0.5); /* for theta/2  */
    lobits = ftp->lobits;
    lenmask = ftp->lenmask;
    ampp = p->xamp;
    cpsp = p->xcps;
    if ((nn = (int32_t)*p->knh) < 0) nn = -nn;
    if (UNLIKELY(nn == 0)) {     /* fix nn = knh */
      nn = 1;
    }
    tnp1 = (nn<<1) + 1;          /* calc 2n + 1 */
    over2n = FL(0.5) / (MYFLT)nn;
    scal = *ampp * over2n;
    inc = (int32_t)(*cpsp * sicvt2);
    ar = p->ar;
    phs = p->lphs;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      //printf("early=%d\n", early);
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (p->ampcod)
        scal = ampp[n] * over2n;
      if (p->cpscod)
        inc = (int32_t)(cpsp[n] * sicvt2);
      dwnphs = phs >> lobits;
      denom = ftbl[dwnphs];
      if (denom > FL(0.0002) || denom < -FL(0.0002)) {
        num = ftbl[dwnphs * tnp1 & lenmask];
        ar[n] = (num / denom - FL(1.0)) * scal;
      }
      else ar[n] = *ampp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h), Str("buzz: not initialised"));
}

int32_t gbzset(CSOUND *csound, GBUZZ *p)
{
    FUNC        *ftp;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (*p->iphs >= 0) {
        p->lphs = (int32_t)(*p->iphs * FMAXLEN);
        p->prvr = FL(0.0);
      }
      p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;
      p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
      p->reported = 0;
      p->last = FL(1.0);
      return OK;
    }
    return NOTOK;
}

static inline MYFLT intpow1(MYFLT x, int32_t n) /* Binary positive power function */
{
    MYFLT ans = FL(1.0);
    while (n!=0) {
      if (n&1) ans = ans * x;
      n >>= 1;
      x = x*x;
    }
    return ans;
}

MYFLT intpow(MYFLT x, int32_t n)   /* Binary power function */
{
    if (n<0) {
      n = -n;
      x = FL(1.0)/x;
    }
    return intpow1(x, n);
}

int32_t gbuzz(CSOUND *csound, GBUZZ *p)
{
    FUNC        *ftp;
    MYFLT       *ar, *ampp, *cpsp, *ftbl;
    int32_t       phs, inc, lobits, lenmask, k, km1, kpn, kpnm1;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       r, absr, num, denom, scal, last = p->last;
    int32_t       nn, lphs = p->lphs;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    lenmask = ftp->lenmask;
    ampp = p->xamp;
    cpsp = p->xcps;
    k = (int32_t)*p->kk;                   /* fix k and n  */
    if ((nn = (int32_t)*p->kn)<0) nn = -nn;
    if (UNLIKELY(nn == 0)) {              /* n must be > 0 */
      nn = 1;
    }
    km1 = k - 1;
    kpn = k + nn;
    kpnm1 = kpn - 1;
    if ((r = *p->kr) != p->prvr || nn != p->prvn) {
      p->twor = r + r;
      p->rsqp1 = r * r + FL(1.0);
      p->rtn = intpow1(r, nn);
      p->rtnp1 = p->rtn * r;
      if ((absr = FABS(r)) > FL(0.999) && absr < FL(1.001))
        p->rsumr = FL(1.0) / nn;
      else p->rsumr = (FL(1.0) - absr) / (FL(1.0) - FABS(p->rtn));
      p->prvr = r;
      p->prvn = (int16)nn;
    }
    scal =  *ampp * p->rsumr;
    inc = (int32_t)(*cpsp * csound->sicvt);
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (p->ampcod)
        scal =  p->rsumr * ampp[n];
      if (p->cpscod)
        inc = (int32_t)(cpsp[n] * csound->sicvt);
      phs = lphs >>lobits;
      denom = p->rsqp1 - p->twor * ftbl[phs];
      num = ftbl[phs * k & lenmask]
        - r * ftbl[phs * km1 & lenmask]
        - p->rtn * ftbl[phs * kpn & lenmask]
        + p->rtnp1 * ftbl[phs * kpnm1 & lenmask];
      if (LIKELY(denom > FL(0.0002) || denom < -FL(0.0002))) {
        ar[n] = last = num / denom * scal;
      }
      else if (last<0)
        ar[n] = last = - (p->ampcod ? ampp[n] : *ampp);
      else
        ar[n] = last = (p->ampcod ? ampp[n] : *ampp);
      lphs += inc;
      lphs &= PHMASK;
    }
    p->last = last;
    p->lphs = lphs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h), Str("gbuzz: not initialised"));
}

#define PLUKMIN 64

static  int16   rand15(CSOUND *);
static  int16   rand16(CSOUND *);

int plukset(CSOUND *csound, PLUCK *p)
{
    int32_t         n;
    int32_t       npts, iphs;
    char        *auxp;
    FUNC        *ftp;
    MYFLT       *ap, *fp;
    MYFLT       phs, phsinc;

    if (UNLIKELY((npts = (int32_t)(csound->esr / *p->icps)) < PLUKMIN)) {
                                        /* npts is wavelen in sampls */
      npts = PLUKMIN;                   /*  (but at least min size)  */
    }
    if ((auxp = p->auxch.auxp) == NULL ||
        npts > p->maxpts) {     /* get newspace    */
      csound->AuxAlloc(csound, (npts+1)*sizeof(MYFLT),&p->auxch);
      auxp = p->auxch.auxp;
      p->maxpts = npts;                         /*      if reqd    */
    }
    ap = (MYFLT *)auxp;                         /* as MYFLT array   */
    if (*p->ifn == 0.0)
      for (n=npts; n--; )                       /* f0: fill w. rands */
        *ap++ = (MYFLT) rand16(csound) * DV32768;
    else if ((ftp = csound->FTnp2Finde(csound, p->ifn)) != NULL) {
      fp = ftp->ftable;                         /* else from ftable  */
      phs = FL(0.0);
      phsinc = (MYFLT)(ftp->flen/npts);
      for (n=npts; n--; phs += phsinc) {
        iphs = (int32_t)phs;
        *ap++ = fp[iphs];
      }
    }
    *ap = *(MYFLT *)auxp;                       /* last = copy of 1st */
    p->npts = npts;
    /* tuned pitch convt */
    p->sicps = (npts * FL(256.0) + FL(128.0)) * csound->onedsr;
    p->phs256 = 0;
    p->method = (int16)*p->imeth;
    p->param1 = *p->ipar1;
    p->param2 = *p->ipar2;
    switch(p->method) {
    case 1:     /* ignore any given parameters */
      break;
    case 2:     /* stretch factor: param1 >= 1 */
      if (UNLIKELY(p->param1 < FL(1.0)))
        return csound->InitError(csound,
                                 Str("illegal stretch factor(param1) value"));
      else p->thresh1 =  (int16)(FL(32768.0) / p->param1);
      break;
    case 3: /* roughness factor: 0 <= param1 <= 1 */
      if (UNLIKELY(p->param1 < FL(0.0) || p->param1 > FL(1.0)))
        return csound->InitError(csound,
                                 Str("illegal roughness factor(param1) value"));
      else
        p->thresh1 = (int16)(FL(32768.0) * p->param1);
      break;
    case 4: /* rough and stretch factor: 0 <= param1 <= 1, param2 >= 1 */
      if (UNLIKELY(p->param1 < FL(0.0) || p->param1 > FL(1.0)))
        return csound->InitError(csound,
                                 Str("illegal roughness factor(param1) value"));
      else p->thresh1 = (int16)(FL(32768.0) * p->param1);
      if (UNLIKELY(p->param2 < FL(1.0)))
        return csound->InitError(csound,
                                 Str("illegal stretch factor(param2) value"));
      else p->thresh2 = (int16)(FL(32768.0) / p->param2);
      break;
    case 5: /* weighting coeff's: param1 + param2 <= 1 */
      if (UNLIKELY(p->param1 + p->param2 > 1))
        return csound->InitError(csound, Str("coefficients too large "
                                             "(param1 + param2)"));
      break;
    case 6: /* ignore any given parameters */
      break;

    default:
      return csound->InitError(csound, Str("unknown method code"));
    }
    return OK;
}

int32_t pluck(CSOUND *csound, PLUCK *p)
{
    MYFLT       *ar, *fp;
    int32_t       phs256, phsinc, ltwopi, offset;
    uint32_t koffset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       frac, diff;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD FIX */
    ar = p->ar;
    phsinc = (int32_t)(*p->kcps * p->sicps);
    phs256 = p->phs256;
    ltwopi = p->npts << 8;
    if (UNLIKELY(phsinc > ltwopi)) goto err2;
    if (UNLIKELY(koffset)) memset(ar, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=koffset; n<nsmps; n++) {
      offset = phs256 >> 8;
      fp = (MYFLT *)p->auxch.auxp + offset;     /* lookup position   */
      diff = fp[1] - fp[0];
      frac = (MYFLT)(phs256 & 255) / FL(256.0); /*  w. interpolation */
      ar[n] =   (fp[0] + diff*frac) * *p->kamp; /*  gives output val */
      if ((phs256 += phsinc) >= ltwopi) {
        int32_t nn;
        MYFLT   newval, preval;
        phs256 -= ltwopi;               /* at phase wrap,    */
        fp=(MYFLT *)p->auxch.auxp;
        preval = fp[0];                 /*   copy last pnt   */
        fp[0] = fp[p->npts];            /*     to first,     */
        fp++;                           /*   apply smoothing */
        nn = p->npts;                   /*     up to npts+1  */
        switch(p->method) {
        case 1:
          do {                  /* simple averaging */
            newval = (*fp + preval) * FL(0.5);
            preval = *fp;
            *fp++ = newval;
          } while (--nn);
          break;
        case 2:
          do {                  /* stretched avrging */
            if (rand15(csound) < p->thresh1) {
              newval = (*fp + preval) * FL(0.5);
              preval = *fp;
              *fp++ = newval;
            }
            else preval = *fp++;
          } while (--nn);
          break;
        case 3:
          do {                  /* simple drum */
            if (rand15(csound) < p->thresh1)
              newval = -(*fp + preval) * FL(0.5);
            else
              newval = (*fp + preval) * FL(0.5);
            preval = *fp;
            *fp++ = newval;
          } while (--nn);
          break;
        case 4:
          do {                  /* stretched drum */
            if (rand15(csound) < p->thresh2) {
              if (rand15(csound) < p->thresh1)
                newval = -(*fp + preval) * FL(0.5);
              else
                newval = (*fp + preval) * FL(0.5);
              preval = *fp;
              *fp++ = newval;
            }
            else preval = *fp++;
          } while (--nn);
          break;
        case 5:
          do {                  /* weighted avraging */
            newval = p->param1 * *fp + p->param2 * preval;
            preval = *fp;
            *fp++ = newval;
          } while (--nn);
          break;
        case 6:
          do {          /* 1st order recursive filter*/
            preval = (*fp + preval) * FL(0.5);
            *fp++ = preval;
          } while (--nn);
          break;
        }
      }
    }
    p->phs256 = phs256;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h), Str("pluck: not initialised"));
 err2:
    return csound->PerfError(csound, &(p->h),
                             Str("pluck: kcps more than sample rate"));
}

#define RNDMUL  15625
#define MASK16  0xFFFF
#define MASK15  0x7FFF

/* quick generate a random int16 between -32768 and 32767 */

static int16 rand16(CSOUND *csound)
{
    csound->ugens4_rand_16 = (csound->ugens4_rand_16 * RNDMUL + 1) & MASK16;
    return (int16) ((int16_t) csound->ugens4_rand_16);
}

/* quick generate a random int16 between 0 and 32767 */

static int16 rand15(CSOUND *csound)
{
    csound->ugens4_rand_15 = (csound->ugens4_rand_15 * RNDMUL + 1) & MASK15;
    return (int16) csound->ugens4_rand_15;
}

/*=========================================================================
 *
 * randint31()
 *
 * 31 bit Park Miller PRNG using Linus Schrage's method for doing it all
 * with 32 bit variables.
 *
 * Code adapted from Ray Garder's public domain code of July 1997 at:
 * http://www.snippets.org/RG_RAND.C     Thanks!
 *
 *  Based on "Random Number Generators: Good Ones Are Hard to Find",
 *  S.K. Park and K.W. Miller, Communications of the ACM 31:10 (Oct 1988),
 *  and "Two Fast Implementations of the 'Minimal Standard' Random
 *  Number Generator", David G. Carta, Comm. ACM 33, 1 (Jan 1990), p. 87-88
 *
 *  Linear congruential generator: f(z) = (16807 * z) mod (2 ** 31 - 1)
 *
 *  Uses L. Schrage's method to avoid overflow problems.
 */

#define BIPOLAR 0x7FFFFFFF      /* Constant to make bipolar */
#define RIA (16807)             /* multiplier */
#define RIM 0x7FFFFFFFL         /* 2**31 - 1 */

#define dv2_31          (FL(4.656612873077392578125e-10))

int32 randint31(int32 seed31)
{
    uint32 rilo, rihi;

    rilo = RIA * (int32_t)(seed31 & 0xFFFF);
    rihi = RIA * (int32_t)((uint32)seed31 >> 16);
    rilo += (rihi & 0x7FFF) << 16;
    if (rilo > RIM) {
      rilo &= RIM;
      ++rilo;
    }
    rilo += rihi >> 15;
    if (rilo > RIM) {
      rilo &= RIM;
      ++rilo;
    }
    return (int32_t)rilo;
}

int32_t rndset(CSOUND *csound, RAND *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {
      if (*p->iseed > FL(1.0)) {    /* As manual suggest seed in range [0,1] */
        uint32 seed;         /* I reinterpret >1 as a time seed */
        seed = csound->GetRandomSeedFromTime();
        csound->Warning(csound, Str("Seeding from current time %"PRIu32"\n"), seed);
        if (!p->new) {
          p->rand = (int32_t) (seed & 0xFFFFUL);
        }
        else {
          p->rand = (int32_t) (seed % 0x7FFFFFFEUL) + 1L;
        }
      }
      else {
        if (p->new) {
          MYFLT seed = *p->iseed;
          if (seed==FL(0.0)) seed = FL(0.5);
          p->rand = (int32_t) (seed * FL(2147483648.0));
          p->rand = randint31(p->rand);
          p->rand = randint31(p->rand);
        }
        else
          p->rand = ((int16)(*p->iseed * FL(32768.0)))&0xffff;
      }
    }
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krand) */

    return OK;
}

int32_t krand(CSOUND *csound, RAND *p)
{
    IGN(csound);
    if (p->new) {
      int32_t r = randint31(p->rand);      /* result is a 31-bit value */
      p->rand = r;
      *p->ar = *p->base +
        dv2_31 * (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * *p->xamp;
    }
    else {
      int16 rand = (int16)p->rand;
      rand *= RNDMUL;
      rand += 1;
      /* IV - Jul 11 2002 */
      *p->ar = *p->base + (MYFLT)rand * *p->xamp * DV32768;
      p->rand = rand;
    }
    return OK;
}

int32_t arand(CSOUND *csound, RAND *p)
{
    IGN(csound);
    MYFLT       *ar;
    int16       rndmul = RNDMUL;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       ampscl;
    MYFLT       base = *p->base;

    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (!p->new) {
      int16     rand = p->rand;
      if (!(p->ampcod)) {
        ampscl = *p->xamp * DV32768;    /* IV - Jul 11 2002 */
        for (n=offset;n<nsmps;n++) {
          rand *= rndmul;
          rand += 1;
          ar[n] = base + (MYFLT)rand * ampscl;          /* IV - Jul 11 2002 */
        }
      }
      else {
        MYFLT *xamp = p->xamp;
        for (n=offset;n<nsmps;n++) {
          rand *= rndmul;
          rand += 1;
          /* IV - Jul 11 2002 */
          ar[n] = base + (MYFLT)rand * xamp[n] * DV32768;
        }
      }
      p->rand = rand;   /* save current rand */
    }
    else {
      int32_t       rand = p->rand;
      if (!(p->ampcod)) {
        ampscl = *p->xamp * dv2_31;
        for (n=offset;n<nsmps;n++) {
          rand = randint31(rand);
          ar[n] = base + (MYFLT)((int32_t)((uint32_t)rand<<1)-BIPOLAR) * ampscl;
        }
      }
      else {
        MYFLT *xamp = p->xamp;
        for (n=offset;n<nsmps;n++) {
          rand = randint31(rand);
          ar[n] = base +
            dv2_31 * (MYFLT)((int32_t)((uint32_t)rand<<1)-BIPOLAR) * xamp[n];
        }
      }
      p->rand = rand;   /* save current rand */
    }

    return OK;
}

int32_t rhset(CSOUND *csound, RANDH *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {                       /* new seed:            */
      if (*p->iseed > FL(1.0)) {    /* As manual suggest sseed in range [0,1] */
        uint32 seed;         /* I reinterpret >1 as a time seed */
        seed = csound->GetRandomSeedFromTime();
        csound->Warning(csound, Str("Seeding from current time %"PRIu32"\n"), seed);
        if (!p->new) {
          p->rand = (int32_t) (seed & 0xFFFFUL);
          p->num1 = (MYFLT) ((int16) p->rand) * DV32768;
        }
        else {
          p->rand = (int32_t) (seed % 0x7FFFFFFEUL) + 1L;
          p->num1 = (MYFLT) ((int32_t) ((uint32_t)p->rand<<1) - BIPOLAR) * dv2_31;
        }
      }
      else if (!p->new) {
        p->rand = 0xffff&(int16)(*p->iseed * 32768L);   /* init rand integ    */
        p->num1 = *p->iseed;                            /*    store fnum      */
      }
      else {
        MYFLT ss = *p->iseed;
        if (ss>FL(1.0)) p->rand = (int32_t) ss;
        else p->rand = (int32_t) (*p->iseed * FL(2147483648.0));
        p->rand = randint31(p->rand);
        p->rand = randint31(p->rand);
        p->num1 = (MYFLT) ((int32_t) ((uint32_t)p->rand<<1) - BIPOLAR) * dv2_31;
      }
      p->phs = 0;                               /*      & phs           */
    }
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krandh) */
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

int32_t krandh(CSOUND *csound, RANDH *p)
{
    IGN(csound);
    *p->ar = *p->base + p->num1 * *p->xamp;     /* rslt = num * amp     */
    p->phs += (int64_t)(*p->xcps * CS_KICVT); /* phs += inc           */

    if (p->phs >= MAXLEN) {                     /* when phs overflows,  */
      p->phs &= PHMASK;                         /*      mod the phs     */
      if (!p->new) {
        int16 rand = (int16)p->rand;
        rand *= RNDMUL;                         /*      & recalc number */
        rand += 1;
        p->num1 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        p->rand = rand;
      }
      else {
        int32_t r = randint31(p->rand);            /*      & recalc number */
        p->rand = r;
        p->num1 = (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * dv2_31;
      }
    }
    return OK;
}

int32_t randh(CSOUND *csound, RANDH *p)
{
    int64_t      phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *ampp, *cpsp;
    MYFLT       base = *p->base;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    inc = (int64_t)(*cpsp++ * csound->sicvt);
    for (n=offset;n<nsmps;n++) {
      /* IV - Jul 11 2002 */
      ar[n] = base + p->num1 * *ampp;   /* rslt = num * amp */
      if (p->ampcod)
        ampp++;
      phs += inc;                               /* phs += inc       */
      if (p->cpscod)
        inc = (int64_t)(*cpsp++ * csound->sicvt);
      if (phs >= MAXLEN) {                      /* when phs o'flows, */
        phs &= PHMASK;
        if (!p->new) {
          int16 rand = p->rand;
          rand *= RNDMUL;                       /*   calc new number */
          rand += 1;
          p->num1 = (MYFLT)rand * DV32768;      /* IV - Jul 11 2002 */
          p->rand = rand;
        }
        else {
          int32_t r = randint31(p->rand);       /*   calc new number */
          p->rand = r;
          p->num1 = (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * dv2_31;
        }
      }
    }
    p->phs = phs;
    return OK;
}

int32_t riset(CSOUND *csound, RANDI *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {                    /* new seed:            */
      if (*p->iseed > FL(1.0)) { /* As manual suggest seed in range [0,1] */
        uint32 seed;             /* I reinterpret >1 as a time seed */
        seed = csound->GetRandomSeedFromTime();
        csound->Warning(csound, Str("Seeding from current time %"PRIu32"\n"), seed);
        if (!p->new) {
          int16 rand = (int16)seed;
/*           int16 ss = rand; */
          /* IV - Jul 11 2002 */
          p->num1 = (MYFLT)(rand) * DV32768; /* store num1,2 */
          rand *= RNDMUL;         /*      recalc random   */
          rand += 1;
          /* IV - Jul 11 2002 */
          p->num2 = (MYFLT)(p->rand=rand) * DV32768;
/*           printf("seed, rand, num1, num2 = %d(%x), %d*%x), %f, %f\n", */
/*                  ss,ss,p->rand, p->rand, p->num1, p->num2); */
        }
        else {
          p->rand = randint31((int32_t) (seed % 0x7FFFFFFEUL) + 1L);
          p->rand = randint31(p->rand);
          p->num1 = (MYFLT)(p->rand<<1) * dv2_31; /* store num1,2 */
          p->rand = randint31(p->rand);
          p->num2 = (MYFLT)(p->rand<<1) * dv2_31;
        }
      }
      else if (!p->new) {
        int16 rand = (int16)(*p->iseed * FL(32768.0)); /* init rand integ */
        rand *= RNDMUL;                 /*      to 2nd value    */
        rand += 1;
        p->num1 = *p->iseed;                    /*      store num1,2    */
        p->num2 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        p->rand = rand;
      }
      else {
        MYFLT ss = *p->iseed;
        if (ss>FL(1.0)) p->rand = (int32_t) ss;
        else if (ss==FL(0.0)) p->rand = (int32_t) (FL(0.5) * FL(2147483648.0));
        else p->rand = (int32_t) (ss * FL(2147483648.0));
        p->rand = randint31(p->rand);
        p->rand = randint31(p->rand);
        p->num1 = (MYFLT)(p->rand<1) * dv2_31; /* store num1,2 */
        p->rand = randint31(p->rand);
        p->num2 = (MYFLT)(p->rand<<1) * dv2_31;
      }
      p->phs = 0;                               /*      & clear phs     */
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;  /* & diff     */
    }
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krandi) */
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

int32_t krandi(CSOUND *csound, RANDI *p)
{                                       /* rslt = (num1 + diff*phs) * amp */
   IGN(csound);
    *p->ar = *p->base + (p->num1 + (MYFLT)p->phs * p->dfdmax) * *p->xamp;
    p->phs += (int64_t)(*p->xcps * CS_KICVT); /* phs += inc           */
    if (p->phs >= MAXLEN) {                     /* when phs overflows,  */
      p->phs &= PHMASK;                         /*      mod the phs     */
      if (!p->new) {
        int16 rand = p->rand;
        rand *= RNDMUL;                         /*      recalc random   */
        rand += 1;
        p->num1 = p->num2;                      /*      & new num vals  */
        p->num2 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        p->rand = rand;
      }
      else {
        int32_t r = randint31(p->rand);    /*      recalc random   */
        p->rand = r;
        p->num1 = p->num2;              /*      & new num vals  */
        p->num2 = (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * dv2_31;
      }
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

int32_t randi(CSOUND *csound, RANDI *p)
{
    int64_t       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *ampp, *cpsp;
    MYFLT       base = *p->base;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    inc = (int64_t)(*cpsp++ * csound->sicvt);
    for (n=offset;n<nsmps;n++) {
      /* IV - Jul 11 2002 */
      ar[n] = base + (p->num1 + (MYFLT)phs * p->dfdmax) * *ampp;
      if (p->ampcod)
        ampp++;
      phs += inc;                               /* phs += inc       */
      if (p->cpscod)
        inc = (int64_t)(*cpsp++ * csound->sicvt);  /*   (nxt inc)      */
      if (phs >= MAXLEN) {                      /* when phs o'flows, */
        phs &= PHMASK;
        if (!p->new) {
          int16 rand = p->rand;
          rand *= RNDMUL;                       /*   calc new numbers*/
          rand += 1;
          p->num1 = p->num2;
          p->num2 = (MYFLT)rand * DV32768;      /* IV - Jul 11 2002 */
          p->rand = rand;
        }
        else {
          int32_t r = randint31(p->rand);          /*   calc new numbers*/
          //printf("r = %x\n", r);
          p->rand = r;
          p->num1 = p->num2;
          p->num2 = (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * dv2_31;
        }
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    }
    p->phs = phs;
    return OK;
}

/* Cubic interpolation between random values -- JPff 2019 */

int32_t rcset(CSOUND *csound, RANDC *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {                    /* new seed:            */
      if (*p->iseed > FL(1.0)) { /* As manual suggest sseed in range [0,1] */
        uint32 seed;             /* I reinterpret >1 as a time seed */
        seed = csound->GetRandomSeedFromTime();
        csound->Warning(csound, Str("Seeding from current time %"PRIu32"\n"), seed);
        if (!p->new) {
          int16 rand = (int16)seed;
/*           int16 ss = rand; */
          /* IV - Jul 11 2002 */
          p->num1 = (MYFLT)(rand) * DV32768; /* store num1,2 */
          rand *= RNDMUL;         /*      recalc random   */
          rand += 1;
          p->num2 = (MYFLT)(rand) * DV32768;
          rand *= RNDMUL;         /*      recalc random   */
          rand += 1;
          p->num3 = (MYFLT)(rand) * DV32768;
          rand *= RNDMUL;         /*      recalc random   */
          rand += 1;
          p->num4 = (MYFLT)(p->rand=rand) * DV32768;
        }
        else {
          p->rand = randint31((int32_t) (seed % 0x7FFFFFFEUL) + 1L);
          p->rand = randint31(p->rand);
          p->num1 = (MYFLT)(p->rand<<1) * dv2_31; /* store num1,2 */
          p->rand = randint31(p->rand);
          p->num2 = (MYFLT)(p->rand<<1) * dv2_31;
          p->rand = randint31(p->rand);
          p->num3 = (MYFLT)(p->rand<<1) * dv2_31;
          p->rand = randint31(p->rand);
          p->num4 = (MYFLT)(p->rand<<1) * dv2_31;
        }
      }
      else if (!p->new) {
        int16 rand = (int16)(*p->iseed * FL(32768.0)); /* init rand integ */
        rand *= RNDMUL;                 /*      to 2nd value    */
        rand += 1;
        p->num1 = *p->iseed;                    /*      store num1,2    */
        p->num2 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        rand *= RNDMUL;                 /*      to 2nd value    */
        rand += 1;
        p->num3 = (MYFLT)rand * DV32768;
        rand *= RNDMUL;                 /*      to 2nd value    */
        rand += 1;
        p->num4 = (MYFLT)rand * DV32768;
        p->rand = rand;
      }
      else {
        MYFLT ss = *p->iseed;
        if (ss>FL(1.0)) p->rand = (int32_t) ss;
        else p->rand = (int32_t) (*p->iseed * FL(2147483648.0));
        p->rand = randint31(p->rand);
        p->rand = randint31(p->rand);
        p->num1 = (MYFLT)(p->rand<1) * dv2_31; /* store num1,2 */
        p->rand = randint31(p->rand);
        p->num2 = (MYFLT)(p->rand<<1) * dv2_31;
        p->rand = randint31(p->rand);
        p->num3 = (MYFLT)(p->rand<<1) * dv2_31;
        p->rand = randint31(p->rand);
        p->num4 = (MYFLT)(p->rand<<1) * dv2_31;
      }
    }
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krandi) */
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    p->phs = 0;
    return OK;
}


int32_t krandc(CSOUND *csound, RANDC *p)
{                                       /* rslt = (num1 + diff*phs) * amp */
    IGN(csound);
    MYFLT a0         =   p->num4 - p->num3 - p->num1 + p->num2;
    MYFLT a1         =   p->num1 - p->num2 - a0;
    MYFLT a2         =   p->num3 - p->num1;
    MYFLT a3         =   p->num2;
    MYFLT mu         =   (MYFLT)p->phs / (MYFLT)MAXLEN;
    *p->ar = *p->base + (((a0 * mu +a1) * mu+a2) * mu + a3) * *p->xamp;
    p->phs += (int64_t)(*p->xcps * CS_KICVT); /* phs += inc           */
    if (p->phs >= MAXLEN) {                     /* when phs overflows,  */
      p->phs &= PHMASK;                         /*      mod the phs     */
      if (!p->new) {
        int16 rand = p->rand;
        rand *= RNDMUL;                         /*      recalc random   */
        rand += 1;
        p->num1 = p->num2;                      /*      & new num vals  */
        p->num2 = p->num3;
        p->num3 = p->num4;
        p->num4 = (MYFLT)rand * DV32768;
        p->rand = rand;
      }
      else {
        int32_t r = randint31(p->rand);    /*      recalc random   */
        p->rand = r;
        p->num1 = p->num2;              /*      & new num vals  */
        p->num2 = p->num3;
        p->num3 = p->num4;
        p->num4 = (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * dv2_31;
      }
    }
    return OK;
}

int32_t randc(CSOUND *csound, RANDC *p)
{
    int64_t       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *ampp, *cpsp;
    MYFLT       mu;
    MYFLT       base = *p->base;
    MYFLT a0         =   p->num4 - p->num3 - p->num1 + p->num2;
    MYFLT a1         =   p->num1 - p->num2 - a0;
    MYFLT a2         =   p->num3 - p->num1;
    MYFLT a3         =   p->num2;
    cpsp = p->xcps;
    ampp = p->xamp;
    inc = (int64_t)(*cpsp++ * csound->sicvt);
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset;n<nsmps;n++) {

      mu         =   (MYFLT)phs/(MYFLT)MAXLEN;

      ar[n]  =   base + (((a0 * mu +a1) * mu+a2) * mu + a3) * *ampp;

      if (p->ampcod)
        ampp++;
      phs += inc;
      //printf("mu = %g  phs, inc, MAXLEN = %ld, %ld, %d\n", mu, phs, inc, MAXLEN);
      if (p->cpscod)
        inc = (int64_t)(*cpsp++ * csound->sicvt);  /*   (nxt inc)      */
      if (phs >= MAXLEN) {                      /* when phs o'flows, */
        phs &= PHMASK;
        if (!p->new) {
          int16 rand = p->rand;
          rand *= RNDMUL;                       /*   calc new numbers*/
          rand += 1;
          p->num1 = p->num2;                      /*      & new num vals  */
          p->num2 = p->num3;
          p->num3 = p->num4;
          p->num4 = (MYFLT)rand * DV32768;
          p->rand = rand;
        }
        else {
          int32_t r = randint31(p->rand);          /*   calc new numbers*/
          p->rand = r;
          p->num1 = p->num2;
          p->num2 = p->num3;
          p->num3 = p->num4;
          p->num4 = (MYFLT)((int32_t)((uint32_t)r<<1)-BIPOLAR) * dv2_31;
        }
        a0         =   p->num4 - p->num3 - p->num1 + p->num2;
        a1         =   p->num1 - p->num2 - a0;
        a2         =   p->num3 - p->num1;
        a3         =   p->num2;

      }
    }
    p->phs = phs;
    return OK;
}

