/*
    buchla.c:

    Copyright (C) 2014 by John ffitch,

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

                                                        /* buchla.c */
#include "csdl.h"


typedef struct {
      OPDS        h;
  // results
      MYFLT       *out1, *out2, *out3;
  // inputs
      MYFLT       *ain1, *ain2, *ain3, *ain4, *in5, *in6;
  // Internal
      MYFLT       so, sx, sd, xo;
      MYFLT       c1, c2, c3;
      // vactro model
      MYFLT       s1p;
} BUCHLA;

#define clip(a,b,c) (a<b ? b : a>c ? c : a)

int poly_LPG_init(CSOUND* csound, BUCHLA *p)
{
    p->so = p->sx = p->sd = p->xo = 0.0;
    p->c1 = (1e-09);
    p->c2 = (2.2e-10);
    return OK;
}

int poly_LPG_perf(CSOUND* csound, BUCHLA *p)
{
    double c3, rf, r3, max_res, a, f, a1, a2, b1, b2, b3, b4;
    double Dmas, yx, yo, yd, tanh_xo, Dx, Do;
    MYFLT *x, *out1, *out2, *out3;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (*p->in5 != FL(0.0))
      c3 = 4.7e-09;
    else
      c3 = 0.0;

    //r1 = 1e3;
    rf = *p->ain2;               /* does this need to be audio? */
    //rf = 30e3;
    r3 = *p->ain3;               /* does this need to be audio? */

    max_res = 1.0*(2.0 *p->c1*r3+(p->c2+c3)*(r3+rf))/(c3*r3);
    //a = in4* max_res;
    x = p->ain1;
    out1 = p->out1;
    out2 = p->out2;
    out3 = p->out3;

    f = 0.5/csound->GetSr(csound);
    //f = 2*pi * (in2+1e-3)*0.5/samplerate;

    a1 = 1.0/(p->c1*rf);
    a2 = -(1/rf+1/r3)/p->c1;
    b1 = 1.0/(rf*p->c2);
    b2 = -2.0/(rf*p->c2);
    b3 = 1.0/(rf*p->c2);
    b4 = c3/p->c2;

    tanh_xo= tanh(p->xo);
    //b2 = (a*c3/c1 -1 - 1/(1+r1/rf))/(rf*(c2+c3));\t\t\t//\tExtra Feedback resistor
    //b3 =(1/(1+r1/rf)- (1+rf/r3)*a*c3/c1)/(rf*(c2+c3));\t//\tExtra Feedback resistor

    Dx =1.0/(1.0-b2*f);
    Do =1.0/(1.0-a2*f);

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
      memset(out3, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (*p->in6 != FL(0.0)) {
      double txo2 = tanh_xo*tanh_xo;
      for (n=offset; n<nsmps; n++) {
        a = clip(p->ain4[n],0.0,max_res);
        Dmas = 1.0/(1.0-Dx*(f*f*b3*Do*a1 + b4*f*a*(1.0-txo2)*Do*a1 - b4));
        yx =(p->sx + f*b1*x[n] + f*b3*Do*p->so + f*b4*(p->sd+(1.0/f)*a*(tanh_xo - p->xo*(1.0-txo2))) +
             b4*a*(1.0-txo2)*Do*p->so)*Dx*Dmas;
        yo =(p->so+f*a2*yx)*Do;
        yd = (p->sd+(1/f)*a*(tanh_xo - p->xo*(1.0-txo2))) + (1.0/f)*(a*((1.0-txo2))*yo - yx);
        p->sx += 2.0*f*(b1*x[n] + b2*yx + b3*yo +b4*yd);
        p->so += 2.0*f*(a1*yx + a2*yo);
        p->sd = -(p->sd+(2.0/f)*a*(tanh_xo - p->xo*(1.0-txo2))) - (2.0/f)*(a*(1.0-txo2)*yo - yx);
        p->xo = yo;
        out1[n] = yo;
        out2[n] = yx;
        out3[n] = yd;
      }
    }
    else /* if (in6 < 0.5) */ {
      for (n=offset; n<nsmps; n++) {
        a = clip(p->ain4[n],0.0,max_res);
        Dmas = 1.0/(1.0-Dx*(f*f*b3*Do*a1 + b4*f*a*Do*a1 - b4));
        yx = (p->sx + f*b1*x[n] + f*b3*Do*p->so + f*b4*p->sd + b4*a*Do*p->so)*Dx*Dmas;
        yo = (p->so+f*a1*yx)*Do;
        yd = p->sd + (1.0/f)*(a*yo-yx) ;
        p->sx += 2.0*f*(b1*x[n] + b2*yx + b3*yo +b4*yd);
        p->so += 2.0*f*(a1*yx + a2*yo);
        p->sd = -p->sd - (2.0/f)*(a*yo - yx);
        out1[n] = yo;
        out2[n] = yx;
        out3[n] = yd;
      }
    }
    return OK;
}

typedef struct {
      OPDS        h;
      MYFLT       *out;
      MYFLT       *inp;
      double      s1;
      double      a_base;
} VACTROL;

int vactrol_init(CSOUND *csound, VACTROL* p)
{
    p->s1 = 00;
    p->a_base = 1000.0*M_PI/(csound->GetSr(csound));
    return OK;
}

int vactrol_perf(CSOUND *csound, VACTROL* p)
{
    double s1 = p->s1;
    double a_base = p->a_base; 
    //double T_DOWN = 3e3; // Fall time
#define T_DOWN (3.0e3)
    //double T_UP = 20.0; // Rise time
#define T_UP (20.0)
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *in = p->inp;
    MYFLT *out = p->out;
    double e0db = csound->Get0dBFS(csound);;
    
    if (UNLIKELY(offset)) {
      memset(out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
      double t_down = 10.0 + T_DOWN*(1-0.9*s1);
      double a_down = a_base /t_down;
      double dsl = (double)in[n]/e0db - s1;
      double x,y;
      double t_up = 1.0 + T_UP*(1.0-0.999*s1);
      double a_up = a_base /t_up;

      if (dsl >= 0.0)
        x = dsl*a_up/(1.0+a_up);
      else
        x = dsl*a_down/(1.0+a_down);
      y = x + dsl;
      printf("s1 = %f dsl = %f x = %f y = %f\n", s1, dsl, x, y);
      s1 =  y + x;
      out[n] = (MYFLT)y*e0db;
    }
    p->s1 = s1;
    
    return OK;
}



#define S       sizeof

static OENTRY buchla_localops[] = {
  { "buchla", S(BUCHLA), 0, 5, "aaa", "aaaaPP", (SUBR)poly_LPG_init, NULL, (SUBR)poly_LPG_perf },
  { "vactrol", S(VACTROL), 0, 5, "a", "a", (SUBR)vactrol_init, NULL, (SUBR)vactrol_perf }
};

LINKAGE_BUILTIN(buchla_localops)
