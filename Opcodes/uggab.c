/*
    uggab.c:

    Copyright (C) 1998 Gabriel Maldonado, John ffitch

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

/********************************************/
/* wrap and mirror UGs by Gabriel Maldonado */
/* and others by same author                */
/* Code adapted by JPff 1998 Sep 19         */
/********************************************/

#include "stdopcod.h"
#include "uggab.h"
#include <math.h>
#include <float.h>

#ifdef USE_FLOAT
#define WRAP_MAX FLT_MAX
#else
#define WRAP_MAX DBL_MAX
#endif

/* Bounds, width, and lower_remainder are fixed for the current block.
   Arguments must be local values without side effects. */
#define WRAP_VALUE(output, input, lower, upper, width, lower_remainder) do { \
    cs_double value = (input);                                               \
    if (value >= (lower) && value < (upper)) {                             \
      (output) = (cs_float)value;                                            \
      break;                                                             \
    }                                                                    \
    if (UNLIKELY((width) > WRAP_MAX)) {                                    \
      /* A finite interval this wide needs at most one wrap. */           \
      if (!(value >= -WRAP_MAX && value <= WRAP_MAX &&                       \
            (lower) >= -WRAP_MAX && (upper) <= WRAP_MAX))                    \
        value = NAN;                                                     \
      else {                                                             \
        /* Keep fast-math from rewriting this as value +/- width. */      \
        int above = value >= (upper);                                    \
        volatile cs_double distance = above ? value - (upper)               \
                                         : (lower) - value;              \
        value = above ? (lower) + distance : (upper) - distance;          \
      }                                                                  \
    } else {                                                             \
      /* Reduce separately so input - lower cannot lose the offset or    \
         overflow. Each remainder is at most half the width. */          \
      value = remainder(value, (width)) - (lower_remainder);              \
      if (value < 0.0)                                                    \
        value += (width);                                                \
      value += (lower);                                                  \
    }                                                                    \
    (output) = (cs_float)value;                                              \
    /* Rounding can reach the excluded upper endpoint. */                \
    if ((output) >= (upper))                                              \
      (output) = (cs_float)(lower);                                         \
} while (0)

static int32_t wrap(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    cs_float *adest = p->xdest;
    cs_float *asig = p->xsig;
    cs_double low = *p->xlow, high = *p->xhigh;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(adest, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&adest[nsmps], '\0', early*sizeof(cs_float));
    }
    if (low >= high) {
      cs_double sum = low + high;
      cs_float average;
      if (fabs(sum) <= WRAP_MAX)
        average = (cs_float)(sum * 0.5);
      else {
        /* Prevent fast-math from adding the bounds before halving. */
        volatile cs_double half_low = low * 0.5;
        average = (cs_float)(half_low + high * 0.5);
      }
      for (n=offset; n<nsmps; n++)
        adest[n] = average;
    } else {
      cs_double width = high - low;
      cs_double lower_remainder = remainder(low, width);
      for (n=offset; n<nsmps; n++)
        WRAP_VALUE(adest[n], asig[n], low, high, width, lower_remainder);
    }
    return OK;
}

static int32_t kwrap(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    cs_double low = *p->xlow, high = *p->xhigh;

    if (low >= high) {
      cs_double sum = low + high;
      if (fabs(sum) <= WRAP_MAX)
        *p->xdest = (cs_float)(sum * 0.5);
      else {
        volatile cs_double half_low = low * 0.5;
        *p->xdest = (cs_float)(half_low + high * 0.5);
      }
    } else {
      cs_double width = high - low;
      cs_double lower_remainder = remainder(low, width);
      WRAP_VALUE(*p->xdest, *p->xsig, low, high, width, lower_remainder);
    }
    return OK;
}

#undef WRAP_VALUE
#undef WRAP_MAX

/*---------------------------------------------------------------------*/

#define MIRROR_VALUE(output, input, lower, upper) do {                         \
    cs_double mirror_input = (input), low = (lower), high = (upper);              \
    cs_double width, remainder, lowrem;                                           \
    int quotient, lowquot, odd;                                                \
                                                                               \
    if (low >= high) {                                                         \
      cs_double sum = low + high;                                                 \
      (output) = (cs_float)(isfinite(sum) ? sum * 0.5                             \
                                     : low * 0.5 + high * 0.5);                \
      break;                                                                   \
    }                                                                          \
    if (mirror_input >= low && mirror_input <= high) {                         \
      (output) = (cs_float)mirror_input;                                          \
      break;                                                                   \
    }                                                                          \
    if (!isfinite(mirror_input) || !isfinite(low) || !isfinite(high)) {        \
      (output) = (cs_float)NAN;                                                   \
      break;                                                                   \
    }                                                                          \
                                                                               \
    width = high - low;                                                        \
    if (!isfinite(width)) {                                                    \
      /* Such a wide finite interval needs at most one reflection. */          \
      (output) = (cs_float)(mirror_input > high                                   \
                         ? high - (mirror_input - high)                        \
                         : low + (low - mirror_input));                        \
      break;                                                                   \
    }                                                                          \
                                                                               \
    /* Reduce separately: input - low can overflow or lose the low offset. */  \
    remainder = remquo(mirror_input, width, &quotient);                        \
    lowrem = remquo(low, width, &lowquot);                                     \
    remainder -= lowrem;                                                       \
    odd = (quotient % 2 != 0) != (lowquot % 2 != 0);                           \
    if (remainder < 0.0) {                                                     \
      remainder += width;                                                      \
      odd = !odd;                                                              \
    }                                                                          \
    /* Quotient parity selects the direction without doubling the width. */    \
    (output) = (cs_float)(odd ? high - remainder : low + remainder);              \
} while (0)

static int32_t kmirror(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    MIRROR_VALUE(*p->xdest, *p->xsig, *p->xlow, *p->xhigh);
    return OK;
}

static int32_t mirror(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    cs_float       *adest, *asig;
    cs_float       xlow, xhigh, xaverage;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;

    adest = p->xdest;
    asig  = p->xsig;
    xlow = *p->xlow;
    xhigh = *p->xhigh;

    if (UNLIKELY(offset)) memset(adest, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&adest[nsmps], '\0', early*sizeof(cs_float));
    }
    if (xlow >= xhigh)  {
      MIRROR_VALUE(xaverage, FL(0.0), xlow, xhigh);
      for (n=offset;n<nsmps;n++) {
        adest[n] = xaverage;
      }
      return OK;                   /* Suggested by Istvan Varga */
    }

    for (n=offset;n<nsmps;n++) {
      MIRROR_VALUE(adest[n], asig[n], xlow, xhigh);
    }
    return OK;
}

static int32_t trig_set(CSOUND *csound, TRIG *p)
{
    IGN(csound);
    p->old_sig = FL(0.0);
    return OK;
}

static int32_t trig(CSOUND *csound, TRIG *p)
{
    cs_float sig = *p->ksig;
    cs_float threshold = *p->kthreshold;

    switch ((int32_t) CS_FLOAT2LONG(*p->kmode)) {
    case 0:       /* down-up */
      if (p->old_sig <= threshold &&
          sig > threshold)
        *p->kout = FL(1.0);
      else
        *p->kout = FL(0.0);
      break;
    case 1:      /* up-down */
      if (p->old_sig >= threshold &&
          sig < threshold)
        *p->kout = FL(1.0);
      else
        *p->kout = FL(0.0);
      break;
    case 2:      /* both */
      if ((p->old_sig <= threshold && sig > threshold) ||
          (p->old_sig >= threshold && sig < threshold ) )
        *p->kout = FL(1.0);
      else
        *p->kout = FL(0.0);
      break;
    default:
      return
        csound->PerfError(csound, &(p->h), "%s",
                          Str(" bad imode value"));
    }
    p->old_sig = sig;
    return OK;
}

/*-------------------------------*/

static int32_t interpol(CSOUND *csound, INTERPOL *p)
{
    IGN(csound);
    cs_float point_value = (*p->point - *p->imin) / (*p->imax - *p->imin);
    *p->r = point_value * (*p->val2 - *p->val1) + *p->val1;
    return OK;
}

static int32_t nterpol_init(CSOUND *csound, INTERPOL *p)
{
    if (LIKELY(*p->imax != *p->imin))
      p->point_factor = FL(1.0)/(*p->imax - *p->imin);
    else
      return csound->InitError(csound, "%s", Str("Min and max the same"));
    return OK;
 }

static int32_t knterpol(CSOUND *csound, INTERPOL *p)
{
    IGN(csound);
    cs_float point_value = (*p->point - *p->imin ) * p->point_factor;
    *p->r = point_value * (*p->val2 - *p->val1) + *p->val1;
    return OK;
}

static int32_t anterpol(CSOUND *csound, INTERPOL *p)
{
    IGN(csound);
    cs_float point_value = (*p->point - *p->imin ) * p->point_factor;
    cs_float *out        = p->r, *val1 = p->val1, *val2 = p->val2;
    uint32_t offset   = p->h.insdshead->ksmps_offset;
    uint32_t early    = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      cs_float fv1 = val1[n];
      out[n] = point_value * (val2[n] - fv1) + fv1;
    }
    return OK;
}

      
static int32_t sum_init(CSOUND *csound, SUM *p) {
    uint32_t nsmps = CS_KSMPS;
    if (p->aux.auxp == NULL || p->aux.size < nsmps * sizeof(cs_float))
      csound->AuxAlloc(csound, (size_t)(nsmps*sizeof(cs_float)), &p->aux);
    return OK;
}


static int32_t sum_(CSOUND *csound, SUM *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t k, nsmps = CS_KSMPS;
    int32_t   count = (int32_t) p->INOCOUNT;
    cs_float *accum = p->aux.auxp, **args = p->argums;
    cs_float *in0, *in1, *in2, *in3;
    if (UNLIKELY(offset)) memset(accum, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&accum[nsmps], '\0', early*sizeof(cs_float));
    }
    memset(accum, '\0', nsmps*sizeof(cs_float));
    int32_t count4 = count - (count % 4);
    for(int32_t i=0; i<count4; i+= 4) {
      in0 = *(args+i);
      in1 = *(args+i+1);
      in2 = *(args+i+2);
      in3 = *(args+i+3);
      for(k=offset; k<nsmps; k++) {
        // ar[k] += in0[k] + in1[k] +in2[k] + in3[k];
        accum[k] += in0[k] + in1[k] +in2[k] + in3[k];
      }
    }
    for(int32_t i=count4; i<count; i++) {
      in0 = *(args + i);
      for(k=offset; k<nsmps; k++) {
        // ar[k] += in0[k];
        accum[k] += in0[k];
      }
    }
    memcpy(p->ar, accum, CS_KSMPS*sizeof(cs_float));

    return OK;
}

/* Actually by JPff but after Gabriel */
static int32_t product_init(CSOUND *csound, SUM *p)
{
    if (UNLIKELY(p->INOCOUNT == 0))
      return csound->InitError(csound, Str("product requires an input"));
    /* Keep existing scratch space sized on reinit. */
    if (p->aux.auxp != NULL)
      return sum_init(csound, p);
    for (int32_t i = 1; i < p->INOCOUNT; ++i) {
      if (p->ar == p->argums[i])
        return sum_init(csound, p);
    }
    return OK;
}

static int32_t product(CSOUND *csound, SUM *p)
{
    IGN(csound);
    int32_t    count = (int32_t) p->INOCOUNT;
    uint32_t  offset = p->h.insdshead->ksmps_offset;
    uint32_t  early  = p->h.insdshead->ksmps_no_end;
    uint32_t   k, nsmps = CS_KSMPS;
    cs_float *ar = p->aux.auxp != NULL ? (cs_float *)p->aux.auxp : p->ar;
    cs_float **args = p->argums;
    cs_float *ag = *args;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    if (ar != ag)
      memcpy(&ar[offset], &ag[offset], sizeof(cs_float)*(nsmps-offset));
    while (--count) {
      ag = *(++args);                   /* over all arguments */
      for (k=offset; k<nsmps; k++) {
        ar[k] *= ag[k];                 /* Over audio vector */
      }
    }
    if (ar != p->ar)
      memcpy(p->ar, ar, CS_KSMPS*sizeof(cs_float));
    return OK;
}

static int32_t rsnsety(CSOUND *csound, RESONY *p)
{
    /* icorrect opts into base-relative linear spacing; old calls keep
       the original spacing and integer conversion of isepmode. */
    if (UNLIKELY(*p->icorrect != FL(0.0) && *p->icorrect != FL(1.0)))
      return csound->InitError(csound, "%s",
                               Str("resony: icorrect must be 0 or 1"));
    cs_double order = (cs_double)*p->ord;
    cs_double scale_value = (cs_double)*p->iscl;
    size_t state_size;
    int32_t clear_state = !*p->istor;
    int32_t new_loop, scale;
    uint32_t nsmps = CS_KSMPS;
    if (UNLIKELY(!isfinite(scale_value) || scale_value < (cs_double)INT32_MIN ||
                 scale_value > (INT32_MAX + 0.0))) {
      return csound->InitError(csound, Str("illegal reson iscl value: %f"),
                               *p->iscl);
    }
    p->scale = scale = (int32_t)scale_value;
    if (UNLIKELY(scale && scale != 1 && scale != 2)) {
      return csound->InitError(csound, Str("illegal reson iscl value: %f"),
                                       *p->iscl);
    }
    if (UNLIKELY(!isfinite(order) || order > (INT32_MAX + 0.0) - 0.5))
      return csound->InitError(csound, Str("resony: invalid order %f"),
                               *p->ord);
    new_loop = order < 0.5 ? 4 : (int32_t)(order + 0.5);
    if (UNLIKELY((size_t)new_loop > SIZE_MAX / (2 * sizeof(cs_float))))
      return csound->InitError(csound, Str("resony: order is too large"));
    clear_state |= p->aux.auxp == NULL || p->loop != new_loop;
    p->loop = new_loop;
    state_size = (size_t)p->loop * 2 * sizeof(cs_float);
    if (p->aux.auxp == NULL || state_size > p->aux.size) {
      csound->AuxAlloc(csound, state_size, &p->aux);
      clear_state = 1;
    }
    p->yt1 = (cs_float*)p->aux.auxp;
    p->yt2 = p->yt1 + p->loop;
    if (clear_state)
      memset(p->yt1, 0, state_size);
    if (p->buffer.auxp == NULL || p->buffer.size<nsmps*sizeof(cs_float))
      csound->AuxAlloc(csound, (size_t)(nsmps*sizeof(cs_float)), &p->buffer);
    return OK;
}

static int32_t resony(CSOUND *csound, RESONY *p)
{
    int32_t j;
    cs_float   *ar = p->ar, *asig;
    cs_float   c3p1, c3t4, omc3, c2sqr;
    cs_float   *yt1, *yt2, c1, c2, c3, cosf;
    cs_double  cf;
    int32_t loop = p->loop;
    if (UNLIKELY(loop==0))
      return csound->InitError(csound, "%s", Str("loop cannot be zero"));
    if (UNLIKELY(*p->icorrect && *p->kcf == FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("resony: base frequency must be nonzero"));
    {
      cs_float   sep = (*p->sep / (cs_float) loop);
      int32_t     flag = *p->icorrect ? (*p->iflag != FL(0.0)) :
                        (int32_t)*p->iflag;
      cs_float   *buffer = (cs_float*) (p->buffer.auxp);
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;

      asig = p->asig;

      memset(buffer, 0, nsmps*sizeof(cs_float));
      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ar[nsmps], '\0', early*sizeof(cs_float));
      }

      yt1 = p->yt1;
      yt2 = p->yt2;

      for (j = 0; j < loop; j++) {
        if (flag && *p->icorrect)      /* linear separation in hertz */
          cosf = (cs_float) cos((cf = (cs_double) (*p->kcf + sep * j))
                             * (cs_double) CS_TPIDSR);
        else if (flag)                /* original linear spacing */
          cosf = (cs_float) cos((cf = (cs_double) (*p->kcf * sep * j))
                             * (cs_double) CS_TPIDSR);
        else                          /* logarithmic separation in octaves */
          cosf = (cs_float) cos((cf = (cs_double) (*p->kcf * pow(2.0, sep * j)))
                             * (cs_double) CS_TPIDSR);
        c3 = EXP(*p->kbw * (cf / *p->kcf) * CS_MTPIDSR);
        c3p1 = c3 + FL(1.0);
        c3t4 = c3 * FL(4.0);
        c2 = c3t4 * cosf / c3p1;
        c2sqr = c2 * c2;
        omc3 = FL(1.0) - c3;
        if (p->scale == 1)
          c1 = omc3 * SQRT(FL(1.0) - c2sqr / c3t4);
        else if (p->scale == 2)
          c1 = SQRT((c3p1*c3p1-c2sqr) * omc3/c3p1);
        else
          c1 = FL(1.0);
        for (n = offset; n < nsmps; n++) {
          cs_float temp = c1 * asig[n] + c2 * *yt1 - c3 * *yt2;
          buffer[n] += temp;
          *yt2 = *yt1;
          *yt1 = temp;
        }
        yt1++;
        yt2++;
      }
      memcpy(&ar[offset], &buffer[offset], sizeof(cs_float)*(nsmps-offset));
      return OK;
    }
}

static int32_t fold_set(CSOUND *csound, FOLD *p)
{
    IGN(csound);
    p->index = 0.0;
    p->value = FL(0.0);         /* This was not initialised -- JPff */
    return OK;
}

static int32_t fold(CSOUND *csound, FOLD *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float *ar = p->ar;
    cs_float *asig = p->asig;
    cs_float kincr = *p->kincr;
    cs_double index = p->index;
    cs_float value = p->value;
    if (UNLIKELY(!isfinite(kincr) || kincr < FL(1.0)))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("fold: increment must be finite and >= 1"));
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      if (index <= 0.0) {
        index += (cs_double)kincr;
        value = asig[n];
      }
      ar[n] = value;
      index -= 1.0;
    }
    p->index = index;
    p->value = value;
    return OK;
}

/* by Gab Maldonado. Under GNU license with a special exception for
   Canonical Csound addition */

/* Normalize before lookup as well as after advancing the loop. */
#define LOOPSEG_WRAP_PHASE(phs) do {                                    \
    if ((phs) < 0.0 || (phs) >= 1.0) {                                  \
      (phs) -= floor(phs);                                              \
      /* A tiny negative phase can round up to one. */                  \
      if ((phs) >= 1.0) (phs) = 0.0;                                    \
    }                                                                   \
} while (0)

static int32_t loopseg_set(CSOUND *csound, LOOPSEG *p)
{
    p->nsegs   = p->INOCOUNT-3;
    // Should check this is even
    if (UNLIKELY((p->nsegs&1)!=0))
      csound->Warning(csound, "%s", Str("loop opcode: wrong argument count"));
    p->args[0] = FL(0.0);
    p->phs     = *p->iphase;
    return OK;
}

static int32_t loopseg(CSOUND *csound, LOOPSEG *p)
{
    IGN(csound);
    cs_float *argp=p->args;
    cs_float beg_seg=FL(0.0), end_seg, durtot=FL(0.0);
    cs_double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs+1;
    int32_t j;
    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;
    LOOPSEG_WRAP_PHASE(phs);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs-1; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        cs_float diff = end_seg - beg_seg;
        cs_float fract = ((cs_float)phs-beg_seg)/diff;
        cs_float v1 = argp[j+1];
        cs_float v2 = argp[j+3];
        *p->out = v1 + (v2-v1) * fract;
        break;
      }
    }
    phs    += si;
    LOOPSEG_WRAP_PHASE(phs);
    p->phs = phs;
    return OK;
}

static int32_t loopxseg(CSOUND *csound, LOOPSEG *p)
{
    IGN(csound);
    cs_float exp1 = FL(1.0)/(FL(1.0)-EXP(FL(1.0)));
    cs_float *argp=p->args;
    cs_float beg_seg=FL(0.0), end_seg, durtot=FL(0.0);
    cs_double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs+1;
    int32_t j;
    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;
    LOOPSEG_WRAP_PHASE(phs);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs-1; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        cs_float diff = end_seg - beg_seg;
        cs_float fract = ((cs_float)phs-beg_seg)/diff;
        cs_float v1 = argp[j+1];
        cs_float v2 = argp[j+3];
        *p->out = v1 + (v2 - v1) * (1 - EXP(fract)) * exp1;
        break;
      }
    }
    phs    += si;
    LOOPSEG_WRAP_PHASE(phs);
    p->phs = phs;
    return OK;
}

static int32_t looptseg_set(CSOUND *csound, LOOPTSEG *p)
{
    IGN(csound);
    p->nsegs   = (p->INOCOUNT-2)/3;
    p->phs     = *p->iphase;
    return OK;
}

static int32_t looptseg(CSOUND *csound, LOOPTSEG *p)
{
    IGN(csound);
    cs_float beg_seg=FL(0.0), end_seg=FL(0.0), durtot=FL(0.0);
    cs_double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs;
    int32_t j;

    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;
    LOOPSEG_WRAP_PHASE(phs);

    for ( j=0; j<nsegs; j++)
      durtot += *(p->argums[j].time);
    for ( j=0; j < nsegs; j++) {
      beg_seg = end_seg;
      end_seg = beg_seg + *(p->argums[j].time) / durtot;
      if (beg_seg <= phs && end_seg > phs) {
        cs_float alpha = *(p->argums[j].type);
        cs_float diff = end_seg - beg_seg;
        cs_float fract = ((cs_float)phs-beg_seg)/diff;
        cs_float v1 = *(p->argums[j].start);
        cs_float v2 = (j!=nsegs-1)?*(p->argums[j+1].start):*(p->argums[0].start);
        if (alpha==FL(0.0))
          *p->out = v1 + (v2 - v1) * fract;
        else if (alpha > FL(0.0))
          /* Keep exponents nonpositive to avoid overflow.  expm1 also
             preserves the linear limit when the curve is close to zero. */
          *p->out = v1 + (v2 - v1) *
            exp((cs_double)alpha * (fract - 1.0)) *
            (expm1(-(cs_double)alpha * fract) / expm1(-(cs_double)alpha));
        else
          *p->out = v1 + (v2 - v1) *
            (expm1((cs_double)alpha * fract) / expm1((cs_double)alpha));
        break;
      }
    }
    phs    += si;
    LOOPSEG_WRAP_PHASE(phs);
    p->phs = phs;
    return OK;
}

static int32_t lpshold(CSOUND *csound, LOOPSEG *p)
{
    IGN(csound);
    cs_float *argp=p->args;
    cs_float beg_seg=0, end_seg, durtot=FL(0.0);
    cs_double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs+1;
    int32_t j;

    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;
    LOOPSEG_WRAP_PHASE(phs);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];
    argp[nsegs] = *p->argums[0];
    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];

    for ( j=0; j < nsegs-1; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;
      if (beg_seg <= phs && end_seg > phs) {
        if (beg_seg <= phs && end_seg > phs) {
          *p->out = argp[j+1];
          break;
        }
      }
    }
    phs    += si;
    LOOPSEG_WRAP_PHASE(phs);
    p->phs = phs;
    return OK;
}

static int32_t loopsegp_set(CSOUND *csound, LOOPSEGP *p)
{
    IGN(csound);
    p->nsegs   = p->INOCOUNT-1;
    p->args[0] = FL(0.0);
    return OK;
}

static int32_t loopsegp(CSOUND *csound, LOOPSEGP *p)
{
    IGN(csound);
    cs_float *argp = p->args;
    cs_float beg_seg=0, end_seg, durtot=FL(0.0);
    cs_float phs;
    int32_t nsegs=p->nsegs+1;
    int32_t j;

    phs = *p->kphase;

    LOOPSEG_WRAP_PHASE(phs);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs-1; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        cs_float diff = end_seg - beg_seg;
        cs_float fract = ((cs_float)phs-beg_seg)/diff;
        cs_float v1 = argp[j+1];
        cs_float v2 = argp[j+3];
        *p->out = v1 + (v2-v1) * fract;
        break;
      }
    }
    return OK;
}

static int32_t lpsholdp(CSOUND *csound, LOOPSEGP *p)
{
    IGN(csound);
    cs_float *argp=p->args;
    cs_float beg_seg=FL(0.0), end_seg, durtot=FL(0.0);
    cs_float phs;
    int32_t nsegs=p->nsegs+1;
    int32_t j;

    phs = *p->kphase;

    LOOPSEG_WRAP_PHASE(phs);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs-1; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        if (beg_seg <= phs && end_seg > phs) {
          *p->out = argp[j+1];
          break;
        }
      }
    }
    return OK;
}

#undef LOOPSEG_WRAP_PHASE

/* by Gab Maldonado. Under GNU license with a special exception
   for Canonical Csound addition */

static int32_t lineto_set(CSOUND *csound, LINETO *p)
{
    IGN(csound);
    p->remaining = 0.0;
    p->incr = FL(0.0);
    p->flag = 1;
    return OK;
}

static int32_t lineto(CSOUND *csound, LINETO *p)
{
    IGN(csound);
    if (UNLIKELY(p->flag)) {
      p->val_incremented = p->current_val = *p->ksig;
      p->flag = 0;
    }
    if (p->remaining > 0.0) {
      p->val_incremented += p->incr;
      if (--p->remaining == 0.0)
        p->val_incremented = p->current_val;
    }
    /* Accept a new target only after the current ramp has finished. */
    if (p->remaining <= 0.0 && *p->ksig != p->current_val) {
      p->current_val = *p->ksig;
      p->remaining = ceil(*p->ktime * CS_EKR);
      if (p->remaining > 0.0)
        p->incr = (p->current_val - p->val_incremented) / p->remaining;
      else
        p->val_incremented = p->current_val;
    }
    *p->kr = p->val_incremented;
    return OK;
}

static int32_t tlineto_set(CSOUND *csound, LINETO2 *p)
{
    IGN(csound);
    p->remaining = 0.0;
    p->incr = FL(0.0);
    p->flag = 1;
    return OK;
}

static int32_t tlineto(CSOUND *csound, LINETO2 *p)
{
    IGN(csound);
    if (UNLIKELY(p->flag)) {
      p->val_incremented = p->current_val = *p->ksig;
      p->flag = 0;
    }
    if (*p->ktrig) {
      p->current_val = *p->ksig;
      p->remaining = ceil(*p->ktime * CS_EKR);
      /* A retrigger starts from the output, not the previous target. */
      if (p->remaining > 0.0)
        p->incr = (p->current_val - p->val_incremented) / p->remaining;
      else
        p->val_incremented = p->current_val;
    }
    else if (p->remaining > 0.0) {
      p->val_incremented += p->incr;
      if (--p->remaining == 0.0)
        p->val_incremented = p->current_val;
    }
    *p->kr = p->val_incremented;
    return OK;
}

/* by Gabriel Maldonado. Under GNU license with a special exception
   for Canonical Csound addition */

/* Higher rates can emit only one new value per output sample.  Keep their
   phase remainder while forcing that update. */
#define RANDOM_PHASE_INCREMENT(inc_, rate_, scale_)                        \
    do {                                                                  \
      cs_float scaled_ = (rate_) * (scale_);                                 \
      if (LIKELY(scaled_ > FL(0.0) && scaled_ < FMAXLEN))                 \
        (inc_) = (uint32_t)scaled_;                                       \
      else if (UNLIKELY(!(scaled_ > FL(0.0))))                            \
        (inc_) = 0U;                                                      \
      else if (scaled_ < (cs_float)UINT64_MAX)                              \
        (inc_) = MAXLEN + (uint32_t)((uint64_t)scaled_ & PHMASK);         \
      else                                                                \
        (inc_) = MAXLEN;                                                  \
    } while (0)

static int32_t vibrato_set(CSOUND *csound, VIBRATO *p)
{
    FUNC        *ftp;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (*p->iphs >= 0 && *p->iphs<1.0)
        p->lphs = (cs_double)*p->iphs * ftp->flen;
      else if (UNLIKELY(*p->iphs>=1.0))
        return csound->InitError(csound, "%s", Str("vibrato@ Phase out of range"));
    }
    else return NOTOK;
    p->xcpsAmpRate = randGab(csound) *(*p->ampMaxRate - *p->ampMinRate) +
      *p->ampMinRate;
    p->xcpsFreqRate = randGab(csound) *(*p->cpsMaxRate - *p->cpsMinRate) +
      *p->cpsMinRate;
    p->phsAmpRate = p->phsFreqRate = 0;
    p->num1amp = p->num2amp = p->num1freq = p->num2freq = FL(0.0);
    p->dfdmaxAmp = p->dfdmaxFreq = FL(0.0);
    p->tablen = ftp->flen;
    p->tablenUPkr = p->tablen * CS_ONEDKR;
    return OK;
}

static int32_t vibrato(CSOUND *csound, VIBRATO *p)
{
    FUNC        *ftp;
    cs_double      phs, inc;
    uint32_t    rateInc;
    cs_float       *ftab, fract, v1;
    cs_float       RandAmountAmp,RandAmountFreq;

    RandAmountAmp = (p->num1amp + (cs_float)p->phsAmpRate * p->dfdmaxAmp) *
      *p->randAmountAmp ;
    RandAmountFreq = (p->num1freq + (cs_float)p->phsFreqRate * p->dfdmaxFreq) *
      *p->randAmountFreq ;

    phs = p->lphs;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    fract = (cs_float) (phs - (int32)phs);
    ftab = ftp->ftable + (int32)phs;
    v1 = *ftab++;
    *p->out = (v1 + (*ftab - v1) * fract) *
      (*p->AverageAmp * POWER(FL(2.0),RandAmountAmp));
    inc = ( *p->AverageFreq * POWER(FL(2.0),RandAmountFreq)) *  p->tablenUPkr;
    phs += inc;
    while (phs >= p->tablen)
      phs -= p->tablen;
    while (phs < 0.0 )
      phs += p->tablen;
    p->lphs = phs;
    RANDOM_PHASE_INCREMENT(rateInc, p->xcpsAmpRate, CS_KICVT);
    p->phsAmpRate += rateInc;
    if (p->phsAmpRate >= MAXLEN) {
      p->xcpsAmpRate =  randGab(csound)  * (*p->ampMaxRate - *p->ampMinRate) +
        *p->ampMinRate;
      p->phsAmpRate &= PHMASK;
      p->num1amp = p->num2amp;
      p->num2amp = BiRandGab(csound) ;
      p->dfdmaxAmp = (p->num2amp - p->num1amp) / FMAXLEN;
    }
    RANDOM_PHASE_INCREMENT(rateInc, p->xcpsFreqRate, CS_KICVT);
    p->phsFreqRate += rateInc;
    if (p->phsFreqRate >= MAXLEN) {
      p->xcpsFreqRate =  randGab(csound)  * (*p->cpsMaxRate - *p->cpsMinRate) +
        *p->cpsMinRate;
      p->phsFreqRate &= PHMASK;
      p->num1freq = p->num2freq;
      p->num2freq = BiRandGab(csound) ;
      p->dfdmaxFreq = (p->num2freq - p->num1freq) / FMAXLEN;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("vibrato(krate): not initialised"));
}

static int32_t vibr_set(CSOUND *csound, VIBR *p)
  /* faster and easier to use than vibrato, but less flexible */
{
    FUNC        *ftp;
#define randAmountAmp   FL(1.59055)   /* these default values are far from */
#define randAmountFreq  FL(0.629921)  /* being the best.  If you think you */
#define ampMinRate      FL(1.0)       /* found better ones, please tell me */
#define ampMaxRate      FL(3.0)       /* by posting a message to
                                         g.maldonado@agora.stm.it */
#define cpsMinRate      FL(1.19377)
#define cpsMaxRate      FL(2.28100)

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      p->lphs = 0.0;
    }
    else return NOTOK;
    p->xcpsAmpRate = randGab(csound)  * (ampMaxRate - ampMinRate) + ampMinRate;
    p->xcpsFreqRate = randGab(csound)  * (cpsMaxRate - cpsMinRate) + cpsMinRate;
    p->phsAmpRate = p->phsFreqRate = 0;
    p->num1amp = p->num2amp = p->num1freq = p->num2freq = FL(0.0);
    p->dfdmaxAmp = p->dfdmaxFreq = FL(0.0);
    p->tablen = ftp->flen;
    p->tablenUPkr = p->tablen * CS_ONEDKR;
    return OK;
}

static int32_t vibr(CSOUND *csound, VIBR *p)
{
    FUNC        *ftp;
    cs_double      phs, inc;
    uint32_t    rateInc;
    cs_float       *ftab, fract, v1;
    cs_float       rAmountAmp,rAmountFreq;

    rAmountAmp =
      (p->num1amp+(cs_float)p->phsAmpRate * p->dfdmaxAmp)*randAmountAmp;
    rAmountFreq =
      (p->num1freq+(cs_float)p->phsFreqRate*p->dfdmaxFreq)*randAmountFreq;
    phs = p->lphs;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("vibrato(krate): not initialised"));
    }
    fract = (cs_float) (phs - (int32)phs); /*PFRAC(phs);*/
    ftab = ftp->ftable + (int32)phs; /*(phs >> ftp->lobits);*/
    v1 = *ftab++;
    *p->out = (v1 + (*ftab - v1) * fract) *
      (*p->AverageAmp * POWER(FL(2.0),rAmountAmp));
    inc = ( *p->AverageFreq * POWER(FL(2.0),rAmountFreq) ) *  p->tablenUPkr;
    phs += inc;
    while (phs >= p->tablen)
      phs -= p->tablen;
    while (phs < 0.0 )
      phs += p->tablen;
    p->lphs = phs;

    RANDOM_PHASE_INCREMENT(rateInc, p->xcpsAmpRate, CS_KICVT);
    p->phsAmpRate += rateInc;
    if (p->phsAmpRate >= MAXLEN) {
      p->xcpsAmpRate =  randGab(csound)  * (ampMaxRate - ampMinRate) + ampMinRate;
      p->phsAmpRate &= PHMASK;
      p->num1amp = p->num2amp;
      p->num2amp = BiRandGab(csound);
      p->dfdmaxAmp = (p->num2amp - p->num1amp) / FMAXLEN;
    }

    RANDOM_PHASE_INCREMENT(rateInc, p->xcpsFreqRate, CS_KICVT);
    p->phsFreqRate += rateInc;
    if (p->phsFreqRate >= MAXLEN) {
      p->xcpsFreqRate =  randGab(csound)  * (cpsMaxRate - cpsMinRate) + cpsMinRate;
      p->phsFreqRate &= PHMASK;
      p->num1freq = p->num2freq;
      p->num2freq = BiRandGab(csound);
      p->dfdmaxFreq = (p->num2freq - p->num1freq) / FMAXLEN;
    }
#undef  randAmountAmp
#undef  randAmountFreq
#undef  ampMinRate
#undef  ampMaxRate
#undef  cpsMinRate
#undef  cpsMaxRate
    return OK;
}

static int32_t jitter2_set(CSOUND *csound, JITTER2 *p)
{
    p->dfdmax1 = p->dfdmax2 = p->dfdmax3 = FL(0.0);
    p->phs1 = p->phs2 = p->phs3 = 0;
    p->num1a = p->num1b = p->num1c = FL(0.0);
    p->num2a = p->num2b = p->num2c = FL(0.0);
    if (*p->option != FL(0.0)) {
      p->num2a   = BiRandGab(csound);
      p->dfdmax1 = (p->num2a - p->num1a) / FMAXLEN;
      p->num2b   = BiRandGab(csound);
      p->dfdmax2 = (p->num2b- p->num1b) / FMAXLEN;
      p->num2c   = BiRandGab(csound);
      p->dfdmax3 = (p->num2c- p->num1c) / FMAXLEN;
    }
    return OK;
}

static int32_t jitter2(CSOUND *csound, JITTER2 *p)
{
    cs_float out1,out2,out3;
    cs_float cps1 = *p->cps1, cps2 = *p->cps2, cps3 = *p->cps3;
    uint32_t inc;
    out1 = (p->num1a + (cs_float)p->phs1 * p->dfdmax1);
    out2 = (p->num1b + (cs_float)p->phs2 * p->dfdmax2);
    out3 = (p->num1c + (cs_float)p->phs3 * p->dfdmax3);

    /* All-zero controls select the historical defaults. */
    if (cps1 == FL(0.0) && cps2 == FL(0.0) && cps3 == FL(0.0) &&
        *p->amp1 == FL(0.0) && *p->amp2 == FL(0.0) && *p->amp3 == FL(0.0)) {
      *p->out = (out1*FL(0.5) + out2*FL(0.3) + out3*FL(0.2)) * *p->gamp;
      cps1 = FL(0.82071231913);
      cps2 = FL(7.009019029039107);
      cps3 = FL(10.0);
    }
    else
      *p->out = (out1* *p->amp1 + out2* *p->amp2 + out3* *p->amp3) * *p->gamp;

    RANDOM_PHASE_INCREMENT(inc, cps1, CS_KICVT);
    p->phs1 += inc;
    RANDOM_PHASE_INCREMENT(inc, cps2, CS_KICVT);
    p->phs2 += inc;
    RANDOM_PHASE_INCREMENT(inc, cps3, CS_KICVT);
    p->phs3 += inc;
    if (p->phs1 >= MAXLEN) {
      p->phs1   &= PHMASK;
      p->num1a   = p->num2a;
      p->num2a   = BiRandGab(csound);
      p->dfdmax1 = (p->num2a - p->num1a) / FMAXLEN;
    }
    if (p->phs2 >= MAXLEN) {
      p->phs2   &= PHMASK;
      p->num1b   = p->num2b;
      p->num2b   = BiRandGab(csound);
      p->dfdmax2 = (p->num2b - p->num1b) / FMAXLEN;
    }
    if (p->phs3 >= MAXLEN) {
      p->phs3   &= PHMASK;
      p->num1c   = p->num2c;
      p->num2c   = BiRandGab(csound);
      p->dfdmax3 = (p->num2c - p->num1c) / FMAXLEN;
    }
    return OK;
}

static int32_t jitter_set(CSOUND *csound, JITTER *p)
{
    p->num2     = BiRandGab(csound);
    p->initflag = 1;
    p->phs=0;
    return OK;
}

static int32_t jitter(CSOUND *csound, JITTER *p)
{
    if (p->initflag) {
      p->initflag = 0;
      *p->ar = p->num2 * *p->amp;
      goto next;
    }
    *p->ar = (p->num1 + (cs_float)p->phs * p->dfdmax) * *p->amp;
    p->phs += (int32)(p->xcps * CS_KICVT);

    if (p->phs >= MAXLEN) {
    next:
      p->xcps   = randGab(csound)  * (*p->cpsMax - *p->cpsMin) + *p->cpsMin;
      p->phs   &= PHMASK;
      p->num1   = p->num2;
      p->num2   = BiRandGab(csound);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

static int32_t jitters_set(CSOUND *csound, JITTERS *p)
{
    p->num1     = BiRandGab(csound);
    p->num2     = BiRandGab(csound);
    p->df1      = FL(0.0);
    p->initflag = 1;
    p->cod      = IS_ASIG_ARG(p->amp) ? 1 : 0;
    p->phs      = 0;
    return OK;
}

/* Start the next segment at phase zero even at an exact boundary. */
#define JSPLINE_WRAP(phase) do {                                    \
    if ((phase) >= 1.0) (phase) -= floor(phase);                     \
  } while (0)

static int32_t jitters(CSOUND *csound, JITTERS *p)
{
    cs_float       x, c3= p->c3, c2= p->c2;
    cs_float       f0 = p->num0, df0= p->df0;

    if (p->initflag == 1) {
      p->initflag = 0;
      goto next;
    }
    p->phs += p->si;
    if (p->phs >= 1.0) {
      cs_float     slope, resd1, resd0, f2, f1;
    next:
      p->si = (randGab(csound) * (*p->cpsMax-*p->cpsMin) + *p->cpsMin)*CS_ONEDKR;
      if (p->si == 0) p->si = 1; /* Is this necessary? */
      JSPLINE_WRAP(p->phs);
      f0 = p->num0 = p->num1;
      f1 = p->num1 = p->num2;
      f2 = p->num2 = BiRandGab(csound);
      df0 = p->df0 = p->df1;
      p->df1 = ( f2  - f0 ) * FL(0.5);
      slope = f1 - f0;
      resd0 = df0 - slope;
      resd1 = p->df1 - slope;
      c3 = p->c3 = resd0 + resd1;
      c2 = p->c2 = - (resd1 + FL(2.0)* resd0);
    }
    x= (cs_float) p->phs;
    *p->ar = (((c3 * x + c2) * x + df0) * x + f0) * *p->amp;
    return OK;
}

static int32_t jittersa(CSOUND *csound, JITTERS *p)
{
    cs_float   x, c3=p->c3, c2=p->c2;
    cs_float   f0= p->num0, df0 = p->df0;
    cs_float   *ar = p->ar, *amp = p->amp;
    cs_float   cpsMax = *p->cpsMax, cpsMin = *p->cpsMin;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  cod = p->cod;
    cs_double phs = p->phs, si = p->si;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    if (UNLIKELY(offset >= nsmps)) return OK;
    if (cod) amp += offset;
    if (p->initflag) {
      p->initflag = 0;
      n = offset;
      goto next;
    }
    for (n=offset; n<nsmps; n++) {
      phs += si;
      if (phs >= 1.0) {
        cs_float   slope, resd1, resd0, f2, f1;
      next:
        si =  (randGab(csound)  * (cpsMax - cpsMin) + cpsMin)*CS_ONEDSR;
        if (si == 0) si = 1; /* Is this necessary? */
        JSPLINE_WRAP(phs);
        f0 = p->num0 = p->num1;
        f1 = p->num1 = p->num2;
        f2 = p->num2 = BiRandGab(csound);
        df0 = p->df0 = p->df1;
        p->df1 = ( f2 - f0 ) * FL(0.5);
        slope = f1 - f0;
        resd0 = df0 - slope;
        resd1 = p->df1 - slope;
        c3 = p->c3 = resd0 + resd1;
        c2 = p->c2 = - (resd1 + FL(2.0)* resd0);
      }
      x = (cs_float) phs;
      ar[n] = (((c3 * x + c2) * x + df0) * x + f0) * *amp;
      if (cod) amp++;
    }
    p->phs = phs;
    p->si =si;
    return OK;
}

/* Table lookup and number validation happen once per control block. */
static int32_t userrand_table(CSOUND *csound, cs_float *number,
                              FUNC **table, int32_t *previous) {
    cs_double value = (cs_double)*number;
    if (UNLIKELY(!(value >= (cs_double)INT32_MIN &&
                   value <= (INT32_MAX + 0.0))))
      return NOTOK;
    int32_t current = CS_FLOAT2LONG(*number);
    if (*table == NULL || *previous != current) {
      *table = csound->FTFind(csound, number);
      if (UNLIKELY(*table == NULL || (*table)->flen == 0))
        return NOTOK;
      *previous = current;
    }
    return OK;
}

static int32_t kDiscreteUserRand(CSOUND *csound, DURAND *p)
{
    if (UNLIKELY(userrand_table(csound, p->tableNum, &p->ftp, &p->pfn) != OK))
      return csound->PerfError(csound, &p->h, Str("Invalid ftable no. %f"),
                               *p->tableNum);
    USER_RAND_LOOKUP(*p->out, p->ftp->ftable, p->ftp->flen,
                     randGab(csound) * p->ftp->flen, 0);
    return OK;
}

static int32_t iDiscreteUserRand(CSOUND *csound, DURAND *p)
{
    p->ftp = NULL;
    if (UNLIKELY(userrand_table(csound, p->tableNum, &p->ftp, &p->pfn) != OK))
      return csound->InitError(csound, Str("Invalid ftable no. %f"), *p->tableNum);
    USER_RAND_LOOKUP(*p->out, p->ftp->ftable, p->ftp->flen,
                     randGab(csound) * p->ftp->flen, 0);
    return OK;
}

static int32_t aDiscreteUserRand(CSOUND *csound, DURAND *p)
{
    cs_float *out = p->out, *table;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, flen;

    if (UNLIKELY(userrand_table(csound, p->tableNum, &p->ftp, &p->pfn) != OK))
      return csound->PerfError(csound, &p->h, Str("Invalid ftable no. %f"),
                               *p->tableNum);
    table = p->ftp->ftable;
    flen = p->ftp->flen;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      USER_RAND_LOOKUP(out[n], table, flen, randGab(csound) * flen, 0);
    }
    return OK;
}

static int32_t kContinuousUserRand(CSOUND *csound, CURAND *p)
{
    cs_float value;
    if (UNLIKELY(userrand_table(csound, p->tableNum, &p->ftp, &p->pfn) != OK))
      return csound->PerfError(csound, &p->h, Str("Invalid ftable no. %f"),
                               *p->tableNum);
    USER_RAND_LOOKUP(value, p->ftp->ftable, p->ftp->flen,
                     randGab(csound) * p->ftp->flen, 1);
    *p->out = value * (*p->max - *p->min) + *p->min;
    return OK;
}

static int32_t iContinuousUserRand(CSOUND *csound, CURAND *p)
{
    cs_float value;
    p->ftp = NULL;
    if (UNLIKELY(userrand_table(csound, p->tableNum, &p->ftp, &p->pfn) != OK))
      return csound->InitError(csound, Str("Invalid ftable no. %f"), *p->tableNum);
    USER_RAND_LOOKUP(value, p->ftp->ftable, p->ftp->flen,
                     randGab(csound) * p->ftp->flen, 1);
    *p->out = value * (*p->max - *p->min) + *p->min;
    return OK;
}

static int32_t Cuserrnd_set(CSOUND *csound, CURAND *p)
{
    IGN(csound);
    p->ftp = NULL;
    p->pfn = 0;
    return OK;
}

static int32_t Duserrnd_set(CSOUND *csound, DURAND *p)
{
    IGN(csound);
    p->ftp = NULL;
    p->pfn = 0;
    return OK;
}

static int32_t aContinuousUserRand(CSOUND *csound, CURAND *p)
{
    cs_float min = *p->min, range = *p->max - min;
    cs_float *out = p->out, *table;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, flen;

    if (UNLIKELY(userrand_table(csound, p->tableNum, &p->ftp, &p->pfn) != OK))
      return csound->PerfError(csound, &p->h, Str("Invalid ftable no. %f"),
                               *p->tableNum);
    table = p->ftp->ftable;
    flen = p->ftp->flen;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      cs_float value;
      USER_RAND_LOOKUP(value, table, flen, randGab(csound) * flen, 1);
      out[n] = value * range + min;
    }
    return OK;
}

static int32_t ikRangeRand(CSOUND *csound, RANGERAND *p)
{ /* gab d5*/
    *p->out = randGab(csound) * (*p->max - *p->min) + *p->min;
    return OK;
}

static int32_t aRangeRand(CSOUND *csound, RANGERAND *p)
{ /* gab d5*/
    cs_float min = *p->min, max = *p->max, *out = p->out;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float rge = max - min;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      out[n] = randGab(csound) * rge + min;
    }
    return OK;
}

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
static int32_t randomi_set(CSOUND *csound, RANDOMI *p)
{
    int32_t mode = (int32_t)(*p->mode);
    p->phs = 0;
    switch (mode) {
    case 1: /* immediate interpolation between kmin and 1st random number */
        p->num1 = FL(0.0);
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
        break;
    case 2: /* immediate interpolation between ifirstval and 1st random number */
        p->num1 = (*p->max - *p->min) ?
          (*p->fstval - *p->min) / (*p->max - *p->min) : FL(0.0);
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
        break;
    case 3: /* immediate interpolation between 1st and 2nd random number */
        p->num1 = randGab(csound);
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
        break;
    default: /* old behaviour as developped by Gabriel */
        p->num1 = p->num2 = FL(0.0);
        p->dfdmax = FL(0.0);
    }
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

static int32_t krandomi(CSOUND *csound, RANDOMI *p)
{
    uint32_t inc;

    *p->ar = (p->num1 + (cs_float)p->phs * p->dfdmax) * (*p->max - *p->min) + *p->min;
    RANDOM_PHASE_INCREMENT(inc, *p->xcps, CS_KICVT);
    p->phs += inc;
    if (p->phs >= MAXLEN) {
      p->phs   &= PHMASK;
      p->num1   = p->num2;
      p->num2   = randGab(csound);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

static int32_t randomi(CSOUND *csound, RANDOMI *p)
{
    uint32_t    phs = p->phs, inc = 0U;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float       *ar, *cpsp;
    cs_float       amp, min;

    cpsp = p->xcps;
    min = *p->min;
    amp =  (*p->max - min);
    ar = p->ar;
    if (p->cpscod)
      cpsp += offset;
    else
      RANDOM_PHASE_INCREMENT(inc, *cpsp, CS_SICVT);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      if (p->cpscod)
        RANDOM_PHASE_INCREMENT(inc, *cpsp++, CS_SICVT);
      ar[n] = (p->num1 + (cs_float)phs * p->dfdmax) * amp + min;
      phs += inc;
      if (phs >= MAXLEN) {
        phs &= PHMASK;
        p->num1 = p->num2;
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    }
    p->phs = phs;
    return OK;
}

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
static int32_t randomh_set(CSOUND *csound, RANDOMH *p)
{
    int32_t mode = (int32_t)(*p->mode);
    p->phs = 0;
    switch (mode) {
    case 2: /* the first output value is ifirstval */
        p->num1 = (*p->max - *p->min) ?
          (*p->fstval - *p->min) / (*p->max - *p->min) : FL(0.0);
        break;
    case 3: /* the first output value is a random number within the defined range */
        p->num1 = randGab(csound);
        break;
    default: /* old behaviour as developped by Gabriel */
        p->num1 = FL(0.0);
    }
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

static int32_t krandomh(CSOUND *csound, RANDOMH *p)
{
    uint32_t inc;

    *p->ar = p->num1 * (*p->max - *p->min) + *p->min;
    RANDOM_PHASE_INCREMENT(inc, *p->xcps, CS_KICVT);
    p->phs += inc;
    if (p->phs >= MAXLEN) {
      p->phs &= PHMASK;
      p->num1 = randGab(csound);
    }
    return OK;
}

static int32_t randomh(CSOUND *csound, RANDOMH *p)
{
    uint32_t    phs = p->phs, inc = 0U;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float       *ar, *cpsp;
    cs_float       amp, min;

    cpsp = p->xcps;
    min  = *p->min;
    amp  = (*p->max - min);
    ar   = p->ar;
    if (p->cpscod)
      cpsp += offset;
    else
      RANDOM_PHASE_INCREMENT(inc, *cpsp, CS_SICVT);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      if (p->cpscod)
        RANDOM_PHASE_INCREMENT(inc, *cpsp++, CS_SICVT);
      ar[n]     = p->num1 * amp + min;
      phs      += inc;
      if (phs >= MAXLEN) {
        phs    &= PHMASK;
        p->num1 = randGab(csound);
      }
    }
    p->phs = phs;
    return OK;
}

#undef RANDOM_PHASE_INCREMENT

static int32_t random3_set(CSOUND *csound, RANDOM3 *p)
{
    p->num1     = randGab(csound);
    p->num2     = randGab(csound);
    p->df1      = FL(0.0);
    p->initflag = 1;
    p->rangeMin_cod = IS_ASIG_ARG(p->rangeMin);
    p->rangeMax_cod = IS_ASIG_ARG(p->rangeMax);
    p->phs      = 0.0;
    return OK;
}

static int32_t random3(CSOUND *csound, RANDOM3 *p)
{
    cs_float       x, c3= p->c3, c2= p->c2;
    cs_float       f0 = p->num0, df0= p->df0;
    cs_float       cpsMin = *p->cpsMin, cpsMax = *p->cpsMax;

    if (UNLIKELY(!isfinite(cpsMin) || !isfinite(cpsMax) ||
                 cpsMin < FL(0.0) || cpsMax < FL(0.0)))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("rspline: rates must be finite and non-negative"));

    if (p->initflag) {
      p->initflag = 0;
      goto next;
    }
    p->phs += p->si;
    if (p->phs >= 1.0) {
      cs_float     slope, resd1, resd0, f2, f1;
    next:
      p->si = (randGab(csound) * (cpsMax-cpsMin) + cpsMin)*CS_ONEDKR;
      if (p->phs > 1.0)
        p->phs = p->phs - ceil(p->phs) + 1.0;
      f0     = p->num0 = p->num1;
      f1     = p->num1 = p->num2;
      f2     = p->num2 = randGab(csound);
      df0    = p->df0 = p->df1;
      p->df1 = ( f2  - f0 ) * FL(0.5);
      slope  = f1 - f0;
      resd0  = df0 - slope;
      resd1  = p->df1 - slope;
      c3     = p->c3 = resd0 + resd1;
      c2     = p->c2 = - (resd1 + FL(2.0)* resd0);
    }
    x = (cs_float) p->phs;
    *p->ar = (((c3 * x + c2) * x + df0) * x + f0) *
      (*p->rangeMax - *p->rangeMin) + *p->rangeMin;
    return OK;
}

static int32_t random3a(CSOUND *csound, RANDOM3 *p)
{
    int32_t         rangeMin_cod = p->rangeMin_cod, rangeMax_cod = p->rangeMax_cod;
    cs_float       x, c3=p->c3, c2=p->c2;
    cs_float       f0 = p->num0, df0 = p->df0;
    cs_float       *ar = p->ar, *rangeMin = p->rangeMin;
    cs_float       *rangeMax = p->rangeMax;
    cs_float       cpsMin = *p->cpsMin, cpsMax = *p->cpsMax;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_double      phs = p->phs, si = p->si;

    if (UNLIKELY(!isfinite(cpsMin) || !isfinite(cpsMax) ||
                 cpsMin < FL(0.0) || cpsMax < FL(0.0)))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("rspline: rates must be finite and non-negative"));

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    if (rangeMin_cod) rangeMin += offset;
    if (rangeMax_cod) rangeMax += offset;
    if (p->initflag) {
      p->initflag = 0;
      n = offset;
      goto next;
    }
    for (n=offset; n<nsmps; n++) {
      phs += si;
      if (phs >= 1.0) {
        cs_float   slope, resd1, resd0, f2, f1;
      next:
        si =  (randGab(csound)  * (cpsMax - cpsMin) + cpsMin)*CS_ONEDSR;
        if (phs > 1.0) phs = phs - ceil(phs) + 1.0;
        f0     = p->num0 = p->num1;
        f1     = p->num1 = p->num2;
        f2     = p->num2 = randGab(csound);
        df0    = p->df0 = p->df1;
        p->df1 = ( f2 - f0 ) * FL(0.5);
        slope  = f1 - f0;
        resd0  = df0 - slope;
        resd1  = p->df1 - slope;
        c3     = p->c3 = resd0 + resd1;
        c2     = p->c2 = - (resd1 + FL(2.0)* resd0);
      }
      x = (cs_float) phs;
      ar[n] = (((c3 * x + c2) * x + df0) * x + f0) *
        (*rangeMax - *rangeMin) + *rangeMin;
      if (rangeMin_cod) rangeMin++;
      if (rangeMax_cod) rangeMax++;
    }
    p->phs = phs;
    p->si  = si;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "wrap.i", S(WRAP),     0,  "i", "iii",  (SUBR)kwrap, NULL,    NULL        },
{ "wrap.k", S(WRAP),     0,  "k", "kkk",  NULL,  (SUBR)kwrap,   NULL        },
{ "wrap.a", S(WRAP),     0,  "a", "akk",  NULL,          (SUBR)wrap  },
{ "mirror.i", S(WRAP),   0,  "i", "iii",  (SUBR)kmirror, NULL,  NULL        },
{ "mirror.k", S(WRAP),   0,  "k", "kkk",  NULL,  (SUBR)kmirror, NULL        },
{ "mirror.a", S(WRAP),   0,  "a", "akk",  NULL,         (SUBR)mirror },
{ "ntrpol.i",S(INTERPOL), 0, "i", "iiiop",(SUBR)interpol                     },
{ "ntrpol.k",S(INTERPOL), 0, "k", "kkkop",(SUBR)nterpol_init, (SUBR)knterpol },
{ "ntrpol.a",S(INTERPOL), 0, "a", "aakop",(SUBR)nterpol_init,(SUBR)anterpol},
{ "fold",    S(FOLD),     0, "a", "ak",   (SUBR)fold_set, (SUBR)fold      },
{ "lineto",   S(LINETO),  0, "k", "kk",   (SUBR)lineto_set,  (SUBR)lineto, NULL },
{ "tlineto",  S(LINETO2), 0, "k", "kkk",  (SUBR)tlineto_set, (SUBR)tlineto, NULL},
{ "vibrato",  S(VIBRATO), TR, "k", "kkkkkkkkio",
                                        (SUBR)vibrato_set, (SUBR)vibrato, NULL   },
{ "vibr",     S(VIBRATO), TR, "k", "kki",  (SUBR)vibr_set, (SUBR)vibr, NULL   },
{ "jitter2",  S(JITTER2), 0, "k", "kkkkkkko", (SUBR)jitter2_set, (SUBR)jitter2 },
{ "jitter",   S(JITTER),  0, "k", "kkk",  (SUBR)jitter_set, (SUBR)jitter, NULL },
{ "jspline",  S(JITTERS), 0, "k", "xkk",
                                (SUBR)jitters_set, (SUBR)jitters, NULL },
{ "jspline.a",  S(JITTERS), 0, "a", "xkk",
    (SUBR)jitters_set, (SUBR)jittersa },
{ "loopseg",  S(LOOPSEG), 0, "k", "kkiz", (SUBR)loopseg_set, (SUBR)loopseg, NULL},
{ "loopxseg", S(LOOPSEG), 0, "k", "kkiz", (SUBR)loopseg_set,(SUBR)loopxseg, NULL},
{ "looptseg", S(LOOPSEG), 0, "k", "kkiz",(SUBR)looptseg_set,(SUBR)looptseg, NULL},
{ "lpshold",  S(LOOPSEG), 0, "k", "kkiz",(SUBR)loopseg_set, (SUBR)lpshold, NULL },
{ "loopsegp", S(LOOPSEGP), 0,"k", "kz",  (SUBR)loopsegp_set,(SUBR)loopsegp, NULL},
{ "lpsholdp", S(LOOPSEGP), 0,"k", "kz",  (SUBR)loopsegp_set,(SUBR)lpsholdp, NULL},
{ "cuserrnd.i", S(CURAND),0,"i",  "iii",  (SUBR)iContinuousUserRand, NULL, NULL },
{ "cuserrnd.k", S(CURAND),0,"k",  "kkk",
                            (SUBR)Cuserrnd_set, (SUBR)kContinuousUserRand, NULL },
{ "cuserrnd.a",S(CURAND),0, "a", "kkk",
                            (SUBR)Cuserrnd_set, (SUBR)aContinuousUserRand },
{ "random.i", S(RANGERAND), 0, "i", "ii",    (SUBR)ikRangeRand, NULL, NULL      },
{ "random.k", S(RANGERAND), 0, "k", "kk",    NULL, (SUBR)ikRangeRand, NULL      },
{ "random.a", S(RANGERAND), 0, "a", "kk",    NULL,  (SUBR)aRangeRand      },
{ "rspline",  S(RANDOM3), 0, "k", "xxkk",
                               (SUBR)random3_set, (SUBR)random3, NULL },
{ "rspline.a",  S(RANDOM3), 0, "a", "xxkk",
                               (SUBR)random3_set, (SUBR)random3a },
{ "randomi",  S(RANDOMI), 0, "a", "kkxoo",
                               (SUBR)randomi_set, (SUBR)randomi },
{ "randomi.k",  S(RANDOMI), 0, "k", "kkkoo",
                               (SUBR)randomi_set, (SUBR)krandomi,NULL },
{ "randomh",  S(RANDOMH), 0, "a", "kkxoo",
                                 (SUBR)randomh_set,(SUBR)randomh },
{ "randomh.k",  S(RANDOMH), 0, "k", "kkkoo",
                                 (SUBR)randomh_set,(SUBR)krandomh,NULL},
{ "urd.i",  S(DURAND),  0, "i", "i", (SUBR)iDiscreteUserRand, NULL, NULL    },
{ "urd.k",  S(DURAND),  0, "k", "k",
                              (SUBR)Duserrnd_set, (SUBR)kDiscreteUserRand },
{ "urd.a",  S(DURAND),  0, "a", "k",
                              (SUBR)Duserrnd_set, (SUBR)aDiscreteUserRand },
{ "duserrnd.i", S(DURAND),0, "i", "i",  (SUBR)iDiscreteUserRand, NULL, NULL },
{ "duserrnd.k", S(DURAND),0, "k", "k",
                            (SUBR)Duserrnd_set,(SUBR)kDiscreteUserRand,NULL },
{ "duserrnd.a", S(DURAND),0, "a", "k",
                                (SUBR)Duserrnd_set,(SUBR)aDiscreteUserRand },
{ "trigger",  S(TRIG),  0, "k", "kkk",  (SUBR)trig_set, (SUBR)trig,   NULL  },
{ "sum",      S(SUM),   0, "a", "y",    (SUBR)sum_init, (SUBR)sum_               },
{ "product",  S(SUM),   0, "a", "y",    (SUBR)product_init, (SUBR)product },
{ "resony",  S(RESONY), 0, "a", "akkikoooo", (SUBR)rsnsety, (SUBR)resony }
};

int32_t uggab_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}
