/*
    crossfm.c:

    Copyright (C) 2005 Francois Pinot

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include "crossfm.h"
#include <math.h>

int32_t xfmset(CSOUND *csound, CROSSFM *p)
{
    FUNC *ftp1 = csound->FTFind(csound, p->ifn1);
    FUNC *ftp2 = csound->FTFind(csound, p->ifn2);
    if (UNLIKELY(ftp1 == NULL  ||  ftp2 == NULL)) {
      return csound->InitError(csound, "%s", Str("crossfm: ftable not found"));
    }
    p->siz1 = (MYFLT)ftp1->flen;
    p->siz2 = (MYFLT)ftp2->flen;
    p->ftp1 = ftp1;
    p->ftp2 = ftp2;
    if (*p->iphs1 >= FL(0.0)) {
      p->phase1 = *p->iphs1;
      p->sig1 = FL(0.0);
    }
    if (*p->iphs2 >= FL(0.0)) {
      p->phase2 = *p->iphs2;
      p->sig2 = FL(0.0);
    }
    p->frq1adv = IS_ASIG_ARG(p->xfrq1) ? 1 : 0;
    p->frq2adv = IS_ASIG_ARG(p->xfrq2) ? 1 : 0;
    p->ndx1adv = IS_ASIG_ARG(p->xndx1) ? 1 : 0;
    p->ndx2adv = IS_ASIG_ARG(p->xndx2) ? 1 : 0;
    return OK;
}

int32_t xfm(CSOUND *csound, CROSSFM *p)
{
    MYFLT *out1, *out2;
    MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;
    MYFLT k, cps;
    MYFLT frq1, frq2, si1, si2;
    MYFLT siz1, siz2;
    MYFLT *tbl1, *tbl2;
    MYFLT phase1, phase2;
    MYFLT sig1, sig2;
    int32_t n1, n2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    out1 = p->aout1;
    out2 = p->aout2;
    xfrq1 = p->xfrq1;
    xfrq2 = p->xfrq2;
    xndx1 = p->xndx1;
    xndx2 = p->xndx2;
    siz1 = p->siz1;
    siz2 = p->siz2;
    tbl1 = p->ftp1->ftable;
    tbl2 = p->ftp2->ftable;
    cps = *p->kcps;
    k = CS_ONEDSR;

    phase1 = p->phase1;
    phase2 = p->phase2;
    sig1 = p->sig1;
    sig2 = p->sig2;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      frq1 = *xfrq1 * cps;
      frq2 = *xfrq2 * cps;
      si1 = (frq1 + *xndx2 * frq2 * sig2) * k;
      si2 = (frq2 + *xndx1 * frq1 * sig1) * k;
      out1[i] = sig1;
      out2[i] = sig2;
      phase1 += si1;
      phase1 -= FLOOR(phase1);
      phase2 += si2;
      phase2 -= FLOOR(phase2);
      n1 = (int32_t)(phase1 * siz1);
      n2 = (int32_t)(phase2 * siz2);
      sig1 = tbl1[n1];
      sig2 = tbl2[n2];
      xfrq1 += p->frq1adv;
      xfrq2 += p->frq2adv;
      xndx1 += p->ndx1adv;
      xndx2 += p->ndx2adv;
    }

    p->phase1 = phase1;
    p->phase2 = phase2;
    p->sig1 = sig1;
    p->sig2 = sig2;
    return OK;
}

int32_t xfmi(CSOUND *csound, CROSSFM *p)
{
    MYFLT *out1, *out2;
    MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;
    MYFLT k, cps;
    MYFLT frq1, frq2, si1, si2;
    MYFLT siz1, siz2;
    MYFLT *tbl1, *tbl2;
    MYFLT phase1, phase2;
    MYFLT sig1, sig2;
    MYFLT x, y1, y2;
    int32_t n1, n2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    out1 = p->aout1;
    out2 = p->aout2;
    xfrq1 = p->xfrq1;
    xfrq2 = p->xfrq2;
    xndx1 = p->xndx1;
    xndx2 = p->xndx2;
    siz1 = p->siz1;
    siz2 = p->siz2;
    tbl1 = p->ftp1->ftable;
    tbl2 = p->ftp2->ftable;
    cps = *p->kcps;
    k = CS_ONEDSR;

    phase1 = p->phase1;
    phase2 = p->phase2;
    sig1 = p->sig1;
    sig2 = p->sig2;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      frq1 = *xfrq1 * cps;
      frq2 = *xfrq2 * cps;
      si1 = (frq1 + *xndx2 * frq2 * sig2) * k;
      si2 = (frq2 + *xndx1 * frq1 * sig1) * k;
      out1[i] = sig1;
      out2[i] = sig2;
      phase1 += si1;
      phase1 -= FLOOR(phase1);
      phase2 += si2;
      phase2 -= FLOOR(phase2);
      x = phase1 * siz1;
      n1 = (int32_t)x;
      y1 = tbl1[n1];
      sig1 = (tbl1[n1+1]-y1) * (x - FLOOR(x)) + y1;
      x = phase2 * siz2;
      n2 = (int32_t)x;
      y2 = tbl2[n2];
      sig2 = (tbl2[n2+1]-y2) * (x - FLOOR(x)) + y2;
      xfrq1 += p->frq1adv;
      xfrq2 += p->frq2adv;
      xndx1 += p->ndx1adv;
      xndx2 += p->ndx2adv;
    }

    p->phase1 = phase1;
    p->phase2 = phase2;
    p->sig1 = sig1;
    p->sig2 = sig2;
    return OK;
}

int32_t xpm(CSOUND *csound, CROSSFM *p)
{
    MYFLT *out1, *out2;
    MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;
    MYFLT k, cps;
    MYFLT frq1, frq2, si1, si2;
    MYFLT siz1, siz2;
    MYFLT *tbl1, *tbl2;
    MYFLT phase1, phase2;
    MYFLT sig1, sig2;
    int32_t n1, n2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    out1 = p->aout1;
    out2 = p->aout2;
    xfrq1 = p->xfrq1;
    xfrq2 = p->xfrq2;
    xndx1 = p->xndx1;
    xndx2 = p->xndx2;
    siz1 = p->siz1;
    siz2 = p->siz2;
    tbl1 = p->ftp1->ftable;
    tbl2 = p->ftp2->ftable;
    cps = *p->kcps;
    k = CS_ONEDSR;

    phase1 = p->phase1;
    phase2 = p->phase2;
    sig1 = p->sig1;
    sig2 = p->sig2;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      frq1 = *xfrq1 * cps;
      frq2 = *xfrq2 * cps;
      out1[i] = sig1;
      out2[i] = sig2;
      phase1 += (frq1 * k);
      si1 = phase1 + *xndx2 * sig2 / TWOPI_F;
      si1 -= FLOOR(si1);
      phase2 += (frq2 * k);
      si2 = phase2 + *xndx1 * sig1 / TWOPI_F;
      si2 -= FLOOR(si2);
      n1 = (int32_t)(si1 * siz1);
      n2 = (int32_t)(si2 * siz2);
      sig1 = tbl1[n1];
      sig2 = tbl2[n2];
      xfrq1 += p->frq1adv;
      xfrq2 += p->frq2adv;
      xndx1 += p->ndx1adv;
      xndx2 += p->ndx2adv;
    }

    p->phase1 = phase1 - FLOOR(phase1);
    p->phase2 = phase2 - FLOOR(phase2);
    p->sig1 = sig1;
    p->sig2 = sig2;
    return OK;
}

int32_t xpmi(CSOUND *csound, CROSSFM *p)
{
    MYFLT *out1, *out2;
    MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;
    MYFLT k, cps;
    MYFLT frq1, frq2, si1, si2;
    MYFLT siz1, siz2;
    MYFLT *tbl1, *tbl2;
    MYFLT phase1, phase2;
    MYFLT sig1, sig2;
    MYFLT x, y1, y2;
    int32_t n1, n2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    out1 = p->aout1;
    out2 = p->aout2;
    xfrq1 = p->xfrq1;
    xfrq2 = p->xfrq2;
    xndx1 = p->xndx1;
    xndx2 = p->xndx2;
    siz1 = p->siz1;
    siz2 = p->siz2;
    tbl1 = p->ftp1->ftable;
    tbl2 = p->ftp2->ftable;
    cps = *p->kcps;
    k = CS_ONEDSR;

    phase1 = p->phase1;
    phase2 = p->phase2;
    sig1 = p->sig1;
    sig2 = p->sig2;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      frq1 = *xfrq1 * cps;
      frq2 = *xfrq2 * cps;
      out1[i] = sig1;
      out2[i] = sig2;
      phase1 += (frq1 * k);
      si1 = phase1 + *xndx2 * sig2 / TWOPI_F;
      si1 -= FLOOR(si1);
      phase2 += (frq2 * k);
      si2 = phase2 + *xndx1 * sig1 / TWOPI_F;
      si2 -= FLOOR(si2);
      x = si1 * siz1;
      n1 = (int32_t)x;
      y1 = tbl1[n1];
      sig1 = (tbl1[n1+1]-y1) * (x - FLOOR(x)) + y1;
      x = si2 * siz2;
      n2 = (int32_t)x;
      y2 = tbl2[n2];
      sig2 = (tbl2[n2+1]-y2) * (x - FLOOR(x)) + y2;
      xfrq1 += p->frq1adv;
      xfrq2 += p->frq2adv;
      xndx1 += p->ndx1adv;
      xndx2 += p->ndx2adv;
  }

    p->phase1 = phase1 - FLOOR(phase1);
    p->phase2 = phase2 - FLOOR(phase2);
    p->sig1 = sig1;
    p->sig2 = sig2;
    return OK;
}

int32_t xfmpm(CSOUND *csound, CROSSFM *p)
{
    MYFLT *out1, *out2;
    MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;
    MYFLT k, cps;
    MYFLT frq1, frq2, si1, si2;
    MYFLT siz1, siz2;
    MYFLT *tbl1, *tbl2;
    MYFLT phase1, phase2;
    MYFLT sig1, sig2;
    int32_t n1, n2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    out1 = p->aout1;
    out2 = p->aout2;
    xfrq1 = p->xfrq1;
    xfrq2 = p->xfrq2;
    xndx1 = p->xndx1;
    xndx2 = p->xndx2;
    siz1 = p->siz1;
    siz2 = p->siz2;
    tbl1 = p->ftp1->ftable;
    tbl2 = p->ftp2->ftable;
    cps = *p->kcps;
    k = CS_ONEDSR;

    phase1 = p->phase1;
    phase2 = p->phase2;
    sig1 = p->sig1;
    sig2 = p->sig2;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      frq1 = *xfrq1 * cps;
      frq2 = *xfrq2 * cps;
      out1[i] = sig1;
      out2[i] = sig2;
      si1 = (frq1 + *xndx2 * frq2 * sig2) * k;
      phase1 += si1;
      phase1 -= FLOOR(phase1);
      phase2 += (frq2 * k);
      si2 = phase2 + *xndx1 * sig1 / TWOPI_F;
      si2 -= FLOOR(si2);
      n1 = (int32_t)(phase1 * siz1);
      n2 = (int32_t)(si2 * siz2);
      sig1 = tbl1[n1];
      sig2 = tbl2[n2];
      xfrq1 += p->frq1adv;
      xfrq2 += p->frq2adv;
      xndx1 += p->ndx1adv;
      xndx2 += p->ndx2adv;
    }

    p->phase1 = phase1;
    p->phase2 = phase2 - FLOOR(phase2);
    p->sig1 = sig1;
    p->sig2 = sig2;
    return OK;
}

int32_t xfmpmi(CSOUND *csound, CROSSFM *p)
{
    MYFLT *out1, *out2;
    MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;
    MYFLT k, cps;
    MYFLT frq1, frq2, si1, si2;
    MYFLT siz1, siz2;
    MYFLT *tbl1, *tbl2;
    MYFLT phase1, phase2;
    MYFLT sig1, sig2;
    MYFLT x, y1, y2;
    int32_t n1, n2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    out1 = p->aout1;
    out2 = p->aout2;
    xfrq1 = p->xfrq1;
    xfrq2 = p->xfrq2;
    xndx1 = p->xndx1;
    xndx2 = p->xndx2;
    siz1 = p->siz1;
    siz2 = p->siz2;
    tbl1 = p->ftp1->ftable;
    tbl2 = p->ftp2->ftable;
    cps = *p->kcps;
    k = CS_ONEDSR;

    phase1 = p->phase1;
    phase2 = p->phase2;
    sig1 = p->sig1;
    sig2 = p->sig2;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      frq1 = *xfrq1 * cps;
      frq2 = *xfrq2 * cps;
      out1[i] = sig1;
      out2[i] = sig2;
      si1 = (frq1 + *xndx2 * frq2 * sig2) * k;
      phase1 += si1;
      phase1 -= FLOOR(phase1);
      phase2 += (frq2 * k);
      si2 = phase2 + *xndx1 * sig1 / TWOPI_F;
      si2 -= FLOOR(si2);
      x = phase1 * siz1;
      n1 = (int32_t)x;
      y1 = tbl1[n1];
      sig1 = (tbl1[n1+1]-y1) * (x - FLOOR(x)) + y1;
      x = si2 * siz2;
      n2 = (int32_t
            )x;
      y2 = tbl2[n2];
      sig2 = (tbl2[n2+1]-y2) * (x - FLOOR(x)) + y2;
      xfrq1 += p->frq1adv;
      xfrq2 += p->frq2adv;
      xndx1 += p->ndx1adv;
      xndx2 += p->ndx2adv;
    }

    p->phase1 = phase1;
    p->phase2 = phase2 - FLOOR(phase2);
    p->sig1 = sig1;
    p->sig2 = sig2;
    return OK;
}

#define S sizeof

static OENTRY crossfm_localops[] = {
  { "crossfm", S(CROSSFM), TR,  "aa", "xxxxkiioo", (SUBR)xfmset, (SUBR)xfm },
  { "crossfmi", S(CROSSFM), TR,  "aa", "xxxxkiioo",(SUBR)xfmset, (SUBR)xfmi },
  { "crosspm", S(CROSSFM), TR,  "aa", "xxxxkiioo", (SUBR)xfmset, (SUBR)xpm },
  { "crosspmi", S(CROSSFM), TR,  "aa", "xxxxkiioo",(SUBR)xfmset, (SUBR)xpmi },
  { "crossfmpm", S(CROSSFM), TR,  "aa", "xxxxkiioo",(SUBR)xfmset,(SUBR)xfmpm},
  { "crossfmpmi", S(CROSSFM),TR,  "aa", "xxxxkiioo",(SUBR)xfmset, (SUBR)xfmpmi },
};

LINKAGE_BUILTIN(crossfm_localops)


