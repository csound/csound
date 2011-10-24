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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*      Granular synthesizer designed and coded by Paris Smaragdis      */
/*      Berklee College of Music Csound development team                */
/*      Copyright (c) May 1994.  All rights reserved                    */

/* Some speed hacks added by John Fitch */

#include "csdl.h"
#include "grain.h"

static inline MYFLT Unirand(CSOUND *csound, MYFLT a)
{
    MYFLT x;
    x = (MYFLT) (csound->Rand31(&(csound->randSeed1)) - 1) / FL(2147483645.0);
    return (x * a);
}

static int agsset(CSOUND *csound, PGRA *p)  /*      Granular U.G. set-up    */
{
    FUNC        *gftp, *eftp;
    int32        bufsize;
    MYFLT       *d;

    if (LIKELY((gftp = csound->FTFind(csound, p->igfn)) != NULL))
      p->gftp = gftp;
    else return NOTOK;

    if (LIKELY((eftp = csound->FTFind(csound, p->iefn)) != NULL))
      p->eftp = eftp;
    else return NOTOK;

    p->gcount = FL(1.0);

    if (*p->opt == 0)
      p->pr = (MYFLT)(gftp->flen << gftp->lobits);
    else
      p->pr = FL(0.0);

    bufsize = sizeof(MYFLT) * (2L * (size_t) (csound->esr * *p->imkglen)
                               + (3L * csound->ksmps));

    if (p->aux.auxp == NULL || bufsize > p->aux.size)
      csound->AuxAlloc(csound, bufsize, &p->aux);
    else memset(p->aux.auxp, '\0', bufsize); /* Clear any old data */
    d  = p->x = (MYFLT *)p->aux.auxp;
    d +=  (int)(csound->esr * *p->imkglen) + csound->ksmps;
    p->y = d;

    p->ampadv = (XINARG1) ? 1 : 0;
    p->lfradv = (XINARG2) ? 1 : 0;
    p->dnsadv = (XINARG3) ? 1 : 0;
    return OK;
}

static int ags(CSOUND *csound, PGRA *p) /*  Granular U.G. a-rate main routine */
{
    FUNC        *gtp, *etp;
    MYFLT       *buf, *out, *rem, *gtbl, *etbl;
    MYFLT       *xdns, *xamp, *xlfr, *temp, amp;
    int32       isc, isc2, inc, inc2, lb, lb2;
    int32       n, i, bufsize;
    int32       ekglen;
    int         nsmps = csound->ksmps;
    MYFLT       kglen = *p->kglen;
    MYFLT       gcount = p->gcount;

                                /* Pick up common values to locals for speed */
    if (UNLIKELY(p->aux.auxp==NULL)) goto err1;
    if (UNLIKELY(kglen<=FL(0.0)))
      return csound->PerfError(csound, Str("grain: grain length zero"));
    gtp  = p->gftp;
    gtbl = gtp->ftable;

    etp  = p->eftp;
    etbl = etp->ftable;
    lb   = gtp->lobits;
    lb2  = etp->lobits;

    buf  = p->x;
    rem  = p->y;

    out  = p->sr;

    if (kglen > *p->imkglen) kglen = *p->imkglen;

    ekglen  = (int32)(csound->esr * kglen);   /* Useful constant */
    inc2    = (int32)(csound->sicvt / kglen); /* Constant for each cycle */
    bufsize = csound->ksmps + ekglen;
    xdns    = p->xdns;
    xamp    = p->xamp;
    xlfr    = p->xlfr;

    memset(buf, '\0', bufsize*sizeof(MYFLT));

    for (i = 0 ; i < nsmps ; i++) {
      if (gcount >= FL(1.0)) { /* I wonder..... */
        gcount = FL(0.0);
        amp = *xamp + Unirand(csound, *p->kabnd);
        isc = (int32) Unirand(csound, p->pr);
        isc2 = 0;
        inc = (int32) ((*xlfr + Unirand(csound, *p->kbnd)) * csound->sicvt);

        temp = buf + i;
        n = ekglen;
        do {
          *temp++ += amp  * *(gtbl + (isc >> lb)) *
                     *(etbl + (isc2 >> lb2));
          isc  = (isc +inc )&PHMASK;
          isc2 = (isc2+inc2)&PHMASK;
        } while (--n);
      }

      xdns += p->dnsadv;
      gcount += *xdns * csound->onedsr;
      xamp += p->ampadv;
      xlfr += p->lfradv;
    }

    n = bufsize;
    temp = rem;
    do {
      *temp = *buf++ + *(temp + csound->ksmps);
      temp++;
    } while (--n);

    memcpy(out, rem, csound->ksmps*sizeof(MYFLT));
    p->gcount = gcount;
    return OK;
 err1:
    return csound->PerfError(csound, Str("grain: not initialised"));
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "grain", S(PGRA),  TR|5,   "a",    "xxxkkkiiio", (SUBR)agsset, NULL, (SUBR)ags }
};

int grain_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

