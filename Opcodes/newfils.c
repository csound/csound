/*
    newfils.c:
    filter opcodes

    (c) Victor Lazzarini, 2004

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

#include "csdl.h"

#include "newfils.h"
#include <math.h>

static int moogladder_init(CSOUND *csound,moogladder *p)
{
    int i;
    if (*p->istor == FL(0.0)) {
      for (i = 0; i < 6; i++)
        p->delay[i] = 0.0;
      for (i = 0; i < 3; i++)
        p->tanhstg[i] = 0.0;
    }
    return OK;
}

static int moogladder_process(CSOUND *csound,moogladder *p)
{
    MYFLT   *out = p->out;
    MYFLT   *in = p->in;
    MYFLT   freq = *p->freq;
    MYFLT   res = *p->res;
    double  res4;
    double  *delay = p->delay;
    double  *tanhstg = p->tanhstg;
    double  stg[4], input;
    double  f, fc, fc2, fc3, fcr, acr, tune;
    double  thermal = (1.0 / 40000.0);  /* transistor thermal voltage  */
    int     j, k, i;

    if (res < 0) res = 0;

    /* sr is half the actual filter sampling rate  */
    fc =  (double)(freq/csound->esr);
    f  =  fc/2;
    fc2 = fc*fc;
    fc3 = fc2*fc;
    /* frequency & amplitude correction  */
    fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
    acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / thermal;   /* filter tuning  */
    res4 = 4.0*(double)res*acr;

    for (i = 0; i < csound->ksmps; i++) {
      /* oversampling  */
      for (j = 0; j < 2; j++) {
        /* filter stages  */
        for (k = 0; k < 4; k++) {
          if (k) {
            input = stg[k-1];
            stg[k] = delay[k]
                     + tune*((tanhstg[k-1] = tanh(input*thermal))
                             - (k != 3 ? tanhstg[k] : tanh(delay[k]*thermal)));
          }
          else {
            input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
            stg[k] = delay[k] + tune*(tanh(input*thermal) - tanhstg[k]);
          }
          delay[k] = stg[k];
        }
        /* 1/2-sample delay for phase compensation  */
        delay[5] = (stg[3] + delay[4])*0.5;
        delay[4] = stg[3];
      }
      out[i] = (MYFLT) delay[5];
    }
    return OK;
}

static int statevar_init(CSOUND *csound,statevar *p)
{
    if (*p->istor==FL(0.0)) {
      p->bpd = p->lpd = p->lp = 0.0;
    }
    if (*p->osamp<=FL(0.0)) p->ostimes = 3;
    else p->ostimes = (int) *p->osamp;
    return OK;
}

static int statevar_process(CSOUND *csound,statevar *p)
{
    MYFLT  *outhp = p->outhp;
    MYFLT  *outlp = p->outlp;
    MYFLT  *outbp = p->outbp;
    MYFLT  *outbr = p->outbr;
    MYFLT  *in = p->in;
    MYFLT  freq = *p->freq;
    MYFLT  res  = *p->res;
    double  lpd = p->lpd;
    double  bpd = p->bpd;
    double  lp  = p->lp, hp = 0.0, bp = 0.0, br = 0.0;
    double  f,q,lim;
    int ostimes = p->ostimes,i,j;

    f = 2.0*sin(freq*(double)csound->pidsr/ostimes);
    q = 1.0/res;
    lim = 0.1*((2.0 - f) / 2.)*(1./ostimes);
    /* csound->Message(csound, "lim: %f, q: %f \n", lim, q); */

    if (q < lim) q = lim;

    for (i=0; i<csound->ksmps;i++) {
      for (j=0;j<ostimes;j++) {

        hp = in[i] - q*bpd - lp;
        bp = hp*f + bpd;
        lp = bpd*f + lpd;
        br = lp + hp;
        bpd = bp;
        lpd = lp;
      }

      outhp[i] = (MYFLT) hp;
      outlp[i] = (MYFLT) lp;
      outbp[i] = (MYFLT) bp;
      outbr[i] = (MYFLT) br;

    }
    p->bpd = bpd;
    p->lpd = lpd;
    p->lp = lp;

    return OK;
}

static int fofilter_init(CSOUND *csound,fofilter *p)
{
    int i;
    if (*p->istor==FL(0.0)) {
      for (i=0;i<4; i++)
        p->delay[i] = 0.0;
    }
    return OK;
}

static int fofilter_process(CSOUND *csound,fofilter *p)
{
    MYFLT  *out = p->out;
    MYFLT  *in = p->in;
    MYFLT  freq = *p->freq;
    MYFLT  ris = *p->ris;
    MYFLT  dec = *p->dec;
    double  *delay = p->delay,ang,fsc,rad1,rad2;
    double  w1,y1,w2,y2;
    int i;

    ang = (double)csound->tpidsr*freq;         /* pole angle */
    fsc = sin(ang) - 3.0;                      /* freq scl   */
    rad1 =  pow(10.0, fsc/(dec*csound->esr));  /* filter radii */
    rad2 =  pow(10.0, fsc/(ris*csound->esr));

    for (i=0;i<csound->ksmps;i++) {

      w1  = in[i] + 2.0*rad1*cos(ang)*delay[0] - rad1*rad1*delay[1];
      y1 =  w1 - delay[1];
      delay[1] = delay[0];
      delay[0] = w1;

      w2  = in[i] + 2.0*rad2*cos(ang)*delay[2] - rad2*rad2*delay[3];
      y2 =  w2 - delay[3];
      delay[3] = delay[2];
      delay[2] = w2;

      out[i] = (MYFLT) (y1 - y2);

    }
    return OK;
}

static OENTRY localops[] = {
  {"moogladder", sizeof(moogladder), 5, "a", "akkp",
                    (SUBR) moogladder_init, NULL, (SUBR) moogladder_process },
  {"statevar", sizeof(statevar), 5, "aaaa", "akkop",
                    (SUBR) statevar_init, NULL, (SUBR) statevar_process     },
  {"fofilter", sizeof(fofilter), 5, "a", "akkkp",
                    (SUBR) fofilter_init, NULL, (SUBR) fofilter_process     }
};

int newfils_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

