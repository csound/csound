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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/***************************************************************\
*   repluck.c                                                  *
*   Various waveguide instruments                              *
*   3 March 1996 John ffitch                                   *
\***************************************************************/

#include "stdopcod.h"
#include "repluck.h"

static int wgpsetin(CSOUND *, WGPLUCK2 *);

static int wgpset(CSOUND *csound, WGPLUCK2 *p)
{
    p->ain = NULL;
    wgpsetin(csound,p);
    return OK;
}

static int wgpsetin(CSOUND *csound, WGPLUCK2 *p)
{
    int     npts;
    int     pickpt;
    int     rail_len;
    MYFLT   upslope;
    MYFLT   downslope;
    MYFLT   *initial_shape;
    int     i;
    int     scale = 1;
    DelayLine   *upper_rail;
    DelayLine   *lower_rail;
    MYFLT   plk = *p->plk;
                                /* Initialize variables....*/
    npts = (int)(CS_ESR / *p->icps);/* Length of full delay */
    while (npts < 512) {        /* Minimum rail length is 256 */
      npts += (int)(CS_ESR / *p->icps);
      scale++;
    }
    rail_len = npts/2/* + 1*/;      /* but only need half length */
    if (plk >= FL(1.0) || plk <= FL(0.0)) {
      plk = (p->ain ? FL(0.0) : FL(0.5));
    }
    pickpt = (int)(rail_len * plk);

                                /* Create upper rail */
    if (p->upper.auxp == NULL) {/* get newspace    */
      csound->AuxAlloc(csound, sizeof(DelayLine),&p->upper);
    }
    upper_rail = (DelayLine*)p->upper.auxp;
    upper_rail->length = rail_len;
    if (rail_len > 0) {
      csound->AuxAlloc(csound, rail_len*sizeof(MYFLT),&p->up_data);
      upper_rail->data = (MYFLT*)p->up_data.auxp;
    }
    //    else upper_rail->data = NULL;
    upper_rail->pointer = upper_rail->data;
    upper_rail->end = upper_rail->data + rail_len - 1;

                                /* Create lower rail */
    if (p->lower.auxp == NULL) {/* get newspace    */
      csound->AuxAlloc(csound, sizeof(DelayLine),&p->lower);
    }
    lower_rail = (DelayLine*)p->lower.auxp;
    lower_rail->length = rail_len;
    //if (rail_len > 0) {  Always true
      csound->AuxAlloc(csound, rail_len*sizeof(MYFLT),&p->down_data);
      lower_rail->data = (MYFLT*)p->down_data.auxp;
      //}
    //else lower_rail->data = NULL;
    lower_rail->pointer = lower_rail->data;
    lower_rail->end = lower_rail->data + rail_len - 1;

                                /* Set initial shape */
    if (plk != FL(0.0)) {
      initial_shape = (MYFLT*) malloc(rail_len*sizeof(MYFLT));
      if (pickpt < 1) pickpt = 1;       /* Place for pluck, in range (0,1.0) */
      upslope = FL(1.0)/(MYFLT)pickpt; /* Slightly faster to precalculate */
      downslope = FL(1.0)/(MYFLT)(rail_len - pickpt - 1);
      for (i = 0; i < pickpt; i++)
        initial_shape[i] = upslope * i;
      for (i = pickpt; i < rail_len; i++)
        initial_shape[i] = downslope * (rail_len - 1 - i);
      for (i=0; i<rail_len; i++)
        upper_rail->data[i] = FL(0.5) * initial_shape[i];
      for (i=0; i<rail_len; i++)
        lower_rail->data[i] = FL(0.5) * initial_shape[i];
      free(initial_shape);
    }
    else {
      memset(upper_rail->data, 0, rail_len*sizeof(MYFLT));
      memset(lower_rail->data, 0, rail_len*sizeof(MYFLT));
      /* for (i = 0; i < rail_len; i++) */
      /*   upper_rail->data[i] = lower_rail->data[i] = FL(0.0); */
    }
                                /* Copy data into structure */
    p->state = FL(0.0);         /* filter memory */
    p->rail_len = rail_len;
    p->scale = scale;
    return OK;
} /* end wgpset(p) */

                                /* Access a delay line with wrapping */
static MYFLT* locate(DelayLine *dl, int position)
{
    MYFLT *outloc = dl->pointer + position;
    while (outloc < dl->data)
      outloc += dl->length;
    while (outloc > dl->end)
      outloc -= dl->length;
    return outloc;
}

static MYFLT getvalue(DelayLine *dl, int position)
{
    MYFLT *outloc = dl->pointer + position;
    while (outloc < dl->data)
      outloc += dl->length;
    while (outloc > dl->end)
      outloc -= dl->length;
    return *outloc;
}

#define OVERCNT (256)
#define OVERSHT (8)
#define OVERMSK (0xFF)

static int wgpluck(CSOUND *csound, WGPLUCK2 *p)
{
    MYFLT   *ar, *ain;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   yp0,ym0,ypM,ymM;
    DelayLine   *upper_rail;
    DelayLine   *lower_rail;
    int     pickup, pickfrac;
    int     i;
    int     scale;
    MYFLT   state = p->state;
    MYFLT   reflect = *p->reflect;

    if (UNLIKELY(reflect <= FL(0.0) || reflect >= FL(1.0))) {
      csound->Warning(csound, Str("Reflection invalid (%f)\n"), reflect);
      reflect = FL(0.5);
    }
    ar         = p->ar;
    ain        = p->ain;
    scale      = p->scale;
    reflect    = FL(1.0) - (FL(1.0) - reflect)/(MYFLT)scale; /* For over sapling */
    upper_rail = (DelayLine*)p->upper.auxp;
    lower_rail = (DelayLine*)p->lower.auxp;
    /* fractional delays */
    pickup     = (int)((MYFLT)OVERCNT * *(p->pickup) * p->rail_len);
    pickfrac   = pickup & OVERMSK;
    pickup     = pickup>>OVERSHT;
    if (UNLIKELY(pickup<0 || pickup > p->rail_len)) {
      csound->Warning(csound, Str("Pickup out of range (%f)\n"), *p->pickup);
      pickup   = p->rail_len * (OVERCNT/2);
      pickfrac = pickup & OVERMSK;
      pickup   = pickup>>OVERSHT;
    }

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT s, s1;
      s = getvalue(upper_rail, pickup) + getvalue(lower_rail, pickup);
      s1 = getvalue(upper_rail, pickup+1) + getvalue(lower_rail, pickup+1);
      ar[n] = s + (s1 - s)*(MYFLT)pickfrac/(MYFLT)OVERCNT; /* Fractional delay */
      if (ain != NULL) {        /* Excite the string from input */
        MYFLT *loc = locate(lower_rail,1);
        *loc += (FL(0.5)* *ain)/(*p->xamp);
        loc = locate(upper_rail,1);
        *loc += (FL(0.5)* *ain++)/(*p->xamp);
      }
      ar[n] *= *p->xamp;        /* and scale */
      for (i=0; i<scale; i++) { /* Loop for precision figure */
        ym0 = getvalue(lower_rail, 1); /* Sample traveling into "bridge" */
        ypM = getvalue(upper_rail, upper_rail->length - 2); /* Sample to "nut" */
        ymM = -ypM;             /* Inverting reflection at rigid nut */
                                /* reflection at yielding bridge */
                                /* Implement a one-pole lowpass with
                                   feedback coefficient from input */
        state = (state * reflect) + ym0 * (FL(1.0) - reflect);
        yp0 = - state;          /* String state update */
                                /* Decrement pointer and then update */
        {
          MYFLT *ptr = upper_rail->pointer;
          ptr--;
          if (UNLIKELY(ptr < upper_rail->data))
            ptr = upper_rail->end;
          *ptr = yp0;
          upper_rail->pointer = ptr;
        }
                                /* Update and then increment pointer */
        {
          MYFLT *ptr = lower_rail->pointer;
          *ptr = ymM;
          ptr++;
          if (UNLIKELY(ptr > lower_rail->end))
            ptr = lower_rail->data;
          lower_rail->pointer = ptr;
        }
      }
    };
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

static int stresonset(CSOUND *csound, STRES *p)
{
    int n;
    p->size = (int) (CS_ESR/20);   /* size of delay line */
    csound->AuxAlloc(csound, p->size*sizeof(MYFLT), &p->aux);
    p->Cdelay = (MYFLT*) p->aux.auxp; /* delay line */
    p->LPdelay = p->APdelay = FL(0.0); /* reset the All-pass and Low-pass delays */
    p->wpointer = p->rpointer = 0; /* reset the read/write pointers */
    for (n = 0; n < p->size; n++)
      p->Cdelay[n] = FL(0.0);
    return OK;
}

static int streson(CSOUND *csound, STRES *p)
{
    MYFLT *out = p->result;
    MYFLT *in = p->ainput;
    MYFLT g = *p->ifdbgain;
    MYFLT freq;
    double a, s, w, sample, tdelay, fracdelay;
    int delay;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int rp = p->rpointer, wp = p->wpointer;
    int size = p->size;
    MYFLT       APdelay = p->APdelay;
    MYFLT       LPdelay = p->LPdelay;
    int         vdt;

    freq = *p->afr;
    if (freq < FL(20.0)) freq = FL(20.0);   /* lowest freq is 20 Hz */
    tdelay = CS_ESR/freq;
    delay = (int) (tdelay - 0.5); /* comb delay */
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
      if (rp >= size) rp -= size;
      tmpo = p->Cdelay[rp];
      w = in[n] + tmpo;
      s = (LPdelay + w)*0.5;
      LPdelay = w;
      out[n] = sample = APdelay + s*a;
      APdelay = s - (sample*a);
      p->Cdelay[wp] = sample*g;
      wp++;
      if (wp == size) wp=0;
    }
    p->rpointer = rp; p->wpointer = wp;
    p->LPdelay = LPdelay; p->APdelay = APdelay;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "repluck", S(WGPLUCK2), 0, 5, "a",  "ikikka",(SUBR)wgpsetin, NULL, (SUBR)wgpluck},
{ "wgpluck2",S(WGPLUCK2), 0, 5, "a",  "ikikk", (SUBR)wgpset,   NULL, (SUBR)wgpluck},
{ "streson", S(STRES),    0, 5, "a",  "akk",  (SUBR)stresonset, NULL, (SUBR)streson}
};

int repluck_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

