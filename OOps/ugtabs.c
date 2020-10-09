/*
    ugtabs.c:  new implementation of table readers and writers

    Copyright (C) 2013 V Lazzarini

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

#include "csoundCore.h"
#include "ugtabs.h"
#include "ugens2.h"
#include <math.h>

//(x >= FL(0.0) ? (int32_t)x : (int32_t)((double)x - 0.99999999))
#define MYFLOOR(x) FLOOR(x)

static inline unsigned int isPowerOfTwo (unsigned int x) {
  return (x > 0) && !(x & (x - 1)) ? 1 : 0;
}

int32_t tabler_init(CSOUND *csound, TABL *p) {

    int32_t ndx, len;
    int32_t mask;

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound,
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    len = p->ftp->flen;
    mask = p->ftp->lenmask;
    p->np2 = isPowerOfTwo(len) ? 0 : 1;

    if (*p->mode)
      p->mul = len;
    else
      p->mul = 1;

    ndx = FLOOR((*p->ndx + *p->offset)*p->mul);
    if (*p->wrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    *p->sig = p->ftp->ftable[ndx];
    return OK;
}


int32_t tabl_setup(CSOUND *csound, TABL *p) {
    if(p->ftp == NULL) {
      /* check for this only on first allocation */
      if (UNLIKELY(IS_ASIG_ARG(p->ndx) != IS_ASIG_ARG(p->sig))) {
        if (CS_KSMPS != 1)
          return
            csound->InitError(csound,
                              Str("table: index type inconsistent with output"));
      }
    }
    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound,
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);

    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;

    p->iwrap = (int32_t) *p->wrap;
    return OK;
}

int32_t tabler_kontrol(CSOUND *csound, TABL *p) {
    int32_t ndx, len = p->len;
    int32_t mask = p->ftp->lenmask;
    IGN(csound);

    ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul);
    if (p->iwrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    *p->sig = p->ftp->ftable[ndx];
    return OK;
}



int32_t tabler_audio(CSOUND *csound, TABL *p)
{
    IGN(csound);
    int32_t ndx, len = p->len, n, nsmps = CS_KSMPS;
    int32_t mask = p->ftp->lenmask;
    MYFLT *sig = p->sig;
    MYFLT *ndx_f = p->ndx;
    MYFLT *func = p->ftp->ftable;
    MYFLT offset = *p->offset;
    MYFLT mul = p->mul;
    int32_t iwrap = p->iwrap;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;

    if (UNLIKELY(koffset)) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=koffset; n < nsmps; n++) {
      ndx = MYFLOOR((ndx_f[n] + offset)*mul);
      if (iwrap) {
        if (p->np2) {
          while(ndx >= len) ndx -= len;
          while(ndx < 0)  ndx += len;
        }
        else ndx &= mask;
      } else {
        if (UNLIKELY(ndx >= len)) ndx = len - 1;
        else if (UNLIKELY(ndx < 0)) ndx = 0;
      }
      p->sig[n] = func[ndx];
    }

    return OK;
}

int32_t tableir_init(CSOUND *csound, TABL *p) {

    int32_t ndx, len;
    int32_t mask;
    MYFLT tmp, frac;
    MYFLT x1, x2;

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound,
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    len = p->ftp->flen;
    p->np2 = isPowerOfTwo(len) ? 0 : 1;
    mask = p->ftp->lenmask;

    if (*p->mode)
      p->mul = len;
    else
      p->mul = 1;

    tmp = (*p->ndx + *p->offset)*p->mul;
    ndx = MYFLOOR(tmp);
    frac = tmp - ndx;

    if (*p->wrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    x1 = p->ftp->ftable[ndx];
    x2 = p->ftp->ftable[ndx+1];
    *p->sig = x1 + (x2 - x1)*frac;
    return OK;
}



int32_t tableir_kontrol(CSOUND *csound, TABL *p) {
    int32_t ndx, len = p->len;
    int32_t mask = p->ftp->lenmask;
    MYFLT tmp, frac;
    MYFLT x1, x2;
    IGN(csound);

    tmp = (*p->ndx + *p->offset)*p->mul;
    ndx = MYFLOOR(tmp);
    frac = tmp - ndx;

    if (p->iwrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    x1 = p->ftp->ftable[ndx];
    x2 = p->ftp->ftable[ndx+1];
    *p->sig = x1 + (x2 - x1)*frac;
    return OK;
}

int32_t tableir_audio(CSOUND *csound, TABL *p)
{
    IGN(csound);
    int32_t ndx, len    = p->len, n, nsmps = CS_KSMPS;
    int32_t mask        = p->ftp->lenmask;
    MYFLT *sig          = p->sig;
    MYFLT *ndx_f        = p->ndx;
    MYFLT *func         = p->ftp->ftable;
    MYFLT offset        = *p->offset;
    MYFLT mul           = p->mul, tmp, frac;
    int32_t iwrap       = p->iwrap;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early   = p->h.insdshead->ksmps_no_end;

    if (UNLIKELY(koffset)) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=koffset; n < nsmps; n++) {
      MYFLT x1, x2;
      tmp = (ndx_f[n] + offset)*mul;
      ndx = MYFLOOR(tmp);
      frac = tmp - ndx;
      if (iwrap) {
        if (p->np2) {
          while(ndx >= len) ndx -= len;
          while(ndx < 0)  ndx += len;
        }
        else ndx &= mask;
      } else {
        if (UNLIKELY(ndx >= len)) ndx = len - 1;
        else if (UNLIKELY(ndx < 0)) ndx = 0;
      }
      x1 = func[ndx];
      x2 = func[ndx+1];
      p->sig[n] = x1 + (x2 - x1)*frac;
    }

    return OK;
}

int32_t table3r_init(CSOUND *csound, TABL *p) {

    int32_t ndx, len;
    int32_t mask;
    MYFLT   tmp, frac;
    MYFLT   x0, x1, x2, x3;
    MYFLT   fracub, fracsq, temp1;
    MYFLT   *func;

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound,
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    len = p->ftp->flen;
    mask = p->ftp->lenmask;
    p->np2 = isPowerOfTwo(len) ? 0 : 1;
    func  =p->ftp->ftable;

    if (*p->mode)
      p->mul = len;
    else
      p->mul = 1;

    tmp = (*p->ndx + *p->offset)*p->mul;
    ndx = MYFLOOR(tmp);
    frac = tmp - ndx;

    if (*p->wrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    if (UNLIKELY(ndx<1 || ndx==len-1 || len <4)) {
      x1 = func[ndx];
      x2 = func[ndx+1];
      *p->sig = x1 + (x2 - x1)*frac;
    } else {
      x0 = func[ndx-1];
      x1 = func[ndx];
      x2 = func[ndx+1];
      x3 = func[ndx+2];
      fracsq = frac*frac;
      fracub = fracsq*x0;
      temp1 = x3+FL(3.0)*x1;
      *p->sig =  x1 + FL(0.5)*fracub +
        frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +
        frac*fracsq*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
    }
    return OK;
}



int32_t table3r_kontrol(CSOUND *csound, TABL *p) {
    int32_t ndx, len = p->len;
    int32_t mask = p->ftp->lenmask;
    MYFLT tmp, frac;
    MYFLT x0, x1, x2, x3;
    MYFLT *func  =p->ftp->ftable;
    MYFLT fracub, fracsq, temp1;

    IGN(csound);

    tmp = (*p->ndx + *p->offset)*p->mul;
    ndx = MYFLOOR(tmp);
    frac = tmp - ndx;

    if (p->iwrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    if (UNLIKELY(ndx<1 || ndx==len-1 || len <4)) {
      x1 = func[ndx];
      x2 = func[ndx+1];
      *p->sig = x1 + (x2 - x1)*frac;
    } else {
      x0 = func[ndx-1];
      x1 = func[ndx];
      x2 = func[ndx+1];
      x3 = func[ndx+2];
      fracsq = frac*frac;
      fracub = fracsq*x0;
      temp1 = x3+FL(3.0)*x1;
      *p->sig =  x1 + FL(0.5)*fracub +
        frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +
        frac*fracsq*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
    }
    return OK;
}

int32_t table3r_audio(CSOUND *csound, TABL *p)
{
    IGN(csound);
    int32_t ndx, len = p->len, n, nsmps = CS_KSMPS;
    int32_t mask = p->ftp->lenmask;
    MYFLT *sig = p->sig;
    MYFLT *ndx_f = p->ndx;
    MYFLT *func = p->ftp->ftable;
    MYFLT offset = *p->offset;
    MYFLT mul = p->mul, tmp, frac;
    int32_t iwrap = p->iwrap;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;

    if (UNLIKELY(koffset)) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=koffset; n < nsmps; n++) {
      MYFLT x0,x1,x2,x3,temp1,fracub,fracsq;
      tmp = (ndx_f[n] + offset)*mul;
      ndx = MYFLOOR(tmp);
      frac = tmp - ndx;
      if (iwrap) {
        if (p->np2) {
          while(ndx >= len) ndx -= len;
          while(ndx < 0)  ndx += len;
        }
        else ndx &= mask;
      } else {
        if (UNLIKELY(ndx >= len)) ndx = len - 1;
        else if (UNLIKELY(ndx < 0)) ndx = 0;
      }

      if (UNLIKELY(ndx<1 || ndx==len-1 || len <4)) {
        x1 = func[ndx];
        x2 = func[ndx+1];
        p->sig[n] = x1 + (x2 - x1)*frac;
      } else {
        x0 = func[ndx-1];
        x1 = func[ndx];
        x2 = func[ndx+1];
        x3 = func[ndx+2];
        fracsq = frac*frac;
        fracub = fracsq*x0;
        temp1 = x3+x1+x1+x1;
        p->sig[n] =  x1 + FL(0.5)*fracub +
          frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +
          fracsq*frac*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
      }
    }
    return OK;
}

int32_t tablkt_setup(CSOUND *csound, TABL *p) {

    if (UNLIKELY(IS_ASIG_ARG(p->ndx) != IS_ASIG_ARG(p->sig))) {
      if (CS_KSMPS != 1)
        return
          csound->InitError(csound,
                            Str("tablekt: index type inconsistent with output"));
    }

    p->iwrap = (int32_t) *p->wrap;
    return OK;
}

int32_t tablerkt_kontrol(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->len = p->ftp->flen;
    p->np2 = isPowerOfTwo(p->len) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;

    return tabler_kontrol(csound,p);
}


int32_t tablerkt_audio(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->lenmask) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;

    return tabler_audio(csound,p);
}

int32_t tableirkt_kontrol(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;

    return tableir_kontrol(csound,p);
}

int32_t tableirkt_audio(CSOUND *csound, TABL *p)
{

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;
    return tableir_audio(csound,p);
}

int32_t table3rkt_kontrol(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;
    return table3r_kontrol(csound,p);;
}

int32_t table3rkt_audio(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;
    return table3r_audio(csound,p);
}

int32_t tablew_init(CSOUND *csound, TABL *p) {

    int32_t ndx, len;
    int32_t mask;
    MYFLT *func;
    int32 iwrap = *p->wrap;

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound,
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    func = p->ftp->ftable;
    mask = p->ftp->lenmask;
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    len = p->ftp->flen;

    if (*p->mode)
      p->mul = len;
    else
      p->mul = 1;

    ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul  + (iwrap==2 ? 0.5:0));
    if (iwrap) {
      ndx = iwrap == 2 ? MYFLOOR(ndx+0.5) : ndx;
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    p->ftp->ftable[ndx] = *p->sig;
    if (ndx == 0 && iwrap==2) func[len] = func[ndx];
    return OK;
}

int32_t tablew_kontrol(CSOUND *csound, TABL *p) {
    int32_t ndx, len = p->len;
    int32_t mask = p->ftp->lenmask;
    MYFLT *func = p->ftp->ftable;
    int32 iwrap = p->iwrap;
    IGN(csound);

    ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul + (iwrap==2 ? 0.5:0));
    if (iwrap) {
      if (p->np2) {
        while(ndx >= len) ndx -= len;
        while(ndx < 0)  ndx += len;
      }
      else ndx &= mask;
    } else {
      if (UNLIKELY(ndx >= len)) ndx = len - 1;
      else if (UNLIKELY(ndx < 0)) ndx = 0;
    }
    func[ndx] = *p->sig;
    if (ndx == 0 && iwrap==2) func[len] = func[ndx];
    return OK;
}

int32_t tablew_audio(CSOUND *csound, TABL *p) {
    IGN(csound);
    int32_t ndx, len = p->len, n, nsmps = CS_KSMPS;
    int32_t mask = p->ftp->lenmask;
    MYFLT *sig = p->sig;
    MYFLT *ndx_f = p->ndx;
    MYFLT *func = p->ftp->ftable;
    MYFLT offset = *p->offset;
    MYFLT mul = p->mul;
    int32 iwrap = p->iwrap;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;

    if (UNLIKELY(early)) nsmps -= early;

    for (n=koffset; n < nsmps; n++) {
      ndx = MYFLOOR((ndx_f[n] + offset)*mul + (iwrap==2 ? 0.5:0));
      if (iwrap) {
        if (p->np2) {
          while(ndx >= len) ndx -= len;
          while(ndx < 0)  ndx += len;
        }
        else ndx &= mask;
      } else {
        if (UNLIKELY(ndx >= len)) ndx = len - 1;
        else if (UNLIKELY(ndx < 0)) ndx = 0;
      }
      func[ndx] = sig[n];
      if (iwrap==2 && ndx == 0) func[len] = func[ndx];
    }
    return OK;
}

int32_t tablewkt_kontrol(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;

    return tablew_kontrol(csound,p);
}


int32_t tablewkt_audio(CSOUND *csound, TABL *p) {

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    p->np2 = isPowerOfTwo(p->ftp->flen) ? 0 : 1;
    if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
    p->len = p->ftp->flen;
    return tablew_audio(csound,p);;
}

int32_t table_length(CSOUND *csound, TLEN *p) {
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ftable)) == NULL)) {
      csound->Warning(csound, Str("table: could not find ftable %d"),
                      (int32_t) *p->ftable);
      *p->ans = FL(-1.0);
      return NOTOK;
    }
    else *p->ans = (MYFLT) ftp->flen;
    return OK;
}

int32_t table_gpw(CSOUND *csound, TGP *p) {
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ftable)) == NULL)) {
      csound->Warning(csound,
                      Str("table: could not find ftable %d"),
                      (int32_t) *p->ftable);
      return NOTOK;
    }
    ftp->ftable[ftp->flen] = ftp->ftable[0];
    return OK;
}

int32_t table_copy(CSOUND *csound, TGP *p) {
    FUNC *dest, *src;
    int32 len1, len2, i, rp;
    if (UNLIKELY((dest = csound->FTnp2Find(csound, p->ftable)) == NULL ||
                 (src = csound->FTnp2Find(csound, p->ftsrc)) == NULL)) {
      csound->Warning(csound,
                      Str("table: could not find ftables %d and/or %d"),
                      (int32_t) *p->ftable, (int32_t) *p->ftsrc);
      return NOTOK;
    }
    len1 = dest->flen;
    len2 = src->flen;
    for (i=rp=0; i<len1;i++) {
      dest->ftable[i] = src->ftable[rp];
      rp = rp == len2 ? 0 : rp+1;
    }
    return OK;
}

int32_t table_mix(CSOUND *csound, TABLMIX *p) {
    int32 np2, np21, np22;
    FUNC *ftp, *ftp1, *ftp2;
    int32 len, len1, len2, flen;
    MYFLT g1, g2, *func, *func1, *func2;
    int32 off, off1, off2;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->tab)) == NULL)) {
      csound->Warning(csound,
                      Str("table: could not find ftable %d"), (int32_t) *p->tab);
      return NOTOK;
    }
    np2 = isPowerOfTwo(ftp->flen) ? 0 : 1;

    if (UNLIKELY((ftp1 = csound->FTnp2Find(csound, p->tab1)) == NULL)) {
      csound->Warning(csound,
                      Str("table: could not find ftable %d"), (int32_t) *p->tab1);
      return NOTOK;
    }
    np21 = isPowerOfTwo(ftp->flen) ? 0 : 1;

    if (UNLIKELY((ftp2 = csound->FTnp2Find(csound, p->tab2)) == NULL)) {
      csound->Warning(csound,
                      Str("table: could not find ftable %d"), (int32_t) *p->tab2);
      return NOTOK;
    }
    np22 = isPowerOfTwo(ftp2->flen) ? 0 : 1;

    len = MYFLOOR(*p->len);
    flen = ftp->flen;
    len1 = ftp1->flen;
    len2 = ftp2->flen;
    func = ftp->ftable;
    func1 = ftp1->ftable;
    func2 = ftp2->ftable;
    off = *p->off;
    off1 = *p->off1;
    off2 = *p->off2;
    g1 = *p->g1;
    g2 = *p->g2;

    if (len>0) {
      int32_t i, p0, p1, p2;
      for (i=0; i < len; i++) {
        p0 = i+off;
        p1 = i+off1;
        p2 = i+off2;
        if (np2) {
          while(p0 < 0) p0 += flen;
          while(p0 >= len1) p0 -= flen;
        }
        else p0 &= ftp->lenmask;
        if (np21) {
          while(p1 < 0) p1 += len1;
          while(p1 >= len1) p1 -= len1;
        }
        else p1 &= ftp1->lenmask;
        if (np22) {
          while (p2 < 0) p2 += len2;
          while (p2 >= len2) p2 -= len2;
        }
        else p2 &= ftp2->lenmask;
        func[p0] = func1[p1]*g1 + func2[p2]*g2;
      }
    } else {
      int32_t i, p0, p1, p2;
      for (i=0; i > len; i--) {
        p0 = i+off;
        p1 = i+off1;
        p2 = i+off2;
        if (np2) {
          while(p0 < 0) p0 += flen;
          while(p0 >= len1) p0 -= flen;
        }
        else p0 &= ftp->lenmask;
        if (np21) {
          while(p1 < 0) p1 += len1;
          while(p1 >= len1) p1 -= len1;
        }
        else p1 &= ftp1->lenmask;
        if (np22) {
          while(p2 < 0) p2 += len2;
          while(p2 >= len2) p2 -= len2;
        }
        else p2 &= ftp2->lenmask;
        func[p0] = func1[p1]*g1 + func2[p2]*g2;
      }
    }
    return OK;
}

int32_t table_ra_set(CSOUND *csound, TABLRA *p) {
    IGN(csound);
    IGN(p);
    return OK;
}

int32_t table_ra(CSOUND *csound, TABLRA *p) {
    int32 pos, np2, nsmps, len, i;
    MYFLT *sig= p->sig, *func;
    int32_t mask;
    FUNC *ftp;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    nsmps = CS_KSMPS;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    np2 = isPowerOfTwo(ftp->flen) ? 0 : 1;

    mask = ftp->lenmask;
    pos = *p->strt + *p->off;

    if (pos < 0)
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not read negative pos %d"), pos);

    if (UNLIKELY(koffset)) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }

    func = ftp->ftable;
    len = ftp->flen;
    for (i=koffset; i < nsmps; i++) {
      if (np2) pos %= len;
      else pos &= mask;
      sig[i] = func[pos];
      pos++;
    }

    return OK;
}

int32_t table_wa_set(CSOUND *csound, TABLWA *p) {
    IGN(csound);
    if(!*p->skipinit) p->pos = 0;
    p->pos += *p->off;
    return OK;
}

int32_t table_wa(CSOUND *csound, TABLWA *p) {
    int32 pos, np2, nsmps, len, i;
    MYFLT *sig= p->sig, *func;
    int32_t mask;
    FUNC *ftp;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    nsmps = CS_KSMPS;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    np2 = isPowerOfTwo(ftp->flen) ? 0 : 1;

    mask = ftp->lenmask;
    pos = p->pos; /*+ *p->off;*/

    if (pos < 0)
      return csound->PerfError(csound, &(p->h),
                               Str("table: could not read negative pos %d"), pos);

    if (UNLIKELY(early)) nsmps -= early;

    func = ftp->ftable;
    len = ftp->flen;
    for (i=koffset; i < nsmps; i++) {
      if (np2) pos %= len;
      else pos &= mask;
      func[pos] = sig[i];
      pos++;
    }
    p->pos = pos;
    *p->strt = pos;
    return OK;
}
