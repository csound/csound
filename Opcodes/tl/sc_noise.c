/*
    sc_noise.c:

    Based on the noise ugens of SuperCollider.

    (c) Tito Latini, 2012

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

/*
        08.03.2015 gausstrig was fixed to properly work at k-time.
        Also I added an optional feature related to the behavior
  of very first impulse.

        -- Gleb Rogozinsky
*/

#include "csoundCore.h"

typedef struct {
        OPDS    h;
        MYFLT   *out, *kamp, *kdensity, density0, thresh, scale;
        int32   rand;
} DUST;

typedef struct {
        OPDS    h;
/* 8.03.15 Added new option ifrst1   --Gleb R */
        MYFLT   *out, *kamp, *kfrq, *kdev, *imode, *ifrst1, frq0;
        int32   count, rand, mmode;
} GAUSSTRIG;

#define BIPOLAR   0x7FFFFFFF    /* Constant to make bipolar */
#define dv2_31    (FL(4.656612873077392578125e-10))

static int dust_init(CSOUND *csound, DUST *p)
{
    p->density0 = FL(0.0);
    p->thresh   = FL(0.0);
    p->scale    = FL(0.0);
    p->rand     = csoundRand31(&csound->randSeed1);
    return OK;
}

static int dust_process_krate(CSOUND *csound, DUST *p)
{
    MYFLT   density, thresh, scale, r;
    density = *p->kdensity;

    if (density != p->density0) {
      thresh = p->thresh = density * csound->onedsr;
      scale  = p->scale  = (thresh > FL(0.0) ? FL(1.0) / thresh : FL(0.0));
      p->density0 = density;
    }
    else {
      thresh = p->thresh;
      scale  = p->scale;
    }
    p->rand = csoundRand31(&p->rand);
    r = (MYFLT)p->rand * dv2_31;
    *p->out = *p->kamp * (r < thresh ? r*scale : FL(0.0));
    return OK;
}

static int dust_process_arate(CSOUND *csound, DUST *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out, density, thresh, scale;
    out = p->out;
    density = *p->kdensity;

    if (density != p->density0) {
      thresh = p->thresh = density * csound->onedsr;
      scale  = p->scale  = (thresh > FL(0.0) ? FL(1.0) / thresh : FL(0.0));
      p->density0 = density;
    }
    else {
      thresh = p->thresh;
      scale  = p->scale;
    }
   memset(out, '\0', offset*sizeof(MYFLT));
   for (n=offset; n<nsmps; n++) {
      MYFLT r;
      p->rand = csoundRand31(&p->rand);
      r = (MYFLT)p->rand * dv2_31;
      out[n] = *p->kamp * (r < thresh ? r*scale : FL(0.0));
    }
    return OK;
}

static int dust2_process_krate(CSOUND *csound, DUST *p)
{
    MYFLT   density, thresh, scale, r;
    density = *p->kdensity;

    if (density != p->density0) {
      thresh = p->thresh = density * csound->onedsr;
      scale = p->scale = (thresh > FL(0.0) ? FL(2.0) / thresh : FL(0.0));
      p->density0 = density;
    }
    else {
      thresh = p->thresh;
      scale  = p->scale;
    }
    p->rand = csoundRand31(&p->rand);
    r = (MYFLT)p->rand * dv2_31;
    *p->out = *p->kamp * (r < thresh ? r*scale - FL(1.0) : FL(0.0));
    return OK;
}

static int dust2_process_arate(CSOUND *csound, DUST *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out, density, thresh, scale;
    out = p->out;
    density = *p->kdensity;

    if (density != p->density0) {
      thresh = p->thresh = density * csound->onedsr;
      scale = p->scale = (thresh > FL(0.0) ? FL(2.0) / thresh : FL(0.0));
      p->density0 = density;
    }
    else {
      thresh = p->thresh;
      scale  = p->scale;
    }
    memset(out, '\0', offset*sizeof(MYFLT));
    for (n=offset; n<nsmps; n++) {
      MYFLT r;
      p->rand = csoundRand31(&p->rand);
      r = (MYFLT)p->rand * dv2_31;
      out[n] = *p->kamp * (r < thresh ? r*scale - FL(1.0) : FL(0.0));
    }
    return OK;
}

/* gausstrig opcode based on Bhob Rainey's GaussTrig ugen */
static int gausstrig_init(CSOUND* csound, GAUSSTRIG *p)
{
    p->rand  = csoundRand31(&csound->randSeed1);
    if (*p->ifrst1 > FL(0.0)) {
      /* values less than FL(0.0) could be used in later versions
         as an offset in samples */
      int     nextsamps;
      MYFLT   nextcount, frq, dev, r1, r2;
      p->frq0 = *p->kfrq;
      frq = (*p->kfrq > FL(0.001) ? *p->kfrq : FL(0.001));
      dev = *p->kdev;

      nextsamps = (int)(csound->GetSr(csound) / frq);
      p->rand = csoundRand31(&p->rand);
      r1 = (MYFLT)p->rand * dv2_31;
      p->rand = csoundRand31(&p->rand);
      r2 = (MYFLT)p->rand * dv2_31;
      nextcount = SQRT(FL(-2.0) * LOG(r1)) * SIN(r2 * TWOPI_F);
      if (nextcount < FL(-1.0)) {
        MYFLT diff = FL(-1.0) - nextcount;
        nextcount  = (FL(1.0) < FL(-1.0) + diff ? FL(1.0) : FL(-1.0) + diff);
      }
      else if (nextcount > FL(1.0)) {
        MYFLT diff = nextcount - FL(1.0);
        nextcount  = (FL(-1.0) > FL(1.0) - diff ? FL(-1.0) : FL(1.0) - diff);
      }
      p->count = (int)(nextsamps + nextcount * dev * nextsamps);
    }
    else {
      /* GaussTrig UGen behavior */
      p->count = 0;
    }
    /*
     * imode > 0 means better frequency modulation. If the frequency
     * changes, the delay before the next impulse is calculed again.
     * With the default imode value we have the classic behavior of
     * the GaussTrig ugen, where the freq modulation is bypassed
     * during the delay time that precedes the next impulse.
     */
    p->mmode = (*p->imode <= FL(0.0) ? 0 : 1);
    return OK;
}
/* a separate k-time init for proper work of gausstrig */
static int gausstrig_initk(CSOUND* csound, GAUSSTRIG *p)
{
    p->rand  = csoundRand31(&csound->randSeed1);
    if (*p->ifrst1 > FL(0.0)) {
      /* values less than FL(0.0) could be used in later versions
         as an offset in samples */
      int     nextsamps;
      MYFLT   nextcount, frq, dev, r1, r2;
      p->frq0 = *p->kfrq;
      frq = (*p->kfrq > FL(0.001) ? *p->kfrq : FL(0.001));
      dev = *p->kdev;
      /* this very line of k-time fix. Changed GetSt to GetKr */
      nextsamps = (int)(csound->GetKr(csound) / frq);
      p->rand = csoundRand31(&p->rand);
      r1 = (MYFLT)p->rand * dv2_31;
      p->rand = csoundRand31(&p->rand);
      r2 = (MYFLT)p->rand * dv2_31;
      nextcount = SQRT(FL(-2.0) * LOG(r1)) * SIN(r2 * TWOPI_F);
      if (nextcount < FL(-1.0)) {
        MYFLT diff = FL(-1.0) - nextcount;
        nextcount  = (FL(1.0) < FL(-1.0) + diff ? FL(1.0) : FL(-1.0) + diff);
      }
      else if (nextcount > FL(1.0)) {
        MYFLT diff = nextcount - FL(1.0);
        nextcount  = (FL(-1.0) > FL(1.0) - diff ? FL(-1.0) : FL(1.0) - diff);
      }
      p->count = (int)(nextsamps + nextcount * dev * nextsamps);
    }
    else {
      /* GaussTrig UGen behavior */
      p->count = 0;
    }
    p->mmode = (*p->imode <= FL(0.0) ? 0 : 1);
    return OK;
}
static int gausstrig_process_krate(CSOUND* csound, GAUSSTRIG *p)
{
    if (p->count <= 0) {
      int     nextsamps;
      MYFLT   nextcount, frq, dev, r1, r2;
      p->frq0 = *p->kfrq;
      frq = (*p->kfrq > FL(0.001) ? *p->kfrq : FL(0.001));
      dev = *p->kdev;
/* this very line of k-time fix. Changed GetSt to GetKr */
      nextsamps = (int)(csound->GetKr(csound) / frq);
      p->rand = csoundRand31(&p->rand);
      r1 = (MYFLT)p->rand * dv2_31;
      p->rand = csoundRand31(&p->rand);
      r2 = (MYFLT)p->rand * dv2_31;
      nextcount = SQRT(FL(-2.0) * LOG(r1)) * SIN(r2 * TWOPI_F);
      if (nextcount < FL(-1.0)) {
        MYFLT diff = FL(-1.0) - nextcount;
        nextcount  = (FL(1.0) < FL(-1.0) + diff ? FL(1.0) : FL(-1.0) + diff);
      }
      else if (nextcount > FL(1.0)) {
        MYFLT diff = nextcount - FL(1.0);
        nextcount  = (FL(-1.0) > FL(1.0) - diff ? FL(-1.0) : FL(1.0) - diff);
      }
      p->count = (int)(nextsamps + nextcount * dev * nextsamps);
      *p->out = *p->kamp;
    }
    else {
      if (p->mmode && *p->kfrq != p->frq0)
        p->count = 0;
      *p->out = FL(0.0);
    }
    p->count--;
    return OK;
}

static int gausstrig_process_arate(CSOUND* csound, GAUSSTRIG *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out = p->out;
    memset(out, '\0', offset*sizeof(MYFLT));
    for (n=offset; n<nsmps; n++) {
      if (p->count <= 0) {
        int     nextsamps;
        MYFLT   nextcount, frq, dev, r1, r2;
        p->frq0 = *p->kfrq;
        frq = (*p->kfrq > FL(0.001) ? *p->kfrq : FL(0.001));
        dev = *p->kdev;
        nextsamps = (int)(csound->GetSr(csound) / frq);
        p->rand = csoundRand31(&p->rand);
        r1 = (MYFLT)p->rand * dv2_31;
        p->rand = csoundRand31(&p->rand);
        r2 = (MYFLT)p->rand * dv2_31;
        nextcount = SQRT(FL(-2.0) * LOG(r1)) * SIN(r2 * TWOPI_F);
        if (nextcount < FL(-1.0)) {
          MYFLT diff = FL(-1.0) - nextcount;
          nextcount  = (FL(1.0) < FL(-1.0) + diff ? FL(1.0) : FL(-1.0) + diff);
        }
        else if (nextcount > FL(1.0)) {
          MYFLT diff = nextcount - FL(1.0);
          nextcount  = (FL(-1.0) > FL(1.0) - diff ? FL(-1.0) : FL(1.0) - diff);
        }
        p->count = (int)(nextsamps + nextcount * dev * nextsamps);
        out[n] = *p->kamp;
      }
      else {
        if (p->mmode && *p->kfrq != p->frq0)
          p->count = 0;
        out[n] = FL(0.0);
      }
      p->count--;
    }
    return OK;
}

static OENTRY scnoise_localops[] = {
  { "dust",      0xffff },
  { "dust2",     0xffff },
  { "gausstrig", 0xffff },
  { "dust.k",      sizeof(DUST), 0,3, "k", "kk",
    (SUBR)dust_init, (SUBR)dust_process_krate, NULL },
  { "dust.a",      sizeof(DUST), 0,5, "a", "kk",
    (SUBR)dust_init, NULL, (SUBR)dust_process_arate },
  { "dust2.k",     sizeof(DUST), 0,3, "k", "kk",
    (SUBR)dust_init, (SUBR)dust2_process_krate, NULL },
  { "dust2.a",     sizeof(DUST), 0,5, "a", "kk",
    (SUBR)dust_init, NULL, (SUBR)dust2_process_arate },
  { "gausstrig.k", sizeof(GAUSSTRIG), 0,3, "k", "kkkoo",
    (SUBR)gausstrig_initk, (SUBR)gausstrig_process_krate, NULL },
  { "gausstrig.a", sizeof(GAUSSTRIG), 0,5, "a", "kkkoo",
    (SUBR)gausstrig_init, NULL, (SUBR)gausstrig_process_arate }
};

LINKAGE_BUILTIN(scnoise_localops)
