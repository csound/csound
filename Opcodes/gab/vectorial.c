/*   Copyright (C) 2002-2004 Gabriel Maldonado

     The gab library is free software; you can redistribute it
     and/or modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     The gab library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with the gab library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
     02111-1307 USA

    Ported to csound5 by:Andres Cabrera andres@geminiflux.com
    This file includes the vectorial table opcodes from newopcodes.c
    Optional arguments to some opcodes and other fixes by Andres Cabrera
    and Istvan Varga.
*/
//#include "stdopcod.h"
#include "csoundCore.h"
#include "interlocks.h"
#include "vectorial.h"
#include <math.h>

static int mtable_i(CSOUND *csound,MTABLEI *p)
{
    FUNC *ftp;
    int j, nargs;
    MYFLT *table, xbmul = FL(0.0), **out = p->outargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtablei: incorrect table number"));
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-4;
    if (*p->ixmode)
      xbmul = (MYFLT) (ftp->flen / nargs);

    if (*p->kinterp) {
      MYFLT     v1, v2 ;
      MYFLT fndx = (*p->ixmode) ? *p->xndx * xbmul : *p->xndx;
      long indx = (long) fndx;
      MYFLT fract = fndx - indx;
      for (j=0; j < nargs; j++) {
        v1 = table[indx * nargs + j];
        v2 = table[(indx + 1) * nargs + j];
        **out++ = v1 + (v2 - v1) * fract;
      }
    }
    else {
      long indx = (*p->ixmode) ? (long)(*p->xndx * xbmul) : (long) *p->xndx;
      for (j=0; j < nargs; j++)
        **out++ =  table[indx * nargs + j];
    }
    return OK;
}

static int mtable_set(CSOUND *csound,MTABLE *p) /*  mtab by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtable: incorrect table number"));
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-4;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    if (*p->ixmode)
      p->xbmul = (MYFLT) ftp->flen / p->nargs;
    return OK;
}

static int mtable_k(CSOUND *csound,MTABLE *p)
{
    int j, nargs = p->nargs;
    MYFLT **out = p->outargs;
    MYFLT *table;
    long len;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtablek: incorrect table number"));
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (*p->ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table= p->ftable;
    len = p->len;
    if (*p->kinterp) {
      MYFLT fndx;
      long indx;
      MYFLT fract;
      long indxp1;
      MYFLT     v1, v2 ;
      fndx = (*p->ixmode) ? *p->xndx * p->xbmul : *p->xndx;
      if (fndx >= len)
        fndx = (MYFLT) fmod(fndx, len);
      indx = (long) fndx;
      fract = fndx - indx;
      indxp1 = (indx < len-1) ? (indx+1) * nargs : 0;
      indx *=nargs;
      for (j=0; j < nargs; j++) {
        v1 = table[indx + j];
        v2 = table[indxp1 + j];
        **out++ = v1 + (v2 - v1) * fract;
      }
    }
    else {
      long indx = (*p->ixmode) ? ((long)(*p->xndx * p->xbmul) % len) * nargs :
                                 ((long) *p->xndx % len ) * nargs ;
      for (j=0; j < nargs; j++)
        **out++ =  table[indx + j];
    }
    return OK;
}

static int mtable_a(CSOUND *csound,MTABLE *p)
{
    int j, nargs = p->nargs;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
     uint32_t k, nsmps = CS_KSMPS;
    int ixmode = (int) *p->ixmode;
    MYFLT **out = p->outargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx, xbmul;
    long len;

    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtablea: incorrect table number"));
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table = p->ftable;
    len = p->len;
    xbmul = p->xbmul;
    if (UNLIKELY(offset))
      for (j=0; j < nargs; j++)
        memset(out[j], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nargs; j++)
        memset(&out[j][nsmps], '\0', early*sizeof(MYFLT));
    }
    if (*p->kinterp) {
      MYFLT fndx;
      long indx;
      MYFLT fract;
      long indxp1;
      for (k=offset; k<nsmps; k++) {
        MYFLT   v1, v2 ;
        fndx = (ixmode) ? *xndx++ * xbmul : *xndx++;
        if (fndx >= len)
          fndx = (MYFLT) fmod(fndx, len);
        indx = (long) fndx;
        fract = fndx - indx;
        indxp1 = (indx < len-1) ? (indx+1) * nargs : 0L;
        indx *=nargs;
        for (j=0; j < nargs; j++) {
          v1 = table[indx + j];
          v2 = table[indxp1 + j];
          out[j][k] = v1 + (v2 - v1) * fract;

        }
      }
    }
    else {
      for (k=offset; k<nsmps; k++) {
        long indx = (ixmode) ? ((long)(*xndx++ * xbmul)%len) * nargs :
                               ((long) *xndx++ %len) * nargs;
        for (j=0; j < nargs; j++) {
          out[j][k] =  table[indx + j];
        }
      }
    }
    return OK;
}

static int mtab_i(CSOUND *csound,MTABI *p)
{
    FUNC *ftp;
    int j, nargs;
    long indx;
    MYFLT *table, **out = p->outargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtabi: incorrect table number"));
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-2;

    indx = (long) *p->xndx;
    for (j=0; j < nargs; j++)
      **out++ =  table[indx * nargs + j];
    return OK;
}

static int mtab_set(CSOUND *csound,MTAB *p)     /* mtab by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtab: incorrect table number"));
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-2;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    return OK;
}

static int mtab_k(CSOUND *csound,MTAB *p)
{
    int j, nargs = p->nargs;
    MYFLT **out = p->outargs;
    MYFLT *table;
    long len, indx;

    table= p->ftable;
    len = p->len;
    indx = ((long) *p->xndx % len ) * nargs ;
    for (j=0; j < nargs; j++)
      **out++ =  table[indx + j];
    return OK;
}

static int mtab_a(CSOUND *csound,MTAB *p)
{
    int j, nargs = p->nargs;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t k, nsmps = CS_KSMPS;
    MYFLT **out = p->outargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx;
    long len;
    table = p->ftable;
    len = p->len;
    if (UNLIKELY(offset))
      for (j=0; j < nargs; j++)
        memset(out[j], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nargs; j++)
        memset(&out[j][nsmps], '\0', early*sizeof(MYFLT));
    }
    for (k=offset;k<nsmps;k++) {
      long indx = ((long) *xndx++ %len) * nargs;
      for (j=0; j < nargs; j++) {
        out[j][k] =  table[indx + j];
      }
    }
    return OK;
}

/* ---------- mtab end --------------- */

static int mtablew_i(CSOUND *csound,MTABLEIW *p)
{
    FUNC *ftp;
    int j, nargs;
    long indx;
    MYFLT *table, xbmul = FL(0.0), **in = p->inargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtablewi: incorrect table number"));
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-3;
    if (*p->ixmode)
      xbmul = (MYFLT) (ftp->flen / nargs);
    indx = (*p->ixmode) ? (long)(*p->xndx * xbmul) : (long) *p->xndx;
    for (j=0; j < nargs; j++)
      table[indx * nargs + j] = **in++;
    return OK;
}

static int mtablew_set(CSOUND *csound,MTABLEW *p)   /* mtabw by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtablew: incorrect table number"));
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-3;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    if (*p->ixmode)
      p->xbmul = (MYFLT) ftp->flen / p->nargs;
    return OK;
}

static int mtablew_k(CSOUND *csound,MTABLEW *p)
{
    int j, nargs = p->nargs;
    MYFLT **in = p->inargs;
    MYFLT *table;
    long len, indx;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtablewk: incorrect table number"));
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (*p->ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table= p->ftable;
    len = p->len;
    indx = (*p->ixmode) ? ((long)(*p->xndx * p->xbmul) % len) * nargs :
                          ((long) *p->xndx % len ) * nargs ;
    for (j=0; j < nargs; j++)
      table[indx + j] = **in++;
    return OK;
}

static int mtablew_a(CSOUND *csound,MTABLEW *p)
{
    int j, nargs = p->nargs;
    int ixmode = (int) *p->ixmode;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t k, nsmps = CS_KSMPS;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT **in = p->inargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx, xbmul;
    long len;

    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtablewa: incorrect table number"));
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table = p->ftable;
    len = p->len;
    xbmul = p->xbmul;
    if (UNLIKELY(early)) nsmps -= early;
    for (k=offset; k<nsmps; k++) {
      long indx = (ixmode) ? ((long)(*xndx++ * xbmul)%len) * nargs :
                             ((long) *xndx++ %len) * nargs;
      for (j=0; j < nargs; j++) {
        table[indx + j] = in[j][k];
      }
    }
    return OK;
}

/* -------------------------------------- */

static int mtabw_i(CSOUND *csound, MTABIW *p)
{
    FUNC *ftp;
    int j, nargs;
    long indx;
    MYFLT *table, **in = p->inargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, Str("vtabwi: incorrect table number"));
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-2;
    indx = (long) *p->xndx;
    for (j=0; j < nargs; j++)
      table[indx * nargs + j] = **in++;
    return OK;
}

static int mtabw_set(CSOUND *csound,MTABW *p)   /* mtabw by G.Maldonado */
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL)) {
      return csound->InitError(csound, Str("vtablew: incorrect table number"));
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-2;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    return OK;
}

static int mtabw_k(CSOUND *csound,MTABW *p)
{
    int j, nargs = p->nargs;
    MYFLT **in = p->inargs;
    MYFLT *table;
    long len, indx;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtablewk: incorrect table number"));
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
    }
    table= p->ftable;
    len = p->len;
    indx = ((long) *p->xndx % len ) * nargs ;
    for (j=0; j < nargs; j++)
      table[indx + j] = **in++;
    return OK;
}

static int mtabw_a(CSOUND *csound,MTABW *p)
{
    int j, nargs = p->nargs;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t k, nsmps = CS_KSMPS;
    MYFLT **in = p->inargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx;
    long len;

    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtabwa: incorrect table number"));
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
    }
    table = p->ftable;
    len = p->len;
    if (UNLIKELY(early)) nsmps -= early;
    for (k=offset; k<nsmps; k++) {
      long indx = ((long) *xndx++ %len) * nargs;
      for (j=0; j < nargs; j++) {
        table[indx + j] = in[j][k];
      }
    }
    return OK;
}

/* The following opcodes come from CsoundAV/vectorial.c */

static int vectorOp_set(CSOUND *csound, VECTOROP *p)
{
    FUNC    *ftp;

    ftp = csound->FTnp2Find(csound, p->ifn);
    if (ftp == NULL)
      return NOTOK;
    p->vector = ftp->ftable;
    /* long elements = (long) *p->kelements; */
    p->len = (long) ftp->flen;
    /* p->dstoffset = (long) *p->kdstoffset;
     if ((elements | (long)*p->kdstoffset) < 0L ||
          (elements + (long)*p->kdstoffset) > p->len) {
       return csound->InitError(csound,
                      Str("vectorop: Destination table length exceeded"));
     } */
    return OK;
}

static int vadd_i(CSOUND *csound, VECTOROPI *p)
{
    FUNC    *ftp;
    MYFLT   *vector;
    int32    i, elements, dstoffset, len;
    MYFLT   value = *p->kval;

    ftp = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))  {
      return csound->InitError(csound,
                               Str("vadd_i: invalid table number %i"),
                               (int) *p->ifn);
    }
    vector = ftp->ftable;
    len = (int32) ftp->flen;
    elements = MYFLT2LRND(*p->ielements);
    dstoffset = MYFLT2LRND(*p->idstoffset);
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      csound->Warning(csound,Str("vadd_i: ifn length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] += value;
    return OK;
}

static int vaddk(CSOUND *csound, VECTOROP *p)
{
    int i, len;
    int32 dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector;
    MYFLT value;
    vector = p->vector;
    value = *p->kval;
    len = p->len;
    dstoffset = MYFLT2LRND(*p->kdstoffset);
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vadd: ifn1 length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] += value;
    return OK;
}

static int vmult_i(CSOUND *csound, VECTOROPI *p)
{
    FUNC    *ftp;
    MYFLT   *vector;
    int32    i, elements, dstoffset, len;
    MYFLT   value = *p->kval;

    ftp = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))  {
      return csound->InitError(csound,Str("vadd_i: invalid table number %i"),
                        (int) *p->ifn);
    }
    vector = ftp->ftable;
    len = (int32) ftp->flen;
    elements = MYFLT2LRND(*p->ielements);
    dstoffset = MYFLT2LRND(*p->idstoffset);
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      csound->Warning(csound,Str("vmult_i: ifn length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] *= value;
    return OK;
}

static int vmultk(CSOUND *csound, VECTOROP *p)
{
    int i, len;
    int32 dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector;
    MYFLT value;
    vector = p->vector;
    value = (MYFLT)*p->kval;
    len = p->len;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vmult: ifn1 length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] *= value;
    return OK;
}

static int vpow_i(CSOUND *csound, VECTOROPI *p)
{
    FUNC    *ftp;
    MYFLT   *vector;
    int32    i, elements, dstoffset, len;
    MYFLT   value = *p->kval;

    ftp = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))  {
      csound->InitError(csound,Str("vpow_i: invalid table number %i"),
                        (int) *p->ifn);
      return NOTOK;
    }
    vector = ftp->ftable;
    len = (int32) ftp->flen;
    elements = (int32) *p->ielements;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      csound->Warning(csound,Str("vpow_i: ifn length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] = POWER(vector[i], value);

    return OK;
}

static int vpowk(CSOUND *csound, VECTOROP *p)
{
    int i, len;
    int32 dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector;
    MYFLT value;
    vector = p->vector;
    value = (MYFLT)*p->kval;
    len = p->len;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vpow: ifn1 length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] = POWER(vector[i], value);
    return OK;
}

static int vexp_i(CSOUND *csound, VECTOROPI *p)
{
    FUNC    *ftp;
    MYFLT   *vector;
    int32    i, elements, dstoffset, len;
    MYFLT   value = *p->kval;

    ftp = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))  {
      return csound->InitError(csound,Str("vexp_i: invalid table number %i"),
                               (int) *p->ifn);
    }
    vector = ftp->ftable;
    len = (int32) ftp->flen;
    elements = (int32) *p->ielements;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      csound->Warning(csound,Str("vexp_i: ifn length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] = POWER(value, vector[i]);
    return OK;
}

static int vexpk(CSOUND *csound, VECTOROP *p)
{
    int i, len;
    int32 dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector;
    MYFLT value;
    vector = p->vector;
    value = (MYFLT)*p->kval;
    len = p->len;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
    }
    else {
      len -= dstoffset;
      vector += dstoffset;
    }
    if (UNLIKELY(elements > len))  {
      elements = len;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vexp: ifn1 length exceeded"));
    }
    for (i = 0; i < elements; i++)
      vector[i] += POWER(value, vector[i]);
    return OK;
}

/* ------------------------- */

static int vectorsOp_set(CSOUND *csound, VECTORSOP *p)
{
    FUNC        *ftp1, *ftp2;
/*     if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vectorsop: ifn1 = ifn2."));*/
    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL))  {
      return csound->InitError(csound,
                               Str("vectorsop: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL)) {
      return csound->InitError(csound,
                               Str("vectorsop: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
    p->vector1 = ftp1->ftable;
    p->vector2 = ftp2->ftable;
    /*int elements = (int) *p->kelements;
    p->dstoffset = (int32) *p->kdstoffset;
    p->srcoffset = (int32) *p->ksrcoffset;*/
    p->len1 = (int32) ftp1->flen+1;  /* Guard point included */
    p->len2 = (int32) ftp2->flen+1;/*
    if ((elements | (int32) *p->kdstoffset) < 0L ||
         (elements + (int32) *p->kdstoffset)  > p->len1) {
      return csound->Warning(csound,
                             Str("vectorops: Destination table length exceeded"));
    }*/
    return OK;
}

static int vcopy(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vcopy: ifn1 length exceeded"));
    }
     /*elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int   i, */n = -srcoffset;
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = 0;
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
        /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vcopy: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction*/
      for (j = n; --j >= 0; i++)
        vector1[j] = vector2[j];
    }
    for (; i < n; i++)
      vector1[i] = vector2[i];
    for ( ; i < elements; i++)
      vector1[i] = FL(0.0);
    return OK;
}

static int vcopy_i(CSOUND *csound, VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, j, n, elements, srcoffset, dstoffset, len1, len2;

    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL))  {
      return csound->InitError(csound,
                               Str("vcopy_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vcopy_i: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
/*     if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vcopy_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vcopy_i: ifn1 length exceeded"));
    }
/*     elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int   i, */n = -srcoffset;
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
/*     n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vcopy_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction*/
      for (j = n; --j >= 0; i++)
        vector1[j] = vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] = vector2[i];
    for ( ; i < elements; i++)
      vector1[i] = FL(0.0);
    return OK;
}

static int vaddvk(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vaddv: ifn1 length exceeded"));
    }
/*     elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int i, n = -srcoffset; */
      i = -srcoffset;
      /*n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = 0;*/
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vaddv: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction*/
      for (j = n; --j >= 0; i++)
        vector1[j] += vector2[j];
    }
    for (; i < n; i++)
      vector1[i] += vector2[i];
    /*for ( ; i < elements; i++)
      vector1[i] = 0;*/
    return OK;
}

static int vaddv_i(CSOUND *csound, VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, n, elements, srcoffset, dstoffset, len1, len2;
    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL)) {
      return csound->InitError(csound,
                               Str("vaddv_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vaddv_i: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
    /* if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vaddv_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vaddv_i: ifn1 length exceeded"));
    }
    /*elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int   i, n = -srcoffset; */
      n = -srcoffset;
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (elements > len2) {
      csound->Warning(csound,Str("vaddv_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    for (i = 0; i < n; i++)
      vector1[i] += vector2[i];
    return OK;
}

static int vsubvk(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vsubv: ifn1 length exceeded"));
    }
        /* elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
        i = -srcoffset;
      /*int i, n = -srcoffset;
       n = (n < elements ? n : elements);
       for (i = 0; i < n; i++)
         vector1[i] = 0;*/
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
          if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vsubv: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] -= vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] -= vector2[i];
    /*for ( ; i < elements; i++)
      vector1[i] = 0;*/
    return OK;
}

static int vsubv_i(CSOUND *csound, VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, j, n, elements, srcoffset, dstoffset, len1, len2;

    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL)) {
      return csound->InitError(csound,
                               Str("vsubv_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vsubv_i: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
/*     if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vsubv_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vsubv_i: ifn1 length exceeded"));
    }
    /* elements = (elements < len1 ? elements : len1); */
    if (srcoffset < 0) {
          n = -srcoffset;
      /*int   i, n = -srcoffset; */
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = 0;
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
        /* n = (elements < len2 ? elements : len2); */
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vsubv_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] -= vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] -= vector2[i];
    return OK;
}

static int vmultvk(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vmultv: ifn1 length exceeded"));
    }
    /*elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      i = -srcoffset;
      /*int i, n = -srcoffset;
       n = (n < elements ? n : elements);
       for (i = 0; i < n; i++)
         vector1[i] = 0;*/
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
      if (UNLIKELY(elements > len2)) {
        if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vmultv: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] *= vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] *= vector2[i];
    /* for ( ; i < elements; i++)
      vector1[i] = 0;*/
    return OK;
}

static int vmultv_i(CSOUND *csound, VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, j, n, elements, srcoffset, dstoffset, len1, len2;

    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL)) {
      return csound->InitError(csound,
                               Str("vmultv_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vmultv_i: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
/*     if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vmultv_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp1->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vmultv_i: ifn1 length exceeded"));
    }
    /* elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int   i, */n = -srcoffset;
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vmultv_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] *= vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] *= vector2[i];
    return OK;
}

static int vdivvk(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vdivv: ifn1 length exceeded"));
    }
        /* elements = (elements < len1 ? elements : len1); */
    if (srcoffset < 0) {
      i = -srcoffset;
      /*int i, n = -srcoffset;
       n = (n < elements ? n : elements);
       for (i = 0; i < n; i++)
         vector1[i] = 0;*/
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vdivv: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] /= vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] /= vector2[i];
    /* for ( ; i < elements; i++)
      vector1[i] = 0;*/
    return OK;
}

static int vdivv_i(CSOUND *csound, VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, j, n, elements, srcoffset, dstoffset, len1, len2;

    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL)) {
      return csound->InitError(csound,
                               Str("vdivv_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vdivv_i: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
     /*if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vdivv_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vdivv_i: ifn1 length exceeded"));
    }
    /* elements = (elements < len1 ? elements : len1); */
    if (srcoffset < 0) {
      n = -srcoffset;
      /*int   i, n = -srcoffset; */
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vdivv_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /*special case: need to reverse direction*/
      for (j = n; --j >= 0; i++)
        vector1[j] = vector2[j];
    }
    for ( ; i < n; i++)
      vector1[i] /= vector2[i];
    return OK;
}

static int vpowvk(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vpowv: ifn1 length exceeded"));
    }
    /*elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      i = -srcoffset;
      /*int i, n = -srcoffset;
       n = (n < elements ? n : elements);
       for (i = 0; i < n; i++)
         vector1[i] = 0; */
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
        /* n = (elements < len2 ? elements : len2); */
    if (UNLIKELY(elements > len2)) {
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vpowv: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] = POWER(vector1[j], vector2[j]);
    }
    for ( ; i < n; i++)
      vector1[i] = POWER(vector1[i], vector2[i]);
    /*for ( ; i < elements; i++)
      vector1[i] = 0;*/
    return OK;
}

static int vpowv_i(CSOUND *csound, VECTORSOPI *p)
{
  FUNC    *ftp1, *ftp2;
  MYFLT   *vector1, *vector2;
  int32    i, j, n, elements, srcoffset, dstoffset, len1, len2;

  ftp1 = csound->FTnp2Find(csound, p->ifn1);
  ftp2 = csound->FTnp2Find(csound, p->ifn2);
  if (UNLIKELY(ftp1 == NULL)) {
      return csound->InitError(csound,
                               Str("vpowv_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
  else if (UNLIKELY(ftp2 == NULL))  {
    return csound->InitError(csound,
                             Str("vpowv_i: ifn2 invalid table number %i"),
                             (int) *p->ifn2);
    }
     /*if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vpowv_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vpowv_i: ifn1 length exceeded"));
    }
        /*elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int   i, */n = -srcoffset;
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vpowv_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /*special case: need to reverse direction*/
      for (j = n; --j >= 0; i++)
        vector1[j] = POWER(vector1[j], vector2[j]);
    }
    for (i = 0; i < n; i++)
      vector1[i] = POWER(vector1[i], vector2[i]);
    return OK;
}

static int vexpvk(CSOUND *csound,VECTORSOP *p)
{
    int i, j, n;
    int32 len1, len2, srcoffset, dstoffset, elements = (int32)*p->kelements;
    MYFLT *vector1, *vector2;
    vector1 = p->vector1;
    vector2 = p->vector2;
    len1 = p->len1;
    len2 = p->len2;
    srcoffset = (int32)*p->ksrcoffset;
    dstoffset = (int32)*p->kdstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vexpv: ifn1 length exceeded"));
    }
        /* elements = (elements < len1 ? elements : len1); */
    if (srcoffset < 0) {
      i = -srcoffset;
      /*int i, n = -srcoffset;
       n = (n < elements ? n : elements);
       for (i = 0; i < n; i++)
         vector1[i] = 0; */
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
     /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      if (UNLIKELY((int) *p->kverbose != 0))
        csound->Warning(csound,Str("vexpv: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] = POWER(vector2[j], vector1[j]);
    }
    for ( ; i < n; i++)
      vector1[i] = POWER(vector2[i], vector1[i]);
    /* for ( ; i < elements; i++)
      vector1[i] = 1;*/
    return OK;
}

static int vexpv_i(CSOUND *csound, VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, j, n, elements, srcoffset, dstoffset, len1, len2;

    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL)) {
      return csound->InitError(csound,
                               Str("vexpv_i: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vexpv_i: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
    /*if (*p->ifn1 == *p->ifn2)
      csound->Warning(csound, Str("vexpv_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vexpv_i: ifn1 length exceeded"));
    }
    /* elements = (elements < len1 ? elements : len1); */
    if (srcoffset < 0) {
      n = -srcoffset;
      /*int   i, n = -srcoffset; */
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
    /*n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vexpv_i: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    if (p->vector1 == p->vector2 && vector1 > vector2) {
      /* special case: need to reverse direction */
      for (j = n; --j >= 0; i++)
        vector1[j] = POWER(vector2[j], vector1[j]);
    }
    for ( ; i < n; i++)
      vector1[i] = POWER(vector2[i], vector1[i]);
    return OK;
}

/*static int vmap(CSOUND *csound,VECTORSOP *p)
{
    int elements = *p->kelements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1 = (vector2++)[(int)*vector1];
      vector1++;
    } while (--elements);
    return OK;
}*/

static int vmap_i(CSOUND *csound,VECTORSOPI *p)
{
    FUNC    *ftp1, *ftp2;
    MYFLT   *vector1, *vector2;
    int32    i, n, elements, srcoffset, dstoffset, len1, len2;

    ftp1 = csound->FTnp2Find(csound, p->ifn1);
    ftp2 = csound->FTnp2Find(csound, p->ifn2);
    if (UNLIKELY(*p->ifn1 == *p->ifn2)) {
      return csound->InitError(csound,
                               Str("vmap: Error: ifn1 and ifn2 can not "
                                   "be the same"));
    }
    if (UNLIKELY(ftp1 == NULL))  {
      return csound->InitError(csound,
                               Str("vmap: ifn1 invalid table number %i"),
                               (int) *p->ifn1);
    }
    else if (UNLIKELY(ftp2 == NULL))  {
      return csound->InitError(csound,
                               Str("vmap: ifn2 invalid table number %i"),
                               (int) *p->ifn2);
    }
/*     if (*p->ifn1 == *p->ifn2)
       csound->Warning(csound, Str("vcopy_i: ifn1 = ifn2."));*/
    vector1 = ftp1->ftable;
    vector2 = ftp2->ftable;
    len1 = (int32) ftp1->flen+1;
    len2 = (int32) ftp2->flen+1;
    elements = (int32) *p->ielements;
    srcoffset = (int32) *p->isrcoffset;
    dstoffset = (int32) *p->idstoffset;
    if (dstoffset < 0) {
      elements += dstoffset;
      srcoffset -= dstoffset;
    }
    else {
      len1 -= dstoffset;
      vector1 += dstoffset;
    }
    if (UNLIKELY(elements > len1))  {
      elements = len1;
      csound->Warning(csound,Str("vmap: ifn1 length exceeded"));
    }
/*     elements = (elements < len1 ? elements : len1);*/
    if (srcoffset < 0) {
      /*int   i, */n = -srcoffset;
      n = (n < elements ? n : elements);
      for (i = 0; i < n; i++)
        vector1[i] = FL(0.0);
      elements -= i;
      vector1 += i;
    }
    else {
      len2 -= srcoffset;
      vector2 += srcoffset;
    }
/*     n = (elements < len2 ? elements : len2);*/
    if (UNLIKELY(elements > len2)) {
      csound->Warning(csound,Str("vmap: ifn2 length exceeded"));
      n = len2;
    }
    else n = elements;
    i = 0;
    for ( ; i < n; i++)
      vector1[i] = vector2[(int)vector1[i]];
    for ( ; i < elements; i++)
      vector1[i] = FL(0.0);
    return OK;
}

static int vlimit_set(CSOUND *csound,VLIMIT *p)
{
    FUNC        *ftp;
    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    else return NOTOK;
    if (UNLIKELY(p->elements > (int)ftp->flen )) {
      return csound->InitError(csound, Str("vectorop: invalid num of elements"));
    }
    return OK;
}

static int vlimit(CSOUND *csound,VLIMIT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT min = *p->kmin, max = *p->kmax;
    do {
      *vector = (*vector > min) ? ((*vector < max) ? *vector : max) : min;
      vector++;
    } while (--elements);
    return OK;
}

/*-----------------------------*/
static int vport_set(CSOUND *csound,VPORT *p)
{
    FUNC        *ftp;
    int elements;
    MYFLT /* *vector,*/ *yt1,*vecInit  = NULL;

    if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL)) {
      p->vector = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
      if (UNLIKELY(elements > (int)ftp->flen) )
        return csound->InitError(csound,
                                 Str("vport: invalid table length or "
                                     "num of elements"));
    }
    else return csound->InitError(csound, Str("vport: invalid table"));
    if (LIKELY(*p->ifnInit)) {
      if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifnInit)) != NULL)) {
        vecInit = ftp->ftable;
        if (UNLIKELY(elements > (int)ftp->flen) )
          return csound->InitError(csound, Str("vport: invalid init table length"
                                               " or num of elements"));
      }
      else return csound->InitError(csound, Str("vport: invalid init table"));
    }
    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, elements * sizeof(MYFLT), &p->auxch);
    yt1 = (p->yt1 = (MYFLT *) p->auxch.auxp);
    if (vecInit) {
      do {
        *yt1++ = *vecInit++;
      } while (--elements);
    }
    else {
      do {
        *yt1++ = FL(0.0);
      } while (--elements);
    }
    p->prvhtim = -FL(100.0);
    return OK;
}

static int vport(CSOUND *csound,VPORT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector, *yt1 = p->yt1, c1, c2;
    if (p->prvhtim != *p->khtim) {
      p->c2 = (MYFLT)pow(0.5, (double)CS_ONEDKR / *p->khtim);
      p->c1 = FL(1.0) - p->c2;
      p->prvhtim = *p->khtim;
    }
    c1 = p->c1;
    c2 = p->c2;
    do {
      *vector = (*yt1 = c1 * *vector + c2 * *yt1);
      vector++; yt1++;
    } while (--elements);
    return OK;
}

/*-------------------------------*/
static int vwrap(CSOUND *csound,VLIMIT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT min = *p->kmin, max = *p->kmax;

    if (min >= max) {
      MYFLT average = (min+max)/2;
      do {
        *vector++ = average;
      } while (--elements);
    }
    else {
      do {
        if (*vector >= max) {
          *vector = min + FMOD(*vector - min, FABS(min-max));
          vector++;
        }
        else {
          *vector = max - FMOD(max - *vector, FABS(min-max));
          vector++;
        }
      } while (--elements);
    }
    return OK;
}

static int vmirror(CSOUND *csound,VLIMIT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT min = *p->kmin, max = *p->kmax;

    if (min >= max) {
      MYFLT average = (min+max)* FL(0.50);
      do {
        *vector++ = average;
      } while (--elements);
    }
    else {
      do {
        while (!((*vector <= max) && (*vector >=min))) {
          if (*vector > max)
            *vector = max + max - *vector;
          else
            *vector = min + min - *vector;
        }
        vector++;
      } while (--elements);
    }
    return OK;
}

#define RNDMUL  15625
// #define MASK16  0xFFFF
// #define MASK15  0x7FFF
#define BIPOLAR 0x7FFFFFFF      /* Constant to make bipolar */
#define RIA 16807               /* multiplier */
#define RIM 0x7FFFFFFFL         /* 2**31 - 1 */

#define dv2_31          (FL(4.656612873077392578125e-10))

static int32 randint31(int32 seed31)
{
    uint32 rilo, rihi;

    rilo = RIA * (int32)(seed31 & 0xFFFF);
    rihi = RIA * (int32)((uint32)seed31 >> 16);
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
    return (int32)rilo;
}

static int vrandh_set(CSOUND *csound,VRANDH *p)
{
    FUNC        *ftp;
    int elements = 0;
    MYFLT *num1;
    uint32 seed;
    int32 r;

    if (*p->iseed >= FL(0.0)) {                       /* new seed:*/
      if (*p->iseed > FL(1.0)) {    /* Seed from current time */
        seed = csound->GetRandomSeedFromTime();
        if (*p->isize == 0) {
          p->rand = (int32) (seed & 0xFFFFUL);
        }
        else {
          p->rand = (int32) (seed % 0x7FFFFFFEUL) + 1L;
        }
        csound->Message(csound,
                        Str("vrandh: Seeding from current time %lu\n"),
                        seed);
      }
      else {
        if (*p->isize == 0)
          p->rand = 0xffff&(short)(*p->iseed * 32768L);   /* init rand integ    */
        else
          p->rand = (int32) (*p->iseed * FL(2147483648.0));
      }
      if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
        p->elements = (int) *p->ielements;
        p->offset = (int) *p->idstoffset;
      }
      else return csound->InitError(csound, Str("vrandh: Invalid table."));
      if (UNLIKELY(*p->idstoffset >= (int)ftp->flen))
        return csound->InitError(csound,
                                 Str("vrandh: idstoffset is greater than"
                                     " table length."));
      p->vector = ftp->ftable + p->offset;
      if (UNLIKELY(p->elements + p->offset > (int)ftp->flen)) {
        csound->Warning(csound,
                        Str("randh: Table length exceeded, "
                            "last elements discarded."));
        p->elements = p->offset - ftp->flen;
      }
    }
    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, p->elements * sizeof(MYFLT), &p->auxch);
    num1 = (p->num1 = (MYFLT *) p->auxch.auxp);
    r = p->rand;
    elements = p->elements;
    do {
      if (*p->isize == 0) {
        *num1++ = (MYFLT) ((short) r) * DV32768;
        r = (int32) (r & 0xFFFFUL);
      }
      else {
        // 31-bit PRNG
        *num1++ = (MYFLT)((int32)((unsigned)r<<1)-BIPOLAR) * dv2_31;
        r = randint31( r);
      }
    } while (--elements);
    p->phs = 0;
    p->rand = r;
    return OK;
}

static int vrandh(CSOUND *csound,VRANDH *p)
{
    MYFLT *vector = p->vector, *num1 = p->num1;
    MYFLT value = *p->krange;
    int elements = p->elements;
    int32 r;

    do {
      *vector++ = (*num1++ * value) + *p->ioffset;
    } while (--elements);

    p->phs += (int32)(*p->kcps * CS_KICVT);
    if (p->phs >= MAXLEN) {
      p->phs &= PHMASK;
      elements = p->elements;
      vector = p->vector;
      num1 = p->num1;
      r = p->rand;
      do {
        if (*p->isize == 0) {
          *num1++ = (MYFLT) ((short) r) * DV32768;
          r *= RNDMUL;
          r += 1;
        }
        else {
          // 31-bit PRNG
          *num1++ = (MYFLT)((int32)((unsigned)r<<1)-BIPOLAR) * dv2_31;
          r = randint31(r);
        }
      } while (--elements);
      p->rand = r;
    }
    return OK;
}

static int vrandi_set(CSOUND *csound,VRANDI *p)
{
    FUNC        *ftp;
    int elements = 0;
    MYFLT *dfdmax, *num1, *num2;
    uint32 seed;
    int32 r;

    if (*p->iseed >= FL(0.0)) {                       /* new seed:*/
      if (*p->iseed > FL(1.0)) {    /* Seed from current time */
        seed = csound->GetRandomSeedFromTime();
        if (*p->isize == 0) {
          p->rand = (int32) (seed & 0xFFFFUL);
        }
        else {
          p->rand = (int32) (seed % 0x7FFFFFFEUL) + 1L;
        }
        csound->Message(csound,
                        Str("vrandi: Seeding from current time %lu\n"), seed);
      }
      else {
        if (*p->isize == 0)
          p->rand = 0xffff&(short)(*p->iseed * 32768L);   /* init rand integ    */
        else
          p->rand = (int32) (*p->iseed * FL(2147483648.0));
      }
      if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL)) {
        p->elements = (int) *p->ielements;
        p->offset = (int) *p->idstoffset;
      }
      else return csound->InitError(csound, Str("vrandi: Invalid table."));
      if (UNLIKELY(p->offset >= (int)ftp->flen))
        return csound->InitError(csound,
                                 Str("vrandi: idstoffset is greater than"
                                     "table length."));
      p->vector = ftp->ftable + p->offset;
      if (UNLIKELY(p->elements > (int)ftp->flen)) {
        csound->Warning(csound,
                        Str("vrandi: Table length exceeded, "
                            "last elements discarded."));
        p->elements = p->offset - ftp->flen;
      }
    }
    if (p->auxch.auxp == NULL) {
      csound->AuxAlloc(csound, p->elements * sizeof(MYFLT) * 3, &p->auxch);
    }
    elements = p->elements;
    num1 = (p->num1 = (MYFLT *) p->auxch.auxp);
    num2 = (p->num2 = &num1[elements]);
    dfdmax = (p->dfdmax = &num1[elements * 2]);
    r = p->rand;
    do {
      *num1 = FL(0.0);
      if (*p->isize == 0) {
        *num2 = (MYFLT) ((short) r) * DV32768;
        r = (int32) (r & 0xFFFFUL);
      }
      else {
        // 31-bit PRNG
        *num2 = (MYFLT)((int32)((unsigned)r<<1)-BIPOLAR) * dv2_31;
        r = randint31(r);
      }
      *dfdmax++ = (*num2++ - *num1++) / FMAXLEN;
    } while (--elements);
    p->phs = 0;
    p->rand = r;
    return OK;
}

static int vrandi(CSOUND *csound,VRANDI *p)
{
    MYFLT *vector = p->vector, *num1 = p->num1, *num2, *dfdmax = p->dfdmax;
    MYFLT value = *p->krange;
    int elements = p->elements;
    int32 r;

    do {
      *vector++ = (((MYFLT)*num1++ + ((MYFLT)p->phs * *dfdmax++)) * value) +
        *p->ioffset;
    } while (--elements);

    p->phs += (int32)(*p->kcps * CS_KICVT);
    if (p->phs >= MAXLEN) {
      p->phs &= PHMASK;
      elements = p->elements;
      vector = p->vector;
      num1 = p->num1;
      num2 = p->num2;
      dfdmax = p->dfdmax;
      r = p->rand;
      do {
        *num1 = *num2;
        if (*p->isize == 0) {
          *num2 = (MYFLT) ((short) r) * DV32768;
          r *= RNDMUL;                         /*      recalc random   */
          r += 1;
        }
        else {
          // 31-bit PRNG
          *num2 = (MYFLT)((int32)((unsigned)r<<1)-BIPOLAR) * dv2_31 ;
          r = randint31(r);
        }
        *dfdmax++ = ((MYFLT)*num2++ - (MYFLT)*num1++) / FMAXLEN;
      } while (--elements);
      p->rand = r;
    }
    return OK;
}

static int vecdly_set(CSOUND *csound, VECDEL *p)
{
    FUNC        *ftp;
    int elements = (p->elements = (int) *p->ielements), j;
    int32 n;

    if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifnOut)) != NULL)) {
      p->outvec = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
      if (UNLIKELY( elements > (int)ftp->flen ))
        return csound->InitError(csound,
                                 Str("vecdelay: invalid num of elements"));
    }
    else return csound->InitError(csound, Str("vecdly: invalid output table"));
    if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifnIn)) != NULL)) {
      p->invec = ftp->ftable;
      if (UNLIKELY(elements > (int)ftp->flen))
        return csound->InitError(csound,
                                 Str("vecdelay: invalid num of elements"));
    }
    else return csound->InitError(csound, Str("vecdly: invalid input table"));
    if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifnDel)) != NULL)) {
      p->dlyvec = ftp->ftable;
      if (UNLIKELY( elements > (int)ftp->flen ))
        return csound->InitError(csound,
                                 Str("vecdelay: invalid num of elements"));
    }
    else return csound->InitError(csound, Str("vecdly: invalid delay table"));

    n = (p->maxd = (int32) (*p->imaxd * CS_EKR));
    if (n == 0) n = (p->maxd = 1);

    if (!*p->istod) {
      if (p->aux.auxp == NULL ||
          (unsigned int)(elements * sizeof(MYFLT *)
                + n * elements * sizeof(MYFLT)
                + elements * sizeof(int32)) > p->aux.size) {
        csound->AuxAlloc(csound, elements * sizeof(MYFLT *)
                 + n * elements * sizeof(MYFLT)
                 + elements * sizeof(int32),
                 &p->aux);
        p->buf= (MYFLT **) p->aux.auxp;
        for (j = 0; j < elements; j++) {
          p->buf[j] = (MYFLT*) ((char*) p->aux.auxp + sizeof(MYFLT*) * elements
                                                    + sizeof(MYFLT) * n * j);
        }
        p->left = (int32*) ((char*) p->aux.auxp + sizeof(MYFLT*) * elements
                                               + sizeof(MYFLT) * n * elements);
      }
      else {
        MYFLT **buf= p->buf;
        for (j = 0; j < elements; j++) {
          MYFLT *temp = buf[j];
          int count = n;
          /* memset(buf[j], 0, sizeof(MYFLT)*n); */
          do {
            *temp++ = FL(0.0);
          } while (--count);
          p->left[j] = 0;
        }
      }
    }
    return OK;
}

static int vecdly(CSOUND *csound,VECDEL *p)
{
    int32 maxd = p->maxd, *indx=p->left, v1, v2;
    MYFLT **buf = p->buf, fv1, fv2, *inVec = p->invec;
    MYFLT *outVec = p->outvec, *dlyVec = p->dlyvec;
    int elements = p->elements;
    if (UNLIKELY(buf==NULL)) {
      return csound->InitError(csound, Str("vecdly: not initialised"));
    }
    do {
      (*buf)[*indx] = *inVec++;
      fv1 = *indx - *dlyVec++ * CS_EKR;
      while (fv1 < FL(0.0))     fv1 += (MYFLT)maxd;
      while (fv1 >= (MYFLT)maxd) fv1 -= (MYFLT)maxd;
      if (fv1 < maxd - 1) fv2 = fv1 + 1;
      else                fv2 = FL(0.0);
      v1 = (int32)fv1;
      v2 = (int32)fv2;
      *outVec++ = (*buf)[v1] + (fv1 - v1) * ((*buf)[v2]-(*buf)[v1]);
      ++buf;
      if (++(*indx) == maxd) *indx = 0;
      ++indx;
    } while (--elements);
    return OK;
}

static int vseg_set(CSOUND *csound,VSEG *p)
{
    TSEG        *segp;
    int nsegs;
    MYFLT       **argp, dur, *vector;
    FUNC *nxtfunc, *curfunc, *ftp;
    int32        flength;

    nsegs = ((p->INCOUNT-2) >> 1);      /* count segs & alloc if nec */

    if ((segp = (TSEG *) p->auxch.auxp) == NULL) {
      csound->AuxAlloc(csound, (int32)(nsegs+1)*sizeof(TSEG), &p->auxch);
      p->cursegp = segp = (TSEG *) p->auxch.auxp;
      (segp+nsegs)->cnt = MAXPOS;
    }
    argp = p->argums;
    if (UNLIKELY((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL))
      return NOTOK;
    if ((ftp = csound->FTnp2Find(csound,p->ioutfunc)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    else return NOTOK;
    if (UNLIKELY( p->elements > (int)ftp->flen ))
      return csound->InitError(csound,
                               Str("vlinseg/vexpseg: invalid num. of elements"));

    /* memset(p->vector, 0, sizeof(MYFLT)*p->elements); */
    vector = p->vector;
    flength = p->elements;

    do {
      *vector++ = FL(0.0);
    } while (--flength);

    if (UNLIKELY(**argp <= FL(0.0)))
      return NOTOK; /* if idur1 <= 0, skip init */
    p->cursegp = segp;              /* else proceed from 1st seg */
    segp--;
    do {
      segp++;           /* init each seg ..  */
      curfunc = nxtfunc;
      dur = **argp++;
      if (UNLIKELY((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL))
        return NOTOK;
      if (dur > FL(0.0)) {
        segp->d = dur * CS_EKR;
        segp->function =  curfunc;
        segp->nxtfunction = nxtfunc;
        segp->cnt = (int32) MYFLT2LRND(segp->d);
      }
      else break;               /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;
    segp->d = FL(0.0);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    return OK;
}

static int vlinseg(CSOUND *csound,VSEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, durovercnt=FL(0.0), *vector;
    int32        flength, upcnt;
    if (UNLIKELY(p->auxch.auxp==NULL)) {
      return csound->InitError(csound, Str("tableseg: not initialised"));
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (int32)segp->d-segp->cnt;
    if (upcnt > 0)
      durovercnt = segp->d/upcnt;
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = p->elements;
    vector = p->vector;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      if (durovercnt > FL(0.0))
        *vector++ = (curval + ((nxtval - curval) / durovercnt));
      else
        *vector++ = curval;
    } while (--flength);
    return OK;
}

static int vexpseg(CSOUND *csound,VSEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, cntoverdur=FL(0.0), *vector;
    int32        flength, upcnt;

    if (UNLIKELY(p->auxch.auxp==NULL)) {
      return csound->InitError(csound, Str("tablexseg: not initialised"));
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (int32)segp->d-segp->cnt;
    if (upcnt > 0) cntoverdur = upcnt/ segp->d;
    while(--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = p->elements;
    vector = p->vector;
    cntoverdur *= cntoverdur;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      *vector++ = (curval + ((nxtval - curval) * cntoverdur));
    } while (--flength);
    return OK;
}

/* ----------------------------------------------- */

#if 0
static int vphaseseg_set(CSOUND *csound,VPSEG *p)
{
    TSEG2       *segp;
    int nsegs,j;
    MYFLT       **argp,  *vector;
    double dur, durtot = 0.0, prevphs;
    FUNC *nxtfunc, *curfunc, *ftp;
    int32        flength;

    nsegs = p->nsegs =((p->INCOUNT-3) >> 1);    /* count segs & alloc if nec */

    if ((segp = (TSEG2 *) p->auxch.auxp) == NULL) {
      csound->AuxAlloc(csound, (int32)(nsegs+1)*sizeof(TSEG), &p->auxch);
      p->cursegp = segp = (TSEG2 *) p->auxch.auxp;
      /* (segp+nsegs)->cnt = MAXPOS;  */
    }
    argp = p->argums;
    if ((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL)
      return NOTOK;
    if ((ftp = csound->FTnp2Find(csound,p->ioutfunc)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    if ( p->elements > (int)ftp->flen )
      return csound->InitError(csound,
                               Str("vphaseseg: invalid num. of elements"));
    vector = p->vector;
    flength = p->elements;

    do {
      *vector++ = FL(0.0);
    } while (--flength);

    if (**argp <= FL(0.0))  return NOTOK; /* if idur1 <= 0, skip init  */
    /* p->cursegp = tempsegp =segp; */      /* else proceed from 1st seg */

    segp--;
    do {
      segp++;           /* init each seg ..  */
      curfunc = nxtfunc;
      dur = **argp++;
      if ((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL) return NOTOK;
      if (dur > FL(0.0)) {
        durtot+=dur;
        segp->d = dur; /* * CS_EKR; */
        segp->function = curfunc;
        segp->nxtfunction = nxtfunc;
        /* segp->cnt = (int32) (segp->d + .5);  */
      }
      else break;               /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;

    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    nsegs = p->nsegs;

    segp = p->cursegp;

    for (j=0; j< nsegs; j++)
      segp[j].d /= durtot;

    for (j=nsegs-1; j>= 0; j--)
      segp[j+1].d = segp[j].d;

    segp[0].d = prevphs = 0.0;

    for (j=0; j<= nsegs; j++) {
      segp[j].d += prevphs;
      prevphs = segp[j].d;
    }
    return OK;
}

static int vphaseseg(CSOUND *csound,VPSEG *p)
{

    TSEG2       *segp = p->cursegp;
    double phase = *p->kphase, partialPhase = 0.0;
    int j, flength;
    MYFLT       *curtab = NULL, *nxttab = NULL, curval, nxtval, *vector;

    while (phase >= 1.0) phase -= 1.0;
    while (phase < 0.0) phase = 0.0;

    for (j = 0; j < p->nsegs; j++) {
      TSEG2 *seg = &segp[j], *seg1 = &segp[j+1];
      if (phase < seg1->d) {
        curtab = seg->function->ftable;
        nxttab = seg1->function->ftable;
        partialPhase = (phase - seg->d) / (seg1->d - seg->d);
        break;
      }
    }

    flength = p->elements;
    vector = p->vector;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      *vector++ = (MYFLT) (curval + ((nxtval - curval) * partialPhase));
    } while (--flength);
    return OK;
}
#endif

/* ------------------------- */
static int kdel_set(CSOUND *csound,KDEL *p)
{
    uint32 n;
    n = (p->maxd = (int32) (*p->imaxd * CS_EKR));
    if (n == 0) n = (p->maxd = 1);

    if (!*p->istod) {
      if (p->aux.auxp == NULL || (unsigned int)(n*sizeof(MYFLT)) > p->aux.size)
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux);
      else {
        memset(p->aux.auxp, 0, sizeof(MYFLT)*n);
      }
      p->left = 0;
    }
    return OK;
}

static int kdelay(CSOUND *csound,KDEL *p)
{
    int32 maxd = p->maxd, indx, v1, v2;
    MYFLT *buf = (MYFLT *)p->aux.auxp, fv1, fv2;

    if (UNLIKELY(buf==NULL)) {
      return csound->InitError(csound, Str("vdelayk: not initialised"));
    }

    indx = p->left;
    buf[indx] = *p->kin;
    fv1 = indx - *p->kdel * CS_EKR;
    while (fv1 < FL(0.0))       fv1 += (MYFLT)maxd;
    while (fv1 >= (MYFLT)maxd) fv1 -= (MYFLT)maxd;
    if (*p->interp) { /*  no interpolation */
      *p->kr = buf[(int32) fv1];
    }
    else {
      if (fv1 < maxd - 1) fv2 = fv1 + 1;
      else                fv2 = FL(0.0);
      v1 = (int32)fv1;
      v2 = (int32)fv2;
      *p->kr = buf[v1] + (fv1 - v1) * (buf[v2]-buf[v1]);
    }
    if (++(p->left) == maxd) p->left = 0;
    return OK;
}

/*  From fractals.c */
static int ca_set(CSOUND *csound,CELLA *p)
{
    FUNC        *ftp;
    int elements;
    MYFLT *currLine, *initVec = NULL;

    if (LIKELY((ftp = csound->FTnp2Find(csound,p->ioutFunc)) != NULL)) {
      p->outVec = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
      if (UNLIKELY( elements > (int)ftp->flen ))
        return csound->InitError(csound, Str("cella: invalid num of elements"));
    }
    else return csound->InitError(csound, Str("cella: invalid output table"));
    if (LIKELY((ftp = csound->FTnp2Find(csound,p->initStateFunc)) != NULL)) {
      initVec = (p->initVec = ftp->ftable);
      if (UNLIKELY(elements > (int)ftp->flen ))
        return csound->InitError(csound, Str("cella: invalid num of elements"));
    }
    else return csound->InitError(csound,
                                  Str("cella: invalid initial state table"));
    if (LIKELY((ftp = csound->FTnp2Find(csound,p->iRuleFunc)) != NULL)) {
      p->ruleVec = ftp->ftable;
    }
    else return csound->InitError(csound, Str("cella: invalid rule table"));

    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, elements * sizeof(MYFLT) * 2, &p->auxch);
    currLine = (p->currLine = (MYFLT *) p->auxch.auxp);
    p->NewOld = 0;
    p->ruleLen = (int) *p->irulelen;
    /* memcpy(currLine, initVec, sizeof(MYFLT)*elements); */
    do {
      *currLine++ = *initVec++;
    } while (--elements);
    return OK;
}

static int ca(CSOUND *csound,CELLA *p)
{
    if (*p->kreinit) {
      MYFLT *currLine = p->currLine, *initVec = p->initVec;
      int elements =  p->elements;
      p->NewOld = 0;
     /* memcpy(currLine, initVec, sizeof(MYFLT)*elements); */
     do {
        *currLine++ = *initVec++;
      } while (--elements);
    }
    if (*p->ktrig) {
      int j, elements = p->elements, jm1, ruleLen = p->ruleLen;
      MYFLT *actual, *previous, *outVec = p->outVec, *ruleVec = p->ruleVec;
      previous = &(p->currLine[elements * p->NewOld]);
      p->NewOld += 1;
      p->NewOld %= 2;
      actual   = &(p->currLine[elements * p->NewOld]);
      if (*p->iradius == 1) {
        for (j=0; j < elements; j++) {
          jm1 = (j < 1) ? elements-1 : j-1;
          outVec[j] = previous[j];
          actual[j] = ruleVec[ (int) (previous[jm1] + previous[j] +
                                      previous[(j+1) % elements] ) % ruleLen];
        }
      } else {
        int jm2;
        for (j=0; j < elements; j++) {
          jm1 = (j < 1) ? elements-1 : j-1;
          jm2 = (j < 2) ? elements-2 : j-2;
          outVec[j] = previous[j];
          actual[j] = ruleVec[ (int) (previous[jm2] +   previous[jm1] +
                                      previous[j] + previous[(j+1) % elements] +
                                      previous[(j+2) % elements])
                               % ruleLen];
        }
      }

    } else {
      int elements =  p->elements;
      MYFLT *actual = &(p->currLine[elements * !(p->NewOld)]), *outVec = p->outVec;
      do {
        *outVec++ = *actual++ ;
      } while (--elements);
    }
    return OK;
}

#define S(x)    sizeof(x)

OENTRY vectorial_localops[] = {
  { "vtablei", S(MTABLEI),   TR, 1, "",   "iiiim", (SUBR)mtable_i,  NULL },
  { "vtablek", S(MTABLE),    TR, 3, "",   "kkkiz",
                                  (SUBR)mtable_set, (SUBR)mtable_k, NULL },
  { "vtablea", S(MTABLE),    TR, 5, "",   "akkiy",
                                  (SUBR)mtable_set, NULL, (SUBR)mtable_a },
  { "vtablewi", S(MTABLEIW), TB, 1, "",   "iiim", (SUBR)mtablew_i,  NULL },
  { "vtablewk", S(MTABLEW),  TB, 3, "",   "kkiz",
                                (SUBR)mtablew_set, (SUBR)mtablew_k, NULL },
  { "vtablewa", S(MTABLEW),  TB, 5, "",   "akiy",
                                (SUBR)mtablew_set, NULL, (SUBR)mtablew_a },
  { "vtabi", S(MTABI),       TR, 1, "",   "iim", (SUBR)mtab_i,  NULL },
  { "vtabk", S(MTAB),        TR, 3, "",   "kiz",
                                      (SUBR)mtab_set, (SUBR)mtab_k, NULL },
  { "vtaba", S(MTAB),        TR, 5, "",  "aiy",
                                      (SUBR)mtab_set, NULL, (SUBR)mtab_a },
  { "vtabwi", S(MTABIW),     TB, 1, "",  "iim", (SUBR)mtabw_i,  NULL },
  { "vtabwk", S(MTABW),      TB, 3, "",  "kiz",
                                       (SUBR)mtabw_set, (SUBR)mtabw_k, NULL },
  { "vtabwa", S(MTABW),      TB, 5, "",  "aiy",
                                       (SUBR)mtabw_set, NULL, (SUBR)mtabw_a },

  { "vadd",   S(VECTOROP),   TB, 3, "",  "ikkOO",
                                           (SUBR)vectorOp_set, (SUBR) vaddk },
  { "vadd_i", S(VECTOROPI),  TB, 1, "",  "iiio",  (SUBR) vadd_i, NULL, NULL },
  { "vmult",  S(VECTOROP),   TB, 3, "",  "ikkOO",
                                    (SUBR)vectorOp_set, (SUBR) vmultk},
  { "vmult_i", S(VECTOROPI), TB, 1, "",  "iiio", (SUBR) vmult_i, NULL, NULL },
  { "vpow",   S(VECTOROP),   TB, 3, "",  "ikkOO",
                                           (SUBR)vectorOp_set, (SUBR) vpowk },
  { "vpow_i", S(VECTOROPI),  TB, 1, "",  "iiio", (SUBR) vpow_i, NULL, NULL  },
  { "vexp",   S(VECTOROP),   TB, 3, "",  "ikkOO",
                                           (SUBR)vectorOp_set, (SUBR) vexpk },
  { "vexp_i", S(VECTOROPI),  TB, 1, "",  "iiio", (SUBR) vexp_i, NULL, NULL  },
  { "vaddv",  S(VECTORSOP),  TB, 3, "",  "iikOOO",
                                         (SUBR)vectorsOp_set, (SUBR) vaddvk },
  { "vaddv_i",  S(VECTORSOPI), TB, 1, "",  "iiioo", (SUBR)vaddv_i, NULL, NULL },
  { "vsubv",  S(VECTORSOP),  TB, 3, "",  "iikOOO",
                                         (SUBR)vectorsOp_set, (SUBR) vsubvk },
  { "vsubv_i",  S(VECTORSOPI),  TB, 1, "",  "iiioo",
                                           (SUBR)vsubv_i, NULL, NULL        },
  { "vmultv", S(VECTORSOP),  TB, 3, "",  "iikOOO",
                                         (SUBR)vectorsOp_set, (SUBR) vmultvk},
  { "vmultv_i", S(VECTORSOPI),  TB, 1, "",  "iiioo", (SUBR)vmultv_i, NULL, NULL },
  { "vdivv",  S(VECTORSOP), TB,  3, "",  "iikOOO",
                                         (SUBR)vectorsOp_set, (SUBR) vdivvk },
  { "vdivv_i",  S(VECTORSOPI),  TB, 1, "",  "iiioo", (SUBR)vdivv_i, NULL, NULL },
  { "vpowv",  S(VECTORSOP),  TB, 3, "",  "iikOOO",
                                         (SUBR)vectorsOp_set, (SUBR) vpowvk },
  { "vpowv_i",  S(VECTORSOPI),  TB, 1, "",  "iiioo", (SUBR)vpowv_i, NULL, NULL },
  { "vexpv",  S(VECTORSOP),  TB, 3, "",  "iikOOO",
                                         (SUBR)vectorsOp_set, (SUBR) vexpvk },
  { "vexpv_i",  S(VECTORSOPI),  TB, 1, "",  "iiioo", (SUBR)vexpv_i, NULL, NULL },
  { "vcopy",  S(VECTORSOP),  TB, 3, "",  "iikOOO",
                                          (SUBR)vectorsOp_set, (SUBR) vcopy },
  { "vcopy_i", S(VECTORSOP), TB, 1, "",  "iiioo", (SUBR) vcopy_i, NULL, NULL},
  { "vmap",   S(VECTORSOPI), TB, 1, "",  "iiioo", (SUBR)vmap_i, NULL, NULL  },
  { "vlimit", S(VLIMIT),  TR, 3, "",  "ikki",(SUBR)vlimit_set, (SUBR)vlimit },
  { "vwrap",  S(VLIMIT),  TB, 3, "",  "ikki",(SUBR)vlimit_set, (SUBR) vwrap },
  { "vmirror", S(VLIMIT),  0,   3, "",  "ikki",(SUBR)vlimit_set, (SUBR)vmirror },
  { "vlinseg", S(VSEG),   TB, 3, "",  "iin", (SUBR)vseg_set,   (SUBR)vlinseg },
  { "vexpseg", S(VSEG),    0,   3, "",  "iin", (SUBR)vseg_set, (SUBR)vexpseg },
  { "vrandh", S(VRANDH),  TB, 3, "",  "ikkiovoo",(SUBR)vrandh_set, (SUBR)vrandh},
  { "vrandi", S(VRANDI),  TB, 3, "",  "ikkiovoo",(SUBR)vrandi_set, (SUBR)vrandi },
  { "vport",  S(VPORT),   TB, 3, "",  "ikio",(SUBR)vport_set,  (SUBR)vport   },
  { "vecdelay", S(VECDEL),0,    3, "",  "iiiiio",(SUBR)vecdly_set, (SUBR)vecdly },
  { "vdelayk", S(KDEL),    0,   3, "k", "kkioo",(SUBR)kdel_set,  (SUBR)kdelay },
  { "vcella", S(CELLA),      TR, 3, "",  "kkiiiiip",(SUBR)ca_set, (SUBR)ca    }
};

int gab_vectorial_init_(CSOUND *csound)
{
    return
      csound->AppendOpcodes(csound, &(vectorial_localops[0]),
                            (int) (sizeof(vectorial_localops) / sizeof(OENTRY)));
}
