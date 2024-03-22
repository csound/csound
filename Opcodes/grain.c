/*
    grain.c:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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

/*      Granular synthesizer designed and coded by Paris Smaragdis      */
/*      Berklee College of Music Csound development team                */
/*      Copyright (c) May 1994.  All rights reserved                    */

/* Some speed hacks added by John Fitch */

#include "stdopcod.h"
#include "grain.h"

static inline MYFLT Unirand(CSOUND *csound, MYFLT a)
{
    MYFLT x;
    x = (MYFLT) (csound->Rand31(&(csound->randSeed1)) - 1) / FL(2147483645.0);
    return (x * a);
}

static int32_t agsset(CSOUND *csound, PGRA *p)  /*      Granular U.G. set-up    */
{
    FUNC        *gftp, *eftp;
    int32        bufsize;
    MYFLT       *d;

    if (LIKELY((gftp = csound->FTnp2Finde(csound, p->igfn)) != NULL))
      p->gftp = gftp;
    else return NOTOK;

    if (LIKELY((eftp = csound->FTnp2Finde(csound, p->iefn)) != NULL))
      p->eftp = eftp;
    else return NOTOK;

    p->gcount = FL(1.0);

    if (*p->opt == 0)
      p->pr = (MYFLT)(gftp->flen << gftp->lobits);
    else
      p->pr = FL(0.0);

    bufsize = sizeof(MYFLT) * (2L * (size_t) (CS_ESR * *p->imkglen)
                               + (3L * CS_KSMPS));

    if (p->aux.auxp == NULL || (uint32_t)bufsize > p->aux.size)
      csound->AuxAlloc(csound, bufsize, &p->aux);
    else memset(p->aux.auxp, '\0', bufsize); /* Clear any old data */
    d  = p->x = (MYFLT *)p->aux.auxp;
    d +=  (int32_t)(CS_ESR * *p->imkglen) + CS_KSMPS;
    p->y = d;

    p->ampadv = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    p->lfradv = IS_ASIG_ARG(p->xlfr) ? 1 : 0;
    p->dnsadv = IS_ASIG_ARG(p->xdns) ? 1 : 0;
    return OK;
}

static inline unsigned int ISPOW2(unsigned int x) {
  return (x > 0) && !(x & (x - 1)) ? 1 : 0;
}

static int32_t ags(CSOUND *csound, PGRA *p) /*  Granular U.G. a-rate main routine */
{
    FUNC        *gtp, *etp;
    MYFLT       *buf, *out, *rem, *gtbl, *etbl;
    MYFLT       *xdns, *xamp, *xlfr, *temp, amp;
    int32       isc, isc2, inc, inc2, lb, lb2;
    int32       n, bufsize;
    int32       ekglen;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    i, nsmps = CS_KSMPS;
    MYFLT       kglen = *p->kglen;
    MYFLT       gcount = p->gcount;
    uint32_t elen, glen;
    MYFLT gcvt, ecvt, einc;
    int pow2tab;
                                /* Pick up common values to locals for speed */
    if (UNLIKELY(p->aux.auxp==NULL)) goto err1;
    if (UNLIKELY(kglen<=FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               Str("grain: grain length zero"));
    gtp  = p->gftp;
    gtbl = gtp->ftable;
    glen = gtp->flen;
    gcvt = glen/csound->GetSr(csound);

    pow2tab = ISPOW2(glen);

    etp  = p->eftp;
    etbl = etp->ftable;
    elen = etp->flen;
    ecvt = elen/csound->GetSr(csound);

    lb   = gtp->lobits;
    lb2  = etp->lobits;

    buf  = p->x;
    rem  = p->y;

    out  = p->sr;

    if (kglen > *p->imkglen) kglen = *p->imkglen;

    ekglen  = (int32)(CS_ESR * kglen);   /* Useful constant */
    inc2    = (int32)(csound->sicvt / kglen); /* Constant for each cycle */
    einc =  (1./kglen) * ecvt;
    bufsize = CS_KSMPS + ekglen;
    xdns    = p->xdns;
    xamp    = p->xamp;
    xlfr    = p->xlfr;

    memset(buf, '\0', bufsize*sizeof(MYFLT));
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset ; i < nsmps ; i++) {
      if (gcount >= FL(1.0)) { /* I wonder..... */
        gcount = FL(0.0);
        amp = *xamp + Unirand(csound, *p->kabnd);

        temp = buf + i;
        n = ekglen;
        isc = (int32) Unirand(csound, p->pr);
        isc2 = 0;
        if(pow2tab) {
          /* VL 21/11/18 original code, fixed-point indexing */
        inc = (int32) ((*xlfr + Unirand(csound, *p->kbnd)) * csound->sicvt);
        do {
          *temp++ += amp  * *(gtbl + (isc >> lb)) *
                     *(etbl + (isc2 >> lb2));
          isc  = (isc +inc )&PHMASK;
          isc2 = (isc2+inc2)&PHMASK;
        } while (--n);
        }
        else {
          /* VL 21/11/18 new code, floating-point indexing */
          MYFLT gph = (MYFLT) isc;
          MYFLT eph = FL(0.0);
          MYFLT ginc = (*xlfr + Unirand(csound, *p->kbnd)) * gcvt;
        do {
          *temp++ += amp * gtbl[(int)gph] * etbl[(int)eph];
          gph += ginc;
          eph += einc;
          while(gph < 0) gph += glen;
          while(gph >= glen) gph -= glen;
          /* *** Can eph ever be negative?  only if einc negative *** * */
          while(eph < 0) eph += elen;
          while(eph >= elen) eph -= elen;
        } while (--n);
        }
      }
      xdns += p->dnsadv;
      gcount += *xdns * csound->onedsr;
      xamp += p->ampadv;
      xlfr += p->lfradv;
    }

    n = bufsize;
    temp = rem;
    do {
      *temp = *buf++ + *(temp + CS_KSMPS);
      temp++;
    } while (--n);

    memcpy(&out[offset], rem, (nsmps-offset)*sizeof(MYFLT));
    p->gcount = gcount;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("grain: not initialised"));
}

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
   { "grain", S(PGRA),  TR, 3,   "a",    "xxxkkkiiio", (SUBR)agsset, (SUBR)ags, NULL, NULL}
  };

int32_t grain_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
