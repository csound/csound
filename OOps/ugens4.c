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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"                 /*                      UGENS4.C        */
#include "ugens4.h"
#include <math.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#ifdef WIN32
/* Need to undefine these else MSVC barfs */
#undef u_char
#undef u_short
#undef u_int
#undef u_long
#include <windows.h>
#endif

int bzzset(ENVIRON *csound, BUZZ *p)
{
    FUNC        *ftp;

    if ((ftp = ftfind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
      if (*p->iphs >= 0)
        p->lphs = (long)(*p->iphs * FL(0.5) * FMAXLEN);
      p->ampcod = (XINARG1) ? 1 : 0;
      p->cpscod = (XINARG2) ? 1 : 0;
      p->reported = 0;          /* No errors yet */
      return OK;
    }
    return NOTOK;
}

int buzz(ENVIRON *csound, BUZZ *p)
{
    FUNC        *ftp;
    MYFLT       *ar, *ampp, *cpsp, *ftbl;
    long        phs, inc, lobits, dwnphs, tnp1, lenmask, nn;
    MYFLT       sicvt2, over2n, scal, num, denom;
    int n;

    ftp = p->ftp;
    if (ftp==NULL) {        /* RWD fix */
      return perferror(Str(X_624,"buzz: not initialised"));
    }
    ftbl = ftp->ftable;
    sicvt2 = sicvt * FL(0.5);           /* for theta/2  */
    lobits = ftp->lobits;
    lenmask = ftp->lenmask;
    ampp = p->xamp;
    cpsp = p->xcps;
    if ((n = (int)*p->knh) < 0) n = -n;
    if (n == 0) {              /* fix n = knh */
      n = 1;
    }
    tnp1 = (n <<1) + 1;                 /* calc 2n + 1 */
    over2n = FL(0.5) / (MYFLT)n;
    scal = *ampp * over2n;
    inc = (long)(*cpsp * sicvt2);
    ar = p->ar;
    phs = p->lphs;
    nn = ksmps;
    do {
      dwnphs = phs >> lobits;
      denom = *(ftbl+dwnphs);
      if (denom > FL(0.0002) || denom < -FL(0.0002)) {
        num = *(ftbl +(dwnphs * tnp1 & lenmask));
        *ar++ = (num / denom - FL(1.0)) * scal;
      }
      else *ar++ = *ampp;
      phs += inc;
      phs &= PHMASK;
      if (p->ampcod)
        scal = *(++ampp) * over2n;
      if (p->cpscod)
        inc = (long)(*(++cpsp)* sicvt2);
    } while (--nn);
    p->lphs = phs;
    return OK;
}

int gbzset(ENVIRON *csound, GBUZZ *p)
{
    FUNC        *ftp;

    if ((ftp = ftfind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
      if (*p->iphs >= 0) {
        p->lphs = (long)(*p->iphs * FMAXLEN);
        p->prvr = FL(0.0);
      }
      p->ampcod = (XINARG1) ? 1 : 0;
      p->cpscod = (XINARG2) ? 1 : 0;
      p->reported = 0;
      p->last = FL(1.0);
      return OK;
    }
    return NOTOK;
}

MYFLT intpow(MYFLT x, long n)   /* Binary power function */
{
    MYFLT ans = FL(1.0);
    if (n<0) {
      n = -n;
      x = FL(1.0)/x;
    }
    while (n!=0) {
      if (n&1) ans = ans * x;
      n >>= 1;
      x = x*x;
    }
    return ans;
}

int gbuzz(ENVIRON *csound, GBUZZ *p)
{
    FUNC        *ftp;
    MYFLT       *ar, *ampp, *cpsp, *ftbl;
    long        phs, inc, lobits, lenmask, k, km1, kpn, kpnm1, nn;
    long        n;
    MYFLT       r, absr, num, denom, scal, last = p->last;
    long        lphs = p->lphs;

    ftp = p->ftp;
    if (ftp==NULL) {
      return perferror(Str(X_793,"gbuzz: not initialised"));
    }
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    lenmask = ftp->lenmask;
    ampp = p->xamp;
    cpsp = p->xcps;
    k = (long)*p->kk;                   /* fix k and n  */
    if ((n = (long)*p->kn)<0) n = -n;
    if (n == 0) {               /* n must be > 0 */
      n = 1;
    }
    km1 = k - 1;
    kpn = k + n;
    kpnm1 = kpn - 1;
    if ((r = *p->kr) != p->prvr || n != p->prvn) {
      p->twor = r * FL(2.0);
      p->rsqp1 = r * r + FL(1.0);
      p->rtn = intpow(r, n);
      p->rtnp1 = p->rtn * r;
      if ((absr = (MYFLT)fabs(r)) > FL(0.999) && absr < FL(1.001))
        p->rsumr = FL(1.0) / n;
      else p->rsumr = (FL(1.0) - absr) / (FL(1.0) - (MYFLT)fabs(p->rtn));
      p->prvr = r;
      p->prvn = (short)n;
    }
    scal =  *ampp * p->rsumr;
    inc = (long)(*cpsp * sicvt);
    ar = p->ar;
    nn = ksmps;
    do {
      phs = lphs >>lobits;
      denom = p->rsqp1 - p->twor * *(ftbl + phs);
      num = *(ftbl + (phs * k & lenmask))
        - r * *(ftbl + (phs * km1 & lenmask))
        - p->rtn * *(ftbl + (phs * kpn & lenmask))
        + p->rtnp1 * *(ftbl + (phs * kpnm1 & lenmask));
      if (denom > FL(0.0002) || denom < -FL(0.0002)) {
        *ar++ = last = num / denom * scal;
      }
      else if (last<0)
        *ar++ = last = - *ampp;
      else
        *ar++ = last = *ampp;
      if (p->ampcod)
        scal =  p->rsumr * *(++ampp);
      lphs += inc;
      lphs &= PHMASK;
      if (p->cpscod)
        inc = (long)(*(++cpsp) * sicvt);
    } while (--nn);
    p->last = last;
    p->lphs = lphs;
    return OK;
}

#define PLUKMIN 64
static  short   rand15(void);
static  short   rand16(void);

int plukset(ENVIRON *csound, PLUCK *p)
{
    int n;
    long        npts, iphs;
    char        *auxp;
    FUNC        *ftp;
    MYFLT       *ap, *fp;
    MYFLT       phs, phsinc;

    if ((npts = (long)(esr / *p->icps))<PLUKMIN)  /* npts is wavelen in sampls */
      npts = PLUKMIN;                   /*  (but at least min size)  */
    if ((auxp = p->auxch.auxp) == NULL ||
        npts > p->maxpts) {     /* get newspace    */
      auxalloc((npts+1)*sizeof(MYFLT),&p->auxch);
      auxp = p->auxch.auxp;
      p->maxpts = npts;                         /*      if reqd    */
    }
    ap = (MYFLT *)auxp;                         /* as MYFLT array   */
    if (*p->ifn == 0.0)
      for (n=npts; n--; )                       /* f0: fill w. rands */
        *ap++ = (MYFLT)rand16() * DV32768;      /* IV - Jul 11 2002 */
    else if ((ftp = ftfind(csound, p->ifn)) != NULL) {
      fp = ftp->ftable;                         /* else from ftable  */
      phs = FL(0.0);
      phsinc = (MYFLT)(ftp->flen/npts);
      for (n=npts; n--; phs += phsinc) {
        iphs = (long)phs;
        *ap++ = *(fp + iphs);
      }
    }
    *ap = *(MYFLT *)auxp;                               /* last= copy of 1st */
    p->npts = npts;
    p->sicps = (npts * FL(256.0) + FL(128.0)) * onedsr; /* tuned pitch convt */
    p->phs256 = 0;
    p->method = (short)*p->imeth;
    p->param1 = *p->ipar1;
    p->param2 = *p->ipar2;
    switch(p->method) {
    case 1:     /* ignore any given parameters */
      break;
    case 2:     /* stretch factor: param1 >= 1 */
      if (p->param1 < FL(1.0))
        return initerror(Str(X_885,"illegal stretch factor(param1) value"));
      else p->thresh1 =  (short)(FL(32768.0) / p->param1);
      break;
    case 3: /* roughness factor: 0 <= param1 <= 1 */
      if (p->param1 < FL(0.0) || p->param1 > FL(1.0))
        return initerror(Str(X_881,"illegal roughness factor(param1) value"));
      else
        p->thresh1 = (short)(FL(32768.0) * p->param1);
      break;
    case 4: /* rough and stretch factor: 0 <= param1 <= 1, param2 >= 1 */
      if (p->param1 < FL(0.0) || p->param1 > FL(1.0))
        return initerror(Str(X_881,"illegal roughness factor(param1) value"));
      else p->thresh1 = (short)(FL(32768.0) * p->param1);
      if (p->param2 < FL(1.0))
        return initerror(Str(X_886,"illegal stretch factor(param2) value"));
      else p->thresh2 = (short)(FL(32768.0) / p->param2);
      break;
    case 5: /* weighting coeff's: param1 + param2 <= 1 */
      if (p->param1 + p->param2 > 1)
        return initerror(Str(X_664,"coefficients too large(param1 + param2)"));
      break;
    case 6: /* ignore any given parameters */
      break;

    default:
      return initerror(Str(X_1338,"unknown method code"));
    }
    return OK;
}

int pluck(ENVIRON *csound, PLUCK *p)
{
    MYFLT       *ar, *fp;
    long        phs256, phsinc, ltwopi, offset;
    int nsmps;
    MYFLT       frac, diff;

    if (p->auxch.auxp==NULL) { /* RWD FIX */
      return perferror(Str(X_1130,"pluck: not initialised"));
    }
    ar = p->ar;
    phsinc = (long)(*p->kcps * p->sicps);
    phs256 = p->phs256;
    ltwopi = p->npts << 8;
    if (phsinc > ltwopi) {
      return perferror(Str(X_1480,"pluck: kcps more than sample rate"));
    }
    nsmps = ksmps;
    do {
      offset = phs256 >> 8;
      fp = (MYFLT *)p->auxch.auxp + offset;     /* lookup position  */
      diff = *(fp+1) - *fp;
      frac = (MYFLT)(phs256 & 255) / FL(256.0); /*  w. interpolation */
      *ar++ =   (*fp + diff*frac) * *p->kamp;   /*  gives output val */
      if ((phs256 += phsinc) >= ltwopi) {
        int nn;
        MYFLT   newval, preval;
        phs256 -= ltwopi;               /* at phase wrap,    */
        fp=(MYFLT *)p->auxch.auxp;
        preval = *fp;                   /*   copy last pnt   */
        *fp = *(fp + p->npts);          /*     to first,     */
        fp++;                           /*   apply smoothing */
        nn = p->npts;                   /*     up to npts+1  */
        switch(p->method) {
        case 1:
          do {                  /* simple averaging */
            newval = (*fp + preval) / FL(2.0);
            preval = *fp;
            *fp++ = newval;
          } while (--nn);
          break;
        case 2:
          do {                  /* stretched avrging */
            if (rand15() < p->thresh1) {
              newval = (*fp + preval) / FL(2.0);
              preval = *fp;
              *fp++ = newval;
            }
            else preval = *fp++;
          } while (--nn);
          break;
        case 3:
          do {                  /* simple drum */
            if (rand15() < p->thresh1)
              newval = -(*fp + preval) / FL(2.0);
            else newval = (*fp + preval) / FL(2.0);
            preval = *fp;
            *fp++ = newval;
          } while (--nn);
          break;
        case 4:
          do {                  /* stretched drum */
            if (rand15() < p->thresh2) {
              if (rand15() < p->thresh1)
                newval = -(*fp + preval) / FL(2.0);
              else newval = (*fp + preval) / FL(2.0);
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
            preval = (*fp + preval)/FL(2.0);
            *fp++ = preval;
          } while (--nn);
          break;
        }
      }
    } while (--nsmps);
    p->phs256 = phs256;
    return OK;
}

#define RNDMUL  15625L
#define MASK16   0xFFFFL
#define MASK15   0x7FFFL

static short
rand16(void)    /* quick generate a random short between -32768 and 32767 */
{
    static long rand = 1000;
    rand *= RNDMUL;
    rand += 1L;
    rand &= MASK16;
    return (short)rand;
}

static short
rand15(void)    /* quick generate a random short between 0 and 32767 */
{
    static long rand = 1000;
    rand *= RNDMUL;
    rand += 1L;
    rand &= MASK15;
    return (short)rand;
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

#define BIPOLAR 0x4fffffff      /* Constant to make bipolar */
#define RIA 16807               /* multiplier */
#define RIM 0x7FFFFFFFL         /* 2**31 - 1 */

#define dv2_31          (FL(4.656612873077392578125e-10))

long randint31(long seed31)
{
    unsigned long rilo, rihi;

    rilo = RIA * (long)(seed31 & 0xFFFF);
    rihi = RIA * (long)((unsigned long)seed31 >> 16);
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
    return (long)rilo;
}

int rndset(ENVIRON *csound, RAND *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {
      if (*p->iseed > FL(1.0)) {    /* As manual suggest sseed in range [0,1] */
        time_t seed;            /* I reinterpret >1 as a time seed */
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        gettimeofday(&tv, NULL);
        seed = (time_t)(1000000*tv.tv_sec + tv.tv_usec);
#elif defined(WIN32)
        FILETIME    t;
        GetSystemTimeAsFileTime((LPFILETIME) &t);
        seed = 2147483648ul * ((unsigned long) t.dwHighDateTime) +
          ((unsigned long) t.dwLowDateTime >> 1);
#else
        seed = time(NULL);
#endif
        printf(Str(X_458,"Seeding from current time %d\n"), seed);
        if (!p->new) {
          p->rand = 0xffff&(short)seed;
        }
        else {
          p->rand = (long) seed;
        }
      }
      else {
        if (p->new) {
          p->rand = (long) (*p->iseed * FL(2147483648.0));
          p->rand = randint31(p->rand);
          p->rand = randint31(p->rand);
        }
        else
          p->rand = ((short)(*p->iseed * FL(32768.0)))&0xffff;
      }
    }
    p->ampcod = (XINARG1) ? 1 : 0;      /* (not used by krand) */
    return OK;
}

int krand(ENVIRON *csound, RAND *p)
{
    if (p->new) {
      long r = randint31(p->rand);         /* result is a 31-bit value */
      p->rand = r;
      *p->ar = *p->base +
        dv2_31 * (MYFLT)((long)((unsigned)r<<1)-BIPOLAR) * *p->xamp;
    }
    else {
      short rand = (short)p->rand;
      rand *= RNDMUL;
      rand += 1;
      /* IV - Jul 11 2002 */
      *p->ar = *p->base + (MYFLT)rand * *p->xamp * DV32768;
      p->rand = rand;
    }
    return OK;
}

int arand(ENVIRON *csound, RAND *p)
{
    MYFLT       *ar;
    short       rndmul = RNDMUL, n = ksmps;
    MYFLT       ampscl;
    MYFLT       base = *p->base;

    ar = p->ar;
    if (!p->new) {
      short     rand = p->rand;
      if (!(p->ampcod)) {
        ampscl = *p->xamp * DV32768;    /* IV - Jul 11 2002 */
        do {
          rand *= rndmul;
          rand += 1;
          *ar++ = base + (MYFLT)rand * ampscl;          /* IV - Jul 11 2002 */
        } while (--n);
      }
      else {
        MYFLT *xamp = p->xamp;
        do {
          rand *= rndmul;
          rand += 1;
          /* IV - Jul 11 2002 */
          *ar++ = base + (MYFLT)rand * *xamp++ * DV32768;
        } while (--n);
      }
      p->rand = rand;   /* save current rand */
    }
    else {
      int       rand = p->rand;
      if (!(p->ampcod)) {
        ampscl = *p->xamp * dv2_31;
        do {
          rand = randint31(rand);
          *ar++ = base + (MYFLT)((long)((unsigned)rand<<1)-BIPOLAR) * ampscl;
        } while (--n);
      }
      else {
          MYFLT *xamp = p->xamp;
          do {
            rand = randint31(rand);
            *ar++ = base +
              dv2_31 * (MYFLT)((long)((unsigned)rand<<1)-BIPOLAR) * *xamp++;
          } while (--n);
      }
      p->rand = rand;   /* save current rand */
    }

    return OK;
}

int rhset(ENVIRON *csound, RANDH *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {                       /* new seed:            */
      if (*p->iseed > FL(1.0)) {    /* As manual suggest sseed in range [0,1] */
        time_t seed;            /* I reinterpret >1 as a time seed */
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        gettimeofday(&tv, NULL);
        seed = (time_t)(1000000*tv.tv_sec + tv.tv_usec);
#elif defined(WIN32)
        FILETIME    t;
        GetSystemTimeAsFileTime((LPFILETIME) &t);
        seed = 2147483648ul * ((unsigned long) t.dwHighDateTime) +
          ((unsigned long) t.dwLowDateTime >> 1);
#else
        seed = time(NULL);
#endif
        printf(Str(X_458,"Seeding from current time %d\n"), seed);
        if (!p->new) {
          p->rand = 0xffff&(short)seed;
          p->num1 = (MYFLT)seed * dv2_31;
        }
        else {
          p->rand = (long) seed;
          p->num1 = (MYFLT)(seed<<1) * dv2_31;
        }
      }
      else if (!p->new) {
        p->rand = 0xffff&(short)(*p->iseed * 32768L);   /* init rand integ    */
        p->num1 = *p->iseed;                            /*    store fnum      */
      }
      else {
        MYFLT ss = *p->iseed;
        if (ss>FL(1.0)) p->rand = (long) ss;
        else p->rand = (long) (*p->iseed * FL(2147483648.0));
        p->rand = randint31(p->rand);
        p->rand = randint31(p->rand);
        p->num1 = (MYFLT)(p->rand<<1) * dv2_31; /* store fnum      */
      }
      p->phs = 0;                               /*      & phs           */
    }
    p->ampcod = (XINARG1) ? 1 : 0;      /* (not used by krandh) */
    p->cpscod = (XINARG2) ? 1 : 0;
    return OK;
}

int krandh(ENVIRON *csound, RANDH *p)
{
    /* IV - Jul 11 2002 */
    *p->ar = *p->base + p->num1 * *p->xamp;     /* rslt = num * amp     */
    p->phs += (long)(*p->xcps * kicvt);         /* phs += inc           */
    if (p->phs >= MAXLEN) {                     /* when phs overflows,  */
      p->phs &= PHMASK;                         /*      mod the phs     */
      if (!p->new) {
        short rand = (short)p->rand;
        rand *= RNDMUL;                 /*      & recalc number */
        rand += 1;
        p->num1 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        p->rand = rand;
      }
      else {
        long r = randint31(p->rand);            /*      & recalc number */
        p->rand = r;
        p->num1 = (MYFLT)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
      }
    }
    return OK;
}

int randh(ENVIRON *csound, RANDH *p)
{
    long        phs = p->phs, inc;
    int n = ksmps;
    MYFLT       *ar, *ampp, *cpsp;
    MYFLT       base = *p->base;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    inc = (long)(*cpsp++ * sicvt);
    do {
      /* IV - Jul 11 2002 */
      *ar++ = base + p->num1 * *ampp;   /* rslt = num * amp */
      if (p->ampcod)
        ampp++;
      phs += inc;                               /* phs += inc       */
      if (p->cpscod)
        inc = (long)(*cpsp++ * sicvt);
      if (phs >= MAXLEN) {                      /* when phs o'flows, */
        phs &= PHMASK;
        if (!p->new) {
          short rand = p->rand;
          rand *= RNDMUL;               /*   calc new number */
          rand += 1;
          p->num1 = (MYFLT)rand * DV32768;      /* IV - Jul 11 2002 */
          p->rand = rand;
        }
        else {
          long r = randint31(p->rand);          /*   calc new number */
          p->rand = r;
          p->num1 = (MYFLT)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
        }
      }
    } while (--n);
    p->phs = phs;
    return OK;
}

int riset(ENVIRON *csound, RANDI *p)
{
    p->new = (*p->sel!=FL(0.0));
    if (*p->iseed >= FL(0.0)) {                       /* new seed:            */
      if (*p->iseed > FL(1.0)) {    /* As manual suggest sseed in range [0,1] */
        time_t seed;            /* I reinterpret >1 as a time seed */
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        gettimeofday(&tv, NULL);
        seed = (time_t)(1000000*tv.tv_sec + tv.tv_usec);
#elif defined(WIN32)
        FILETIME    t;
        GetSystemTimeAsFileTime((LPFILETIME) &t);
        seed = 2147483648ul * ((unsigned long) t.dwHighDateTime) +
          ((unsigned long) t.dwLowDateTime >> 1);
#else
        seed = time(NULL);
#endif
        printf(Str(X_458,"Seeding from current time %d\n"), seed);
        if (!p->new) {
          short rand = (short)seed;
/*           short ss = rand; */
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
          p->rand = randint31((long)seed);
          p->rand = randint31(p->rand);
          p->num1 = (MYFLT)(p->rand<<1) * dv2_31; /* store num1,2 */
          p->rand = randint31(p->rand);
          p->num2 = (MYFLT)(p->rand<<1) * dv2_31;
        }
      }
      else if (!p->new) {
        short rand = (short)(*p->iseed * FL(32768.0)); /* init rand integ */
        rand *= RNDMUL;                 /*      to 2nd value    */
        rand += 1;
        p->num1 = *p->iseed;                    /*      store num1,2    */
        p->num2 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        p->rand = rand;
      }
      else {
        MYFLT ss = *p->iseed;
        if (ss>FL(1.0)) p->rand = (long) ss;
        else p->rand = (long) (*p->iseed * FL(2147483648.0));
        p->rand = randint31(p->rand);
        p->rand = randint31(p->rand);
        p->num1 = (MYFLT)(p->rand<1) * dv2_31; /* store num1,2 */
        p->rand = randint31(p->rand);
        p->num2 = (MYFLT)(p->rand<<1) * dv2_31;
      }
      p->phs = 0;                               /*      & clear phs     */
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;  /* & diff     */
    }
    p->ampcod = (XINARG1) ? 1 : 0;      /* (not used by krandi) */
    p->cpscod = (XINARG2) ? 1 : 0;
    return OK;
}

int krandi(ENVIRON *csound, RANDI *p)
{                                       /* rslt = (num1 + diff*phs) * amp */
    /* IV - Jul 11 2002 */
    *p->ar = *p->base + (p->num1 + (MYFLT)p->phs * p->dfdmax) * *p->xamp;
    p->phs += (long)(*p->xcps * kicvt); /* phs += inc           */
    if (p->phs >= MAXLEN) {             /* when phs overflows,  */
      p->phs &= PHMASK;                 /*      mod the phs     */
      if (!p->new) {
        short rand = p->rand;
        rand *= RNDMUL;         /*      recalc random   */
        rand += 1;
        p->num1 = p->num2;              /*      & new num vals  */
        p->num2 = (MYFLT)rand * DV32768;        /* IV - Jul 11 2002 */
        p->rand = rand;
      }
      else {
        long r = randint31(p->rand);    /*      recalc random   */
        p->rand = r;
        p->num1 = p->num2;              /*      & new num vals  */
        p->num2 = (MYFLT)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
      }
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

int randi(ENVIRON *csound, RANDI *p)
{
    long        phs = p->phs, inc;
    int         n = ksmps;
    MYFLT       *ar, *ampp, *cpsp;
    MYFLT       base = *p->base;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    inc = (long)(*cpsp++ * sicvt);
    do {
      /* IV - Jul 11 2002 */
      *ar++ = base + (p->num1 + (MYFLT)phs * p->dfdmax) * *ampp;
      if (p->ampcod)
        ampp++;
      phs += inc;                               /* phs += inc       */
      if (p->cpscod)
        inc = (long)(*cpsp++ * sicvt);          /*   (nxt inc)      */
      if (phs >= MAXLEN) {                      /* when phs o'flows, */
        phs &= PHMASK;
        if (!p->new) {
          short rand = p->rand;
          rand *= RNDMUL;                       /*   calc new numbers*/
          rand += 1;
          p->num1 = p->num2;
          p->num2 = (MYFLT)rand * DV32768;      /* IV - Jul 11 2002 */
          p->rand = rand;
        }
        else {
          long r = randint31(p->rand);          /*   calc new numbers*/
          p->rand = r;
          p->num1 = p->num2;
          p->num2 = (MYFLT)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
        }
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    } while (--n);
    p->phs = phs;
    return OK;
}

