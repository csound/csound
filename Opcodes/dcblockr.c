/*
    dcblockr.c:

    Copyright (C) 1998 John ffitch, 2008 V Lazzarini

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

/*******************************************/
/*  DC Blocking Filter                     */
/*  by Perry R. Cook, 1995-96              */
/*  This guy is very helpful in, uh,       */
/*  blocking DC.  Needed because a simple  */
/*  low-pass reflection filter allows DC   */
/*  to build up inside recursive           */
/*  structures.                            */
/*******************************************/

#include "stdopcod.h"
#include "dcblockr.h"

static int32_t dcblockrset(CSOUND *csound, DCBlocker* p)
{
    IGN(csound);
    p->outputs = 0.0;
    p->inputs = 0.0;
    p->gain = (double)*p->gg;
    if (p->gain == 0.0 || p->gain>=1.0 || p->gain<=-1.0)
      p->gain = 0.99;
    return OK;
}

static int32_t dcblockr(CSOUND *csound, DCBlocker* p)
{
    IGN(csound);
    MYFLT       *ar = p->ar;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    double      gain = p->gain;
    double      outputs = p->outputs;
    double      inputs = p->inputs;
    MYFLT       *samp = p->in;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      double sample = (double)samp[n];
      outputs = sample - inputs + (gain * outputs);
      inputs = sample;
      ar[n] = (MYFLT)outputs;
    }
    p->outputs = outputs;
    p->inputs = inputs;
    return OK;
}

/*******************************************/
/*  DC Blocking Filter                     */
/*  Improved DC attenuation                */
/*  V Lazzarini                            */
/*******************************************/

typedef struct _dcblk2 {
  OPDS    h;
  MYFLT   *output;
  MYFLT   *input, *order, *iskip;
  AUXCH   delay1;
  AUXCH   iirdelay1, iirdelay2, iirdelay3, iirdelay4;
  double  ydels[4];
  size_t  dp1, dp2;
  size_t  del1size, iirdelsize;
  double  scaler;
} DCBlock2;


static int32_t dcblock2set(CSOUND *csound, DCBlock2* p)
{
    double order_value = (double)*p->order;
    size_t del1size, iirdelsize, del1bytes, iirdelbytes;
    int32_t clear_state, order;

    if (UNLIKELY(!(order_value >= (double)INT32_MIN &&
                   order_value <= (double)INT32_MAX)))
      return csound->InitError(csound, Str("dcblock2: invalid order %f"),
                               *p->order);
    order = (int32_t)order_value;
    if (order == 0) order = 128;
    else if (order < 4) order = 4;

    iirdelsize = (size_t)order;
    if (UNLIKELY(iirdelsize - 1 > SIZE_MAX / (2 * sizeof(double))))
      return csound->InitError(csound, "%s",
                               Str("dcblock2: order is too large"));
    del1size = (iirdelsize - 1) * 2;
    del1bytes = del1size * sizeof(double);
    iirdelbytes = iirdelsize * sizeof(double);
    /* State can be reused only when the effective order is unchanged. */
    clear_state = !*p->iskip || p->iirdelsize != iirdelsize;

    if (p->delay1.auxp == NULL ||
        p->delay1.size < del1bytes)
      csound->AuxAlloc(csound, del1bytes, &p->delay1);

    if (p->iirdelay1.auxp == NULL ||
        p->iirdelay1.size < iirdelbytes)
      csound->AuxAlloc(csound, iirdelbytes, &p->iirdelay1);

    if (p->iirdelay2.auxp == NULL ||
        p->iirdelay2.size < iirdelbytes)
      csound->AuxAlloc(csound, iirdelbytes, &p->iirdelay2);

    if (p->iirdelay3.auxp == NULL ||
        p->iirdelay3.size < iirdelbytes)
      csound->AuxAlloc(csound, iirdelbytes, &p->iirdelay3);

    if (p->iirdelay4.auxp == NULL ||
        p->iirdelay4.size < iirdelbytes)
      csound->AuxAlloc(csound, iirdelbytes, &p->iirdelay4);

    p->del1size = del1size;
    p->iirdelsize = iirdelsize;
    p->scaler = 1.0 / (double)order;
    if (clear_state) {
      memset(p->ydels, 0, sizeof(p->ydels));
      memset(p->delay1.auxp, 0, del1bytes);
      memset(p->iirdelay1.auxp, 0, iirdelbytes);
      memset(p->iirdelay2.auxp, 0, iirdelbytes);
      memset(p->iirdelay3.auxp, 0, iirdelbytes);
      memset(p->iirdelay4.auxp, 0, iirdelbytes);
      p->dp1 = 0; p->dp2 = 0;
    }
    return OK;
}

static int32_t dcblock2(CSOUND *csound, DCBlock2* p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT    *in = p->input;
    MYFLT    *out = p->output;
    double   *del1 = (double *)p->delay1.auxp;
    double   *iirdel[4],x1,x2,y,del;
    double   *ydels = p->ydels;
    double   scale = p->scaler;
    size_t       p1 = p->dp1;
    size_t       p2 = p->dp2;
    int32_t      j;
    size_t       del1size = p->del1size;
    size_t       iirdelsize = p->iirdelsize;

    iirdel[0] = (double *) p->iirdelay1.auxp;
    iirdel[1] = (double *) p->iirdelay2.auxp;
    iirdel[2] = (double *) p->iirdelay3.auxp;
    iirdel[3] = (double *) p->iirdelay4.auxp;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset; i < nsmps; i++) {

      /* long delay */
      del = del1[p1];
      del1[p1] = x1 = (double)in[i];

      /* IIR cascade */
      for (j=0; j < 4; j++) {
        x2 = iirdel[j][p2];
        iirdel[j][p2] = x1;
        y = x1 - x2 + ydels[j];
        ydels[j] = y;
        x1 = y*scale;
      }
      out[i] = (MYFLT)(del - x1);

      p1 = (p1 == del1size - 1 ? 0 : p1 + 1);
      p2 = (p2 == iirdelsize - 1 ? 0 : p2 + 1);
    }

    p->dp1 = p1; p->dp2 = p2;
    return OK;
}




#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "dcblock", S(DCBlocker), 0,  "a", "ao",
                                   (SUBR)dcblockrset, (SUBR)dcblockr},
  { "dcblock2", S(DCBlock2), 0, "a", "aoo",
                                   (SUBR)dcblock2set, (SUBR)dcblock2}
};

int32_t dcblockr_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
