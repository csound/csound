/*
    repluck.c:

    Copyright (C) 1996 John ffitch
                  1998 Victor Lazzarini

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

/***************************************************************\
*   repluck.c                                                  *
*   Various waveguide instruments                              *
*   3 March 1996 John ffitch                                   *
\***************************************************************/

#include "stdopcod.h"
#include "repluck.h"

static int32_t wgpsetin(CSOUND *, WGPLUCK2 *);

static int32_t wgpset(CSOUND *csound, WGPLUCK2 *p)
{
    p->ain = NULL;
    return wgpsetin(csound,p);
}

static int32_t wgpsetin(CSOUND *csound, WGPLUCK2 *p)
{
    double      periodSamples;
    size_t      railBytes;
    int32_t     period;
    int32_t     npts;
    int32_t     pickpt;
    int32_t     rail_len;
    MYFLT   upslope;
    MYFLT   downslope;
    int32_t     i;
    int32_t     scale;
    DelayLine   *upper_rail;
    DelayLine   *lower_rail;
    MYFLT   plk = *p->plk;
                                /* Initialize variables....*/
    periodSamples = CS_ESR / (double)*p->icps;
    if (UNLIKELY(!(periodSamples >= 1.0 &&
                   periodSamples <= (double)INT32_MAX)))
      return csound->InitError(csound, "%s",
                               Str("repluck/wgpluck2: invalid frequency"));
    period = (int32_t)periodSamples;
    scale = 512 / period;
    if (512 % period != 0)
      scale++;
    npts = period * scale;
    rail_len = npts/2/* + 1*/;      /* but only need half length */
    if (UNLIKELY(!(plk >= FL(0.0) && plk <= FL(1.0)))) {
      plk = (p->ain ? FL(0.0) : FL(0.5));
    }
    pickpt = (int32_t)(rail_len * plk);
    if (UNLIKELY((size_t)rail_len > SIZE_MAX / sizeof(MYFLT)))
      return csound->InitError(csound, "%s",
                               Str("repluck: delay line too large"));
    railBytes = (size_t)rail_len * sizeof(MYFLT);

                                /* Create upper rail */
    if (p->upper.auxp == NULL) {/* get newspace    */
      csound->AuxAlloc(csound, sizeof(DelayLine),&p->upper);
    }
    upper_rail = (DelayLine*)p->upper.auxp;
    upper_rail->length = rail_len;
    csound->AuxAlloc(csound, railBytes, &p->up_data);
    upper_rail->data = (MYFLT*)p->up_data.auxp;
    upper_rail->pointer = upper_rail->data;
    upper_rail->end = upper_rail->data + rail_len - 1;

                                /* Create lower rail */
    if (p->lower.auxp == NULL) {/* get newspace    */
      csound->AuxAlloc(csound, sizeof(DelayLine),&p->lower);
    }
    lower_rail = (DelayLine*)p->lower.auxp;
    lower_rail->length = rail_len;
    csound->AuxAlloc(csound, railBytes, &p->down_data);
    lower_rail->data = (MYFLT*)p->down_data.auxp;
    lower_rail->pointer = lower_rail->data;
    lower_rail->end = lower_rail->data + rail_len - 1;

    /* AuxAlloc clears both rails when there is no initial pluck. */
    if (LIKELY(plk != FL(0.0))) {
      /* Keep the peak between the two fixed ends of the string. */
      if (UNLIKELY(pickpt < 1)) pickpt = 1;
      if (UNLIKELY(pickpt > rail_len - 2)) pickpt = rail_len - 2;
      upslope = FL(1.0)/(MYFLT)pickpt;
      downslope = FL(1.0)/(MYFLT)(rail_len - pickpt - 1);
      for (i = 0; i < pickpt; i++) {
        MYFLT value = FL(0.5) * upslope * i;
        upper_rail->data[i] = value;
        lower_rail->data[i] = value;
      }
      for (i = pickpt; i < rail_len; i++) {
        MYFLT value = FL(0.5) * downslope * (rail_len - 1 - i);
        upper_rail->data[i] = value;
        lower_rail->data[i] = value;
      }
    }
                                /* Copy data into structure */
    p->state = FL(0.0);         /* filter memory */
    p->rail_len = rail_len;
    p->scale = scale;
    return OK;
} /* end wgpset(p) */

/* Offsets are in [0, length], so one subtraction wraps the index. */
#define WG_DELAY_INDEX(result, line, position)                           \
  do {                                                                   \
    uint32_t wg_length_ = (uint32_t)(line)->length;                      \
    uint32_t wg_index_ = (uint32_t)((line)->pointer - (line)->data) +    \
                         (uint32_t)(position);                           \
    if (wg_index_ >= wg_length_) wg_index_ -= wg_length_;                \
    (result) = wg_index_;                                                \
  } while (0)

#define OVERCNT (256)

static int32_t wgpluck(CSOUND *csound, WGPLUCK2 *p)
{
    MYFLT   *ar, *ain;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   yp0,ym0,ypM,ymM;
    DelayLine   *upper_rail;
    DelayLine   *lower_rail;
    int32_t     pickup, pickfrac;
    int32_t     i;
    int32_t     scale;
    uint32_t    upperIndex, lowerIndex;
    MYFLT   state = p->state;
    MYFLT   reflect = *p->reflect;
    MYFLT   amplitude = *p->xamp;
    MYFLT   excitationScale = amplitude != FL(0.0) ?
      FL(0.5) / amplitude : FL(0.0);
    MYFLT   pickupPosition = *p->pickup;

    if (UNLIKELY(!(reflect > FL(0.0) && reflect < FL(1.0)))) {
      csound->Warning(csound, Str("Reflection invalid (%f)\n"), reflect);
      reflect = FL(0.5);
    }
    ar         = p->ar;
    ain        = p->ain;
    scale      = p->scale;
    reflect    = FL(1.0) - (FL(1.0) - reflect)/(MYFLT)scale;
    upper_rail = (DelayLine*)p->upper.auxp;
    lower_rail = (DelayLine*)p->lower.auxp;
    /* fractional delays */
    if (UNLIKELY(!(pickupPosition >= FL(0.0) &&
                   pickupPosition <= FL(1.0)))) {
      csound->Warning(csound, Str("Pickup out of range (%f)\n"),
                      pickupPosition);
      pickupPosition = FL(0.5);
    }
    {
      double scaledPickup = (double)pickupPosition * p->rail_len;
      pickup = (int32_t)scaledPickup;
      pickfrac = (int32_t)((scaledPickup - pickup) * OVERCNT);
    }

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT s, s1;
      WG_DELAY_INDEX(upperIndex, upper_rail, pickup);
      WG_DELAY_INDEX(lowerIndex, lower_rail, pickup);
      s = upper_rail->data[upperIndex] + lower_rail->data[lowerIndex];
      if (UNLIKELY(++upperIndex == (uint32_t)upper_rail->length))
        upperIndex = 0;
      if (UNLIKELY(++lowerIndex == (uint32_t)lower_rail->length))
        lowerIndex = 0;
      s1 = upper_rail->data[upperIndex] + lower_rail->data[lowerIndex];
      ar[n] = s + (s1 - s)*(MYFLT)pickfrac/(MYFLT)OVERCNT; /* Fractional delay */
      if (ain != NULL && amplitude != FL(0.0)) {
        MYFLT excitation = ain[n] * excitationScale;
        WG_DELAY_INDEX(lowerIndex, lower_rail, 1);
        lower_rail->data[lowerIndex] += excitation;
        WG_DELAY_INDEX(upperIndex, upper_rail, 1);
        upper_rail->data[upperIndex] += excitation;
      }
      ar[n] *= amplitude;
      for (i=0; i<scale; i++) { /* Loop for precision figure */
        WG_DELAY_INDEX(lowerIndex, lower_rail, 1);
        ym0 = lower_rail->data[lowerIndex]; /* Sample traveling into "bridge" */
        WG_DELAY_INDEX(upperIndex, upper_rail, upper_rail->length - 2);
        ypM = upper_rail->data[upperIndex]; /* Sample traveling to "nut" */
        ymM = -ypM;             /* Inverting reflection at rigid nut */
                                /* reflection at yielding bridge */
                                /* Implement a one-pole lowpass with
                                   feedback coefficient from input */
        state = (state * reflect) + ym0 * (FL(1.0) - reflect);
        yp0 = - state;          /* String state update */
                                /* Decrement pointer and then update */
        {
          MYFLT *ptr = upper_rail->pointer;
          if (UNLIKELY(ptr == upper_rail->data))
            ptr = upper_rail->end;
          else
            ptr--;
          *ptr = yp0;
          upper_rail->pointer = ptr;
        }
                                /* Update and then increment pointer */
        {
          MYFLT *ptr = lower_rail->pointer;
          *ptr = ymM;
          if (UNLIKELY(ptr == lower_rail->end))
            ptr = lower_rail->data;
          else
            ptr++;
          lower_rail->pointer = ptr;
        }
      }
    }
    p->state = state;           /* Remember last state sample */

    return OK;
} /* end wgpluck(p) */

/*******************************************************/
/* streson.c : string resonator opcode                 */
/*             takes one input and passes it through   */
/*             emulates the resonance                  */
/*             of a string tuned to a kfun fundamental */
/*          Victor Lazzarini, 1998                     */
/*******************************************************/

static int32_t stresonset(CSOUND *csound, STRES *p)
{
    p->size = (int32_t) (CS_ESR/20);   /* size of delay line */
    csound->AuxAlloc(csound, p->size*sizeof(MYFLT), &p->aux);
    p->Cdelay = (MYFLT*) p->aux.auxp; /* delay line */
    p->LPdelay = p->APdelay = FL(0.0); /* reset the All-pass and Low-pass delays */
    p->wpointer = p->rpointer = 0; /* reset the read/write pointers */
    memset(p->Cdelay, '\0', p->size*sizeof(MYFLT));
    return OK;
}

static int32_t streson(CSOUND *csound, STRES *p)
{
    MYFLT *out = p->result;
    MYFLT *in = p->ainput;
    MYFLT g = *p->ifdbgain;
    MYFLT freq;
    double a, s, w, sample, tdelay, fracdelay;
    int32_t delay;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t rp = p->rpointer, wp = p->wpointer;
    int32_t size = p->size;
    MYFLT       APdelay = p->APdelay;
    MYFLT       LPdelay = p->LPdelay;
    int32_t         vdt;

    freq = *p->afr;
    if (UNLIKELY(!(freq >= FL(20.0))))
      freq = FL(20.0);                 /* lowest freq is 20 Hz */
    tdelay = CS_ESR/freq;
    delay = (int32_t) (tdelay - 0.5); /* comb delay */
    fracdelay = tdelay - (delay + 0.5); /* fractional delay */
    vdt = size - delay;       /* set the var delay */
    a = (1.0-fracdelay)/(1.0+fracdelay);   /* set the all-pass gain */
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      /* GetSample(p); */
      MYFLT tmpo;
      rp = (vdt + wp);
      if (UNLIKELY(rp >= size)) rp -= size;
      tmpo = p->Cdelay[rp];
      w = in[n] + tmpo;
      s = (LPdelay + w)*0.5;
      LPdelay = w;
      out[n] = sample = APdelay + s*a;
      APdelay = s - (sample*a);
      p->Cdelay[wp] = sample*g;
      wp++;
      if (UNLIKELY(wp == size)) wp=0;
    }
    p->rpointer = rp; p->wpointer = wp;
    p->LPdelay = LPdelay; p->APdelay = APdelay;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
   { "repluck", S(WGPLUCK2), 0,  "a",  "ikikka",(SUBR)wgpsetin, (SUBR)wgpluck},
   { "wgpluck2",S(WGPLUCK2), 0,  "a",  "ikikk", (SUBR)wgpset, (SUBR)wgpluck},
   { "streson", S(STRES),    0,  "a",  "akk",  (SUBR)stresonset, (SUBR)streson}
};

int32_t repluck_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
