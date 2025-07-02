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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

                                                        /* buchla.c */
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#ifdef MSVC
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#define clip(a,b,c) (a<b ? b : a>c ? c : a)

//#ifdef JPFF

typedef struct {
      OPDS        h;
  // results
      MYFLT       *out1;
      //MYFLT     *out2, *out3;
  // inputs
      MYFLT       *ain1, *aenv, *knt, *kin3, *ain4, *ksw5, *ksw6;
  // Internal
      MYFLT       so, sx, sd, xo;
      double      f;
} BUCHLA;

static double kontrolconvert(CSOUND *csound, double in1, double in2);

static int32_t warn = 0;
int32_t poly_LPG_init(CSOUND* csound, BUCHLA *p)
{
    p->so = p->sx = p->sd = p->xo = 0.0;
    if (warn==0) csound->Message(csound, "**** Experimental code ****\n");
    warn++;
#define C1 (1e-09)
#define C2 (2.2e-10)
    p->f = 0.5/CS_ESR;
    return OK;
}

int32_t poly_LPG_perf(CSOUND* csound, BUCHLA *p)
{
    double c3, r3, rf, max_res, a, f=p->f, a1, a2, b1, b2, b3, b4;
    double Dmas, yx, yo, yd, tanh_xo, Dx, Do;
    MYFLT *x, *out1;
    //MYFLT *out2, *out3;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT e0dbfs = csound->Get0dBFS(csound);

    if (*p->ksw5 != FL(0.0))
      c3 = 4.7e-09;
    else
      c3 = 0.0;

    //r1 = 1e3;
    //
    //#define rf (30.e3)
    r3 = *p->kin3;               /* does this need to be audio? */

    x = p->ain1;
    out1 = p->out1;
    /* out2 = p->out2; */
    /* out3 = p->out3; */

    //f = 2*pi * (in2+1e-3)*0.5/samplerate;

    b4 =  c3/C2;

    tanh_xo= tanh(p->xo);

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      /* memset(out2, '\0', offset*sizeof(MYFLT)); */
      /* memset(out3, '\0', offset*sizeof(MYFLT)); */
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      /* memset(&out2[nsmps], '\0', early*sizeof(MYFLT)); */
      /* memset(&out3[nsmps], '\0', early*sizeof(MYFLT)); */
    }

    if (*p->ksw6 != FL(0.0)) {
      double txo2 = tanh_xo*tanh_xo;
      double knt = *p->knt;
      for (n=offset; n<nsmps; n++) {
        rf = kontrolconvert(csound, (double)p->aenv[n], knt);
        max_res = 1.0*(2.0*C1*r3+(C2+c3)*(r3+rf))/(c3*r3);
        a = clip(p->ain4[n],0.0,max_res);
        a1 =  1.0/(C1*rf);
        a2 = -(1/rf+1/r3)/C1;
        b1 =  1.0/(rf*C2);
        b2 = -2.0/(rf*C2);
        b3 =  1.0/(rf*C2);
        Dx =1.0/(1.0-b2*f);
        Do =1.0/(1.0-a2*f);
        Dmas = 1.0/(1.0-Dx*(f*f*b3*Do*a1 + b4*f*a*(1.0-txo2)*Do*a1 - b4));
        yx =(p->sx + f*b1*x[n]/e0dbfs + f*b3*Do*p->so +
            f*b4*(p->sd+(1.0/f)*a*(tanh_xo - p->xo*(1.0-txo2))) +
             b4*a*(1.0-txo2)*Do*p->so)*Dx*Dmas;
        yo =(p->so+f*a2*yx)*Do;
        yd = (p->sd+(1/f)*a*(tanh_xo - p->xo*(1.0-txo2))) +
             (1.0/f)*(a*((1.0-txo2))*yo - yx);
        p->sx += 2.0*f*(b1*x[n]/e0dbfs + b2*yx + b3*yo +b4*yd);
        p->so += 2.0*f*(a1*yx + a2*yo);
        p->sd = -(p->sd+(2.0/f)*a*(tanh_xo - p->xo*(1.0-txo2))) -
                 (2.0/f)*(a*(1.0-txo2)*yo - yx);
        p->xo = yo;
        out1[n] = (MYFLT)yo*e0dbfs*25.0; /* JPff scaling */
        /* out2[n] = (MYFLT)yx; */
        /* out3[n] = (MYFLT)yd; */
      }
    }
    else /* if (ksw6 < 0.5) */ {
      double knt = *p->knt;
      for (n=offset; n<nsmps; n++) {
        rf = kontrolconvert(csound, (double)p->aenv[n], knt);
        max_res = 1.0*(2.0*C1*r3+(C2+c3)*(r3+rf))/(c3*r3);
        a1 =  1.0/(C1*rf);
        a2 = -(1/rf+1/r3)/C1;
        b1 =  1.0/(rf*C2);
        b2 = -2.0/(rf*C2);
        b3 =  1.0/(rf*C2);
        a = clip(p->ain4[n],0.0,max_res);
        Dx =  1.0/(1.0-b2*f);
        Do =  1.0/(1.0-a2*f);
        Dmas = 1.0/(1.0-Dx*(f*f*b3*Do*a1 + b4*f*a*Do*a1 - b4));
        yx = (p->sx + f*b1*x[n] + f*b3*Do*p->so +
              f*b4*p->sd + b4*a*Do*p->so)*Dx*Dmas;
        yo = (p->so+f*a1*yx)*Do;
        yd = p->sd + (1.0/f)*(a*yo-yx) ;
        p->sx += 2.0*f*(b1*x[n] + b2*yx + b3*yo +b4*yd);
        p->so += 2.0*f*(a1*yx + a2*yo);
        p->sd = -p->sd - (2.0/f)*(a*yo - yx);
        out1[n] = yo*25.0*e0dbfs; /* JPff scaling */
        /* out2[n] = yx; */
        /* out3[n] = yd; */
      }
    }
    return OK;
}
//#endif

typedef struct {
      OPDS        h;
      MYFLT       *out;
      MYFLT       *inp;
      MYFLT       *up;
      MYFLT       *down;
      double      s1;
      double      a_base;
      double      t_up;
      double      t_down;
} VACTROL;

int32_t vactrol_init(CSOUND *csound, VACTROL* p)
{
    p->s1 = 0;
    p->a_base = 1000.0*PI/(CS_ESR);
    p->t_down = *p->down<FL(0.0) ? 3.0e3 : (double)*p->down;
    p->t_up   = *p->up<FL(0.0) ? 20.0 : (double)*p->up;
    return OK;
}

int32_t vactrol_perf(CSOUND *csound, VACTROL* p)
{
    double s1 = p->s1;
    double a_base = p->a_base;
    double T_DOWN = p->t_down; // Fall time
    double T_UP   = p->t_up; // Rise time
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *in = p->inp;
    MYFLT *out = p->out;
    double e0db = csound->Get0dBFS(csound);

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
      y = x + s1;
      s1 =  y + x;
      out[n] = (MYFLT)y*e0db; /* JPff extra scale */
    }
    p->s1 = s1;

    return OK;
}

//#ifdef JPFF

//Nonlinear control circuit maps V_b to R_f (Vactrol Resistance)

static double kontrolconvert(CSOUND *csound, double in1, double in2)
{
    double R1, R2;
    double offset = 0.9999*in2 + 0.0001;
    double zerodb = csound->Get0dBFS(csound);
    double V3, Ia, If, Ifbound1, Ifbound2, Ifbound3;
    double Vb = in1/zerodb;
    double ans;

#define scale (0.48) // This value is tuned for appropriate input range.
    // Constants
#define A (3.4645912)
#define B (1136.2129956)
#define G (2.0e5)
#define Ifmax (40.0e-3)
#define Ifmin (10.1e-6)
#define R2max (10.0e3)
#define R3 (150.0e3)
#define R4 (470.0e3)
#define R5 (100.0e3)
#define R6max (20.0e3)
#define R7 (33.0e3)
#define R8 (4.7e3)
#define R9 (470)
#define VB (3.9)
#define VF (0.7)
#define VT (26.0e-3)
#define Vs (15.0)
#define k0 (1.468e2)
#define k1 (4.9202e-1)
#define k2 (4.1667e-4)
#define k3 (7.3915e-9)
#define kl (6.3862)
#define n (3.9696)

#define gamma (0.0001)

#define R6 (scale * R6max)

#define alpha (1.0 + (R6+R7) * (1/R3 + 1/R5))
#define beta  (((1/alpha) - 1)/(R6 + R7) - 1/R8)

#define bound1 (600* alpha *n*VT/(G*(R6+R7-1/(alpha*beta))))

    //Inputs
    R1 = (1-offset);
    R2 = offset;

    Ia = Vb/R5 + Vs/(R3*(1+R1/R2));

    if (Ia <= -bound1) {
      V3 = -Ia/(alpha*beta);
    }
    else if ( Ia < bound1) {
      double x, w;
      x = G*Ia*(R6+R7-1/(alpha*beta))/(alpha*n*VT);
      w = k0 + k1*x + k2*x*x+ k3*x*x*x;
      V3 = -(alpha/G)*n*VT*w - Ia/(alpha*beta);
    }
    else {
      V3 = kl*alpha/G*n*VT-Ia*(R6+R7);
    }
    Ifbound1 = alpha*(Ifmin - beta*V3);
    Ifbound2 = VB/(R6+R7);
    Ifbound3 = (gamma*G*VB + alpha*R9*(VB*beta+Ifmax))/(gamma*G*(R6+R7) + R9);

    if (Ia <= Ifbound1) {
      If = Ifmin;
    }
    else if (Ia <= Ifbound2) {
      If = beta * V3 + Ia/alpha;
    }
    else if (Ia <= Ifbound3) {
      If = gamma * G *(Ia*(R6+R7) - VB)/(alpha*R9) - beta*VB + Ia/alpha;
    }
    else {
      If = Ifmax;
    }

    ans = (B + A / pow(If,1.4));
    //printf("%f,%f (%f/%f/%f) -> %f\n", in1, in2, A, B, pow(If, 1.4),  ans);
    return ans;
}
//#endif


#define S       sizeof

static OENTRY buchla_localops[] = {
  #ifdef JPFF
  { "buchla", S(BUCHLA), 0,  "a", "aakkaPP",
                            (SUBR)poly_LPG_init, (SUBR)poly_LPG_perf },
  #endif
  { "vactrol", S(VACTROL), 0,  "a", "ajj",
                                 (SUBR)vactrol_init, (SUBR)vactrol_perf }
};

LINKAGE_BUILTIN(buchla_localops)
