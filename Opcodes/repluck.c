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

#include "csdl.h"
#include "repluck.h"

int wgpsetin(WGPLUCK2 *);

int wgpset(WGPLUCK2 *p)
{
    p->ain = NULL;
    wgpsetin(p);
    return OK;
}

int wgpsetin(WGPLUCK2 *p)
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
    npts = (int)(esr / *p->icps);/* Length of full delay */
    while (npts < 512) {        /* Minimum rail length is 256 */
      npts += (int)(esr / *p->icps);
      scale++;
    }
    rail_len = npts/2/* + 1*/;      /* but only need half length */
    if (plk >= FL(1.0) || plk <= FL(0.0)) {
/*       printf("Pluck point %f invalid, using 0.5\n", plk); */
      plk = (p->ain ? FL(0.0) : FL(0.5));
    }
    pickpt = (int)(rail_len * plk);

                                /* Create upper rail */
    if (p->upper.auxp == NULL) {/* get newspace    */
      auxalloc(sizeof(DelayLine),&p->upper);
    }
    upper_rail = (DelayLine*)p->upper.auxp;
    upper_rail->length = rail_len;
    if (rail_len > 0) {
      auxalloc(rail_len*sizeof(MYFLT),&p->up_data);
      upper_rail->data = (MYFLT*)p->up_data.auxp;
    }
    else upper_rail->data = NULL;
    upper_rail->pointer = upper_rail->data;
    upper_rail->end = upper_rail->data + rail_len - 1;

                                /* Create lower rail */
    if (p->lower.auxp == NULL) {/* get newspace    */
      auxalloc(sizeof(DelayLine),&p->lower);
    }
    lower_rail = (DelayLine*)p->lower.auxp;
    lower_rail->length = rail_len;
    if (rail_len > 0) {
      auxalloc(rail_len*sizeof(MYFLT),&p->down_data);
      lower_rail->data = (MYFLT*)p->down_data.auxp;
    }
    else lower_rail->data = NULL;
    lower_rail->pointer = lower_rail->data;
    lower_rail->end = lower_rail->data + rail_len - 1;

                                /* Set initial shape */
    if (plk != 0.0) {
      initial_shape = (MYFLT*)mmalloc(rail_len*sizeof(MYFLT));
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
      mfree((char *)initial_shape);
    }
    else {
      for (i = 0; i < rail_len; i++)
        upper_rail->data[i] = lower_rail->data[i] = FL(0.0);
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

int wgpluck(WGPLUCK2 *p)
{
    MYFLT   *ar, *ain;
    int     nsmps;
    MYFLT   yp0,ym0,ypM,ymM;
    DelayLine   *upper_rail;
    DelayLine   *lower_rail;
    int     pickup, pickfrac;
    int         i;
    int         scale;
    MYFLT   state = p->state;
    MYFLT   reflect = *p->reflect;

    if (reflect <= FL(0.0) || reflect >= FL(1.0)) {
      printf(Str(X_443,"Reflection invalid (%f)\n"), reflect);
      reflect = FL(0.5);
    }
    ar   = p->ar;
    ain   = p->ain;
    scale = p->scale;
    reflect = FL(1.0) - (FL(1.0) - reflect)/(MYFLT)scale; /* For over sapling */
    nsmps = ksmps;              /* Number of points to calculate */
    upper_rail = (DelayLine*)p->upper.auxp;
    lower_rail = (DelayLine*)p->lower.auxp;
    pickup = (int)((MYFLT)OVERCNT * *(p->pickup) * p->rail_len); /* fract delays */
    pickfrac = pickup & OVERMSK;
    pickup = pickup>>OVERSHT;
    if (pickup<0 || pickup > p->rail_len) {
      printf(Str(X_423,"Pickup out of range (%f)\n"), p->pickup);
      pickup =  p->rail_len * (OVERCNT/2);
      pickfrac = pickup & OVERMSK;
      pickup = pickup>>OVERSHT;
    }
                                /* *** Start the loop .... *** */
    do {                        /* while (--nsmps) */
      MYFLT s, s1;
      s = getvalue(upper_rail, pickup) + getvalue(lower_rail, pickup);
      s1 = getvalue(upper_rail, pickup+1) + getvalue(lower_rail, pickup+1);
      *ar = s + (s1 - s)*(MYFLT)pickfrac/(MYFLT)OVERCNT; /* Fractional delay */
      if (ain != NULL) {        /* Excite the string from input */
        MYFLT *loc = locate(lower_rail,1);
        *loc += (FL(0.5)* *ain)/(*p->xamp);
        loc = locate(upper_rail,1);
        *loc += (FL(0.5)* *ain++)/(*p->xamp);
      }
      *ar++ *= *p->xamp;        /* Increment and scale */
      for (i=0; i<scale; i++) { /* Loop for precision figure */
        ym0 = getvalue(lower_rail, 1); /* Sample traveling into "bridge" */
        ypM = getvalue(upper_rail, upper_rail->length - 2); /* Sample to "nut" */
        ymM = -ypM;             /* Inverting reflection at rigid nut */
                                /* reflection at yielding bridge */
                                /* Implement a one-pole lowpass with feedback coefficient from input */
        state = (state * reflect) + ym0 * (FL(1.0) - reflect);
        yp0 = - state;          /* String state update */
                                /* Decrement pointer and then update */
        {
          MYFLT *ptr = upper_rail->pointer;
          ptr--;
          if (ptr < upper_rail->data)
            ptr = upper_rail->end;
          *ptr = yp0;
          upper_rail->pointer = ptr;
        }
                                /* Update and then increment pointer */
        {
          MYFLT *ptr = lower_rail->pointer;
          *ptr = ymM;
          ptr++;
          if (ptr > lower_rail->end)
            ptr = lower_rail->data;
          lower_rail->pointer = ptr;
        }
      }
    } while (--nsmps);
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

int stresonset(STRES *p)
{
    int n;
    p->size = (int) (esr/20);   /* size of delay line */
    auxalloc(p->size*sizeof(MYFLT), &p->aux);
    p->Cdelay = (MYFLT*) p->aux.auxp; /* delay line */
    p->LPdelay = p->APdelay = FL(0.0); /* reset the All-pass and Low-pass delays */
    p->wpointer = p->rpointer = 0; /* reset the read/write pointers */
    for (n = 0; n < p->size; n++)
      p->Cdelay[n] = FL(0.0);
    return OK;
}

int streson(STRES *p)
{
    MYFLT *out = p->result;
    MYFLT *in = p->ainput;
    MYFLT g = *p->ifdbgain;
    MYFLT freq, a, s, w, sample, tdelay, fracdelay;
    int delay, nn = ksmps;
    int rp = p->rpointer, wp = p->wpointer;
    int size = p->size;
    MYFLT       APdelay = p->APdelay;
    MYFLT       LPdelay = p->LPdelay;
    int         vdt = p->vdtime;

    freq = *p->afr;
    if (freq < 20) freq = FL(20.0);   /* lowest freq is 20 Hz */
    tdelay = esr/freq;
    delay = (int) (tdelay - FL(0.5)); /* comb delay */
    fracdelay = tdelay - (delay + FL(0.5)); /* fractional delay */
    p->vdtime = size - delay;       /* set the var delay */
    a = (1-fracdelay)/(1+fracdelay);   /* set the all-pass gain */
    do {
      /* GetSample(p); */
      MYFLT tmpo;
      tmpo = p->Cdelay[rp];
      rp = (vdt + wp)%size;
      w = *in++ + tmpo;
      s = LPdelay*FL(0.5) + w*FL(0.5);
      LPdelay = w;
      *out++ = sample = APdelay + s*a;
      APdelay = s - (sample*a);
      /* PutSample(sample*g, p); */
      p->Cdelay[wp] = sample*g;
      wp++;
      if (wp == size) wp=0;
    } while (--nn);
    p->rpointer = rp; p->wpointer = wp;
    p->LPdelay = LPdelay; p->APdelay = APdelay;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "repluck", S(WGPLUCK2), 5, "a",  "ikikka",(SUBR)wgpsetin, NULL, (SUBR)wgpluck},
{ "wgpluck2",S(WGPLUCK2), 5, "a",  "ikikk", (SUBR)wgpset,   NULL, (SUBR)wgpluck},
{ "streson", S(STRES),    5, "a",  "aki",  (SUBR)stresonset, NULL, (SUBR)streson}
};

long opcode_size(void)
{
    return sizeof(localops);
}

OENTRY *opcode_init(ENVIRON *xx)
{
    return localops;
}
