 /*
    sndwarp.c:

    Copyright (C) 1997 Richard Karpen, John ffitch

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/**************************************************************/
/*************sndwarp******************************************/
/*** By Richard Karpen - 1992, 1995, 1997 *********************/
/**************************************************************/
/*This is a version that uses table lookup instead of reading */
/*from soundfiles.                                            */
/**************************************************************/

#include "stdopcod.h"
#include "sndwarp.h"

#define unirand(x) ((cs_float) (x->Rand31((x->RandSeed31(x))) - 1) / FL(2147483645.0))

/* Check before converting counts or allocating sections. Both endpoints of
   the random window range must fit the counter and allow a nonzero divisor. */
static int32_t sndwarpcheck(CSOUND *csound, cs_float overlap,
                            cs_float wsize, cs_float randw)
{
    if (UNLIKELY(!(overlap >= FL(1.0) &&
                   (cs_double)overlap < (INT32_MAX + 0.0) + 1.0) ||
                 overlap != (cs_float)(int32_t)overlap))
      return csound->InitError(csound, "%s", Str("sndwarp: ioverlap must be a "
                                               "positive integer"));
    if (UNLIKELY((size_t)(int32_t)overlap > SIZE_MAX / sizeof(WARPSECTION)))
      return csound->InitError(csound, "%s", Str("sndwarp: too many overlaps"));
    if (UNLIKELY(!(wsize >= FL(2.0) && randw >= FL(0.0) &&
                   wsize + randw < FL(2147483648.0))))
      return csound->InitError(csound, "%s", Str("sndwarp: window size must be "
                        "at least 2 and iwsize + irandw must be below 2^31; "
                        "irandw must be nonnegative"));
    return OK;
}

static int32_t sndwarpgetset(CSOUND *csound, SNDWARP *p)
{
    int32_t         i;
    int32_t         nsections;
    FUNC        *ftpWind, *ftpSamp;
    WARPSECTION *exp;
    char        *auxp;
    cs_float       iwsize;

    if (UNLIKELY(sndwarpcheck(csound, *p->ioverlap, *p->iwsize,
                              *p->irandw) != OK))
      return NOTOK;
    nsections = (int32_t)*p->ioverlap;
    if ((auxp = p->auxch.auxp) == NULL || nsections != p->nsections) {
      csound->AuxAlloc(csound, (size_t)nsections*sizeof(WARPSECTION), &p->auxch);
      auxp = p->auxch.auxp;
      p->nsections = nsections;
    }
    p->exp = (WARPSECTION *)auxp;

    if (UNLIKELY((ftpSamp = csound->FTFind(csound, p->isampfun)) == NULL))
      return NOTOK;
    p->ftpSamp  = ftpSamp;
    p->sampflen = ftpSamp->flen;

    if (UNLIKELY((ftpWind = csound->FTFind(csound, p->ifn)) == NULL))
      return NOTOK;
    p->ftpWind = ftpWind;
    p->flen    = ftpWind->flen;

    p->maxFr   = -1 + ftpSamp->flen;
    p->prFlg   = 1;    /* true */
    p->begin   = (int32_t)(*p->ibegin * CS_ESR);

    exp        = p->exp;
    iwsize = *p->iwsize;
    for (i=0; i< nsections; i++) {
      if (i==0) {
        exp[i].wsize = (int32_t)iwsize;
        exp[i].cnt = 0;
        exp[i].ampphs = FL(0.0);
      }
      else {
        exp[i].wsize = (int32_t) (iwsize + (unirand(csound) * (*p->irandw)));
        exp[i].cnt=(int32_t)(exp[i].wsize*((cs_float)i/nsections));
        exp[i].ampphs = p->flen*((cs_float)i/nsections);
      }
      exp[i].offset = (cs_float)p->begin;
      exp[i].ampincr = (cs_float)p->flen/(exp[i].wsize-1);
      /* exp[i].section = i+1;  *//* section number just used for debugging! */

    }
    p->ampcode = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    p->timewarpcode = IS_ASIG_ARG(p->xtimewarp) ? 1 : 0;
    p->resamplecode = IS_ASIG_ARG(p->xresample) ? 1 : 0;
    return OK;
}

static int32_t sndwarp(CSOUND *csound, SNDWARP *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float       frm_0,frm_1;
    int32       base, longphase;
    cs_float       frac, frIndx;
    cs_float       *r1, *r2, *amp, *timewarpby, *resample;
    WARPSECTION *exp;
    FUNC        *ftpWind, *ftpSamp;
    int32_t         i;
    cs_float       v1, v2, windowamp, fract;
    cs_float       flen = (cs_float)p->flen;
    cs_float       iwsize = *p->iwsize;
    int32_t         overlap = p->nsections;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
    r1 = p->r1;
    r2 = p->r2;
    memset(r1, 0, nsmps*sizeof(cs_float));
    if (p->OUTOCOUNT >1) memset(r2, 0, nsmps*sizeof(cs_float));
    exp = p->exp;
    ftpWind = p->ftpWind;
    ftpSamp = p->ftpSamp;

    if (UNLIKELY(early)) nsmps -= early;
    for (i=0; i<overlap; i++) {
      resample = p->xresample + (p->resamplecode ? offset : 0);
      timewarpby = p->xtimewarp + (p->timewarpcode ? offset : 0);
      amp = p->xamp + (p->ampcode ? offset : 0);

      for (n=offset; n<nsmps;n++) {
        if (exp[i].cnt < exp[i].wsize) goto skipover;

        if (*p->itimemode!=0)
          exp[i].offset=(CS_ESR * *timewarpby)+p->begin;
        /* A zero time scale repeats the window at its current offset. */
        else if (*timewarpby != FL(0.0))
          exp[i].offset += (cs_float)exp[i].wsize/(*timewarpby);

        exp[i].cnt=0;
        exp[i].wsize = (int32_t) (iwsize + (unirand(csound) * (*p->irandw)));
        exp[i].ampphs = FL(0.0);
        exp[i].ampincr = flen/(exp[i].wsize-1);

      skipover:

        frIndx =(cs_float)((exp[i].cnt * *resample)  + exp[i].offset);
        exp[i].cnt += 1;
        /* Hold the first frame when reading before the start of the table. */
        if (frIndx < FL(0.0)) frIndx = FL(0.0);
        if (frIndx > (cs_float)p->maxFr) { /* not past last one */
          frIndx = (cs_float)p->maxFr;
          if (p->prFlg) {
            p->prFlg = 0;   /* false */
            csound->Warning(csound, "%s", Str("SNDWARP at last sample frame"));
          }
        }
        longphase = (int32)exp[i].ampphs;
        if (longphase > p->flen-1) longphase = p->flen-1;
        v1 = *(ftpWind->ftable + longphase);
        v2 = *(ftpWind->ftable + longphase + 1);
        fract = (cs_float)(exp[i].ampphs - (int32)exp[i].ampphs);
        windowamp = v1 + (v2 - v1)*fract;
        exp[i].ampphs += exp[i].ampincr;

        base = (int32)frIndx;    /* index of basis frame of interpolation */
        frac = ((cs_float)(frIndx - (cs_float)base));
        frm_0 = *(ftpSamp->ftable + base);
        frm_1 = *(ftpSamp->ftable + (base+1));
        if (frac != FL(0.0)) {
          r1[n] += ((frm_0 + frac*(frm_1-frm_0)) * windowamp) * *amp;
          if (i==0)
           if (p->OUTOCOUNT > 1)
             r2[n] += (frm_0 + frac*(frm_1-frm_0)) * *amp;
        }
        else {
          r1[n] += (frm_0 * windowamp) * *amp;
          if (i==0)
            if (p->OUTOCOUNT > 1)
              r2[n] += frm_0 * *amp;
        }
        if (p->ampcode) amp++;
        if (p->timewarpcode) timewarpby++;
        if (p->resamplecode) resample++;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("sndwarp: not initialised"));
}

/****************************************************************/
/**************STEREO VERSION OF SNDWARP*************************/
/****************************************************************/

static int32_t sndwarpstgetset(CSOUND *csound, SNDWARPST *p)
{
    int32_t         i;
    int32_t         nsections;
    FUNC        *ftpWind, *ftpSamp;
    WARPSECTION *exp;
    char        *auxp;
    cs_float       iwsize;

    if (UNLIKELY(p->OUTOCOUNT != 2 && p->OUTOCOUNT != 4)) {
      return csound->InitError(csound, "%s", Str("Wrong number of outputs "
                                           "in sndwarpst; must be 2 or 4"));
    }
    if (UNLIKELY(sndwarpcheck(csound, *p->ioverlap, *p->iwsize,
                              *p->irandw) != OK))
      return NOTOK;
    nsections = (int32_t)*p->ioverlap;
    if ((auxp = p->auxch.auxp) == NULL || nsections != p->nsections) {
      csound->AuxAlloc(csound, (size_t)nsections*sizeof(WARPSECTION), &p->auxch);
      auxp = p->auxch.auxp;
      p->nsections = nsections;
    }
    p->exp = (WARPSECTION *)auxp;

    if (UNLIKELY((ftpSamp = csound->FTFind(csound, p->isampfun)) == NULL))
      return NOTOK;
    p->ftpSamp = ftpSamp;
    p->sampflen=ftpSamp->flen;

    if (UNLIKELY((ftpWind = csound->FTFind(csound, p->ifn)) == NULL))
      return NOTOK;
    p->ftpWind = ftpWind;
    p->flen=ftpWind->flen;

    p->maxFr  = -1L + (int32)(ftpSamp->flen*FL(0.5));
    p->prFlg = 1;    /* true */
    p->begin = (int32_t)(*p->ibegin * CS_ESR);
    iwsize = *p->iwsize;
    exp = p->exp;
    for (i=0; i< nsections; i++) {
      if (i==0) {
        exp[i].wsize = (int32_t)iwsize;
        exp[i].cnt=0;
        exp[i].ampphs = FL(0.0);
      }
      else {
        exp[i].wsize = (int32_t) (iwsize + (unirand(csound) * (*p->irandw)));
        exp[i].cnt=(int32_t)(exp[i].wsize*((cs_float)i/nsections));
        exp[i].ampphs = p->flen*((cs_float)i/nsections);
      }
      exp[i].offset = (cs_float)p->begin;
      exp[i].ampincr = (cs_float)p->flen/(exp[i].wsize-1);
      /* exp[i].section = i+1;  *//* section number just used for debugging! */

    }
    p->ampcode = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    p->timewarpcode = IS_ASIG_ARG(p->xtimewarp) ? 1 : 0;
    p->resamplecode = IS_ASIG_ARG(p->xresample) ? 1 : 0;
    return OK;
}

static int32_t sndwarpstset(CSOUND *csound, SNDWARPST *p)
{
    return sndwarpstgetset(csound,p);
}

static int32_t sndwarpst(CSOUND *csound, SNDWARPST *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float       frm10,frm11, frm20, frm21;
    int32        base, longphase;
    cs_float       frac, frIndx;
    cs_float       *r1, *r2,*r3, *r4, *amp, *timewarpby, *resample;
    WARPSECTION *exp;
    FUNC        *ftpWind, *ftpSamp;
    int32_t         i;
    cs_float       v1, v2, windowamp, fract;
    cs_float       flen = (cs_float)p->flen;
    cs_float       iwsize = *p->iwsize;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;  /* RWD fix */
    r1 = p->r1;
    r2 = p->r2;
    r3 = p->r3;
    r4 = p->r4;
    memset(r1, 0,nsmps*sizeof(cs_float));
    memset(r2, 0,nsmps*sizeof(cs_float));
    if (p->OUTOCOUNT >2) {
      memset(r3, 0,nsmps*sizeof(cs_float));
      memset(r4, 0,nsmps*sizeof(cs_float));
    }
    exp = p->exp;
    ftpWind = p->ftpWind;
    ftpSamp = p->ftpSamp;
    if (UNLIKELY(early)) nsmps -= early;
    for (i=0; i<p->nsections; i++) {
      resample = p->xresample + (p->resamplecode ? offset : 0);
      timewarpby = p->xtimewarp + (p->timewarpcode ? offset : 0);
      amp = p->xamp + (p->ampcode ? offset : 0);

      for (n=offset; n<nsmps;n++) {
        if (exp[i].cnt < exp[i].wsize) goto skipover;

        if (*p->itimemode!=0)
          exp[i].offset=(CS_ESR * *timewarpby)+p->begin;
        /* A zero time scale repeats the window at its current offset. */
        else if (*timewarpby != FL(0.0))
          exp[i].offset += (cs_float)exp[i].wsize/(*timewarpby);

        exp[i].cnt=0;
        exp[i].wsize = (int32_t) (iwsize + (unirand(csound) * (*p->irandw)));
        exp[i].ampphs = FL(0.0);
        exp[i].ampincr = flen/(exp[i].wsize-1);

      skipover:
        frIndx =(cs_float)(exp[i].cnt * *resample)  + (cs_float)exp[i].offset;
        exp[i].cnt += 1;
        /* Hold the first frame when reading before the start of the table. */
        if (frIndx < FL(0.0)) frIndx = FL(0.0);
        if (frIndx > (cs_float)p->maxFr) {  /* not past last one */
          frIndx = (cs_float)p->maxFr;
          if (p->prFlg) {
            p->prFlg = 0;   /* false */
            csound->Warning(csound, "%s", Str("SNDWARP at last sample frame"));
          }
        }
        longphase = (int32)exp[i].ampphs;
        if (longphase > p->flen-1) longphase = p->flen-1;
        v1 = *(ftpWind->ftable + longphase);
        v2 = *(ftpWind->ftable + longphase + 1);
        fract = (cs_float)(exp[i].ampphs - (int32)exp[i].ampphs);
        windowamp = v1 + (v2 - v1)*fract;
        exp[i].ampphs += exp[i].ampincr;

        base = (int32)frIndx;    /* index of basis frame of interpolation */
        frac = ((cs_float)(frIndx - (cs_float)base));

        frm10 = *(ftpSamp->ftable + (base * 2));
        frm11 = *(ftpSamp->ftable + ((base+1)*2));
        frm20 = *(ftpSamp->ftable + (base*2)+1);
        if (((base+1)*2)+1 < (int)ftpSamp->flen + 1)
          frm21 = *(ftpSamp->ftable + (((base+1)*2)+1));
        else
          frm21 = FL(0.0);
        if (frac != FL(0.0)) {
          r1[n] += ((frm10 + frac*(frm11-frm10)) * windowamp) * *amp;
          r2[n] += ((frm20 + frac*(frm21-frm20)) * windowamp) * *amp;
          if (i==0)
            if (p->OUTOCOUNT > 2) {
              r3[n] += (frm10 + frac*(frm11-frm10)) * *amp;
              r4[n] += (frm20 + frac*(frm21-frm20)) * *amp;
            }
        }
        else {
          r1[n] += (frm10 * windowamp) * *amp;
          r2[n] += (frm20 * windowamp) * *amp;
          if (i==0)
            if (p->OUTOCOUNT > 2) {
              r3[n] += frm10 * *amp;
              r4[n] += frm20 * *amp;
            }
        }
        if (p->ampcode) amp++;
        if (p->timewarpcode) timewarpby++;
        if (p->resamplecode) resample++;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("sndwarpst: not initialised"));
}

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
   { "sndwarp", S(SNDWARP), TR,  "mm", "xxxiiiiiii",
    (SUBR)sndwarpgetset, (SUBR)sndwarp},
   { "sndwarpst", S(SNDWARPST), TR,  "mmmm","xxxiiiiiii",
    (SUBR)sndwarpstset,(SUBR)sndwarpst}
};

int32_t sndwarp_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}

