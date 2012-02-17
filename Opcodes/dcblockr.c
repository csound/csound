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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

#include "csdl.h"
#include "dcblockr.h"

static int dcblockrset(CSOUND *csound, DCBlocker* p)
{
    p->outputs = 0.0;
    p->inputs = 0.0;
    p->gain = (double)*p->gg;
    if (p->gain == 0.0 || p->gain>=1.0 || p->gain<=-1.0)
      p->gain = 0.99;
    return OK;
}

static int dcblockr(CSOUND *csound, DCBlocker* p)
{
    MYFLT       *ar = p->ar;
    int         n, nsmps = csound->ksmps;
    double      gain = p->gain;
    double      outputs = p->outputs;
    double      inputs = p->inputs;
    MYFLT       *samp = p->in;

    for (n=0; n<nsmps; n++) {
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
  OPDS h;
  MYFLT *output;
  MYFLT *input, *order, *iskip;
  AUXCH delay1;
  AUXCH iirdelay1, iirdelay2, iirdelay3, iirdelay4;
  double ydels[4];
  int dp1,dp2;
  double scaler;
} DCBlock2;


static int dcblock2set(CSOUND *csound, DCBlock2* p)
{
    int order = (int) *p->order;
    if (order == 0) order = 128;
    else if (order < 4) order = 4;

    if (p->delay1.auxp == NULL ||
        p->delay1.size < (order-1)*2*sizeof(double))
      csound->AuxAlloc(csound, (order-1)*2*sizeof(double),
                       &p->delay1);

    if (p->iirdelay1.auxp == NULL ||
        p->iirdelay1.size < (order)*sizeof(double))
      csound->AuxAlloc(csound,
                       (order)*sizeof(double), &p->iirdelay1);

    if (p->iirdelay2.auxp == NULL ||
        p->iirdelay2.size < (order)*sizeof(double))
      csound->AuxAlloc(csound,
                       (order)*sizeof(double), &p->iirdelay2);

    if (p->iirdelay3.auxp == NULL ||
        p->iirdelay3.size < (order)*sizeof(double))
      csound->AuxAlloc(csound,
                       (order)*sizeof(double), &p->iirdelay3);

    if (p->iirdelay4.auxp == NULL ||
        p->iirdelay4.size < (order)*sizeof(double))
      csound->AuxAlloc(csound,
                       (order)*sizeof(double), &p->iirdelay4);

    p->scaler = 1.0/order;
    if (!*p->iskip) {
      memset(p->ydels, 0, 4*sizeof(double));
      /* p->ydels[0] = 0.0;   p->ydels[1] = 0.0; */
      /* p->ydels[2] = 0.0;   p->ydels[3] = 0.0; */
      memset(p->delay1.auxp, 0, sizeof(double)*(order-1)*2);
      memset(p->iirdelay1.auxp, 0, sizeof(double)*(order));
      memset(p->iirdelay2.auxp, 0, sizeof(double)*(order));
      memset(p->iirdelay3.auxp, 0, sizeof(double)*(order));
      memset(p->iirdelay4.auxp, 0, sizeof(double)*(order));
      p->dp1 = 0; p->dp2 = 0;
    }
    return OK;
}

static int dcblock2(CSOUND *csound, DCBlock2* p)
{
    MYFLT *in = p->input;
    MYFLT *out = p->output;
    double *del1 = (double *)p->delay1.auxp;
    double *iirdel[4],x1,x2,y,del;
    double  *ydels = p->ydels;
    double scale = p->scaler;
    int p1 = p->dp1;
    int p2 = p->dp2;
    int i,j,del1size, iirdelsize, nsmps = csound->ksmps;

    iirdel[0] = (double *) p->iirdelay1.auxp;
    iirdel[1] = (double *) p->iirdelay2.auxp;
    iirdel[2] = (double *) p->iirdelay3.auxp;
    iirdel[3] = (double *) p->iirdelay4.auxp;

    del1size = p->delay1.size/sizeof(double);
    iirdelsize = p->iirdelay1.size/sizeof(double);

    for (i=0; i < nsmps; i++) {

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

      p1 = (p1 == del1size-1 ? 0 : p1 + 1);
      p2 = (p2 == iirdelsize-1 ? 0 : p2 + 1);
    }

    p->dp1 = p1; p->dp2 = p2;
     return OK;
}




#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "dcblock", S(DCBlocker), 5, "a", "ao", (SUBR)dcblockrset, NULL, (SUBR)dcblockr},
  { "dcblock2", S(DCBlock2), 5, "a", "aoo", (SUBR)dcblock2set, NULL, (SUBR)dcblock2}
};

int dcblockr_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

