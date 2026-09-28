/*
    bilbar.c:

    Copyright (C) 2006 by Stefan Bilbao and John ffitch

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <float.h>
#include <math.h>

/* %% bar sound synthesis translated from Mathlab and much changed */

/*  Note: position of strike along bar (0 - 1), normalized strike
    velocity, and spatial width of strike.  */

typedef struct {
    OPDS    h;
    cs_float   *ar;                /* Output */

    cs_float   *kbcL, *kbcR, *iK, *ib, *kscan, *iT30;
    cs_float   *ipos, *ivel, *iwid;

    cs_double  *w, *w1, *w2;
    int32_t     step, first;
    cs_double  s0, s1, s2, t0, t1;
    int32_t     bcL, bcR, N;
    AUXCH   w_aux;
} BAR;

static int32_t bar_init(CSOUND *csound, BAR *p)
{
    if (*p->iK >= FL(0.0) || p->w_aux.auxp == NULL) {
      cs_double  K = FABS(*p->iK); /* ~=3.0  stiffness parameter, dimensionless */
      cs_double  T30 = *p->iT30;   /* ~=5.0; 30 db decay time (s) */
      cs_double  b = *p->ib;       /* ~=0.001 high-frequency loss parameter
                                   (keep small) */
      cs_double  dt, sig, dxmin, points, dx;
      size_t  gridSize;
      int32_t N;

      if (UNLIKELY(!(T30 > 0.0)))
        return csound->InitError(csound, "%s",
                                 Str("barmodel: decay time must be positive"));
      if (UNLIKELY(!(b >= 0.0)))
        return csound->InitError(csound, "%s",
                                 Str("barmodel: loss must be non-negative"));

      /* %%%%%%%%%%%%%%%%%% derived parameters */
      dt = (cs_double)CS_ONEDSR;
      sig = (2.0*(cs_double)CS_ESR)*(pow(10.0,3.0*dt/T30)-1.0);
      dxmin = sqrt(dt*(b+hypot(b, K+K)));
      if (UNLIKELY(!(sig >= 0.0 && sig <= DBL_MAX)))
        return csound->InitError(csound, "%s",
                                 Str("barmodel: decay time is too short"));
      points = 1.0 / dxmin;
      if (UNLIKELY(!(points >= 1.0 &&
                     points <= (INT32_MAX + 0.0) - 5 &&
                     points <= (cs_double)(SIZE_MAX /
                                        (3 * sizeof(cs_double)) - 5))))
        return csound->InitError(csound, "%s",
                                 Str("barmodel: stiffness and loss produce "
                                     "an invalid grid size"));
      N = (int32_t) points;
      dx = 1.0/N;

      /* %%%%%%%%%%%%%%%%%%% scheme coefficients */
      p->s0 = (2.0-6.0*K*K*dt*dt/(dx*dx*dx*dx)-2.0*b*dt/(dx*dx))/(1.0+sig*dt*0.5);
      p->s1 = (4.0*K*K*dt*dt/(dx*dx*dx*dx)+b*dt/(dx*dx))/(1.0+sig*dt*0.5);
      p->s2 = -K*K*dt*dt/((dx*dx*dx*dx)*(1.0+sig*dt*0.5));
      p->t0 = (-1.0+2.0*b*dt/(dx*dx)+sig*dt*0.5)/(1.0+sig*dt*0.5);
      p->t1 = (-b*dt)/(dx*dx*(1.0+sig*dt*0.5));

/*     csound->Message(csound,"Scheme: %f %f %f ; %f %f\n",
                       p->s0, p->s1, p->s2, p->t0, p->t1); */

      /* %%%%%%%%%%%%%%%%%%%%% create grid functions */

      gridSize = (size_t)N + 5;
      csound->AuxAlloc(csound, 3 * gridSize * sizeof(cs_double), &(p->w_aux));
      p->w = (cs_double *) p->w_aux.auxp;
      p->w1 = &(p->w[N + 5]);
      p->w2 = &(p->w1[N + 5]);
      p->step = p->first = 0;
      p->N = N;
    }
    /*
    else {
      if (UNLIKELY(p->w_aux.auxp == NULL))
        return csound->InitError(csound, "%s", Str("No data to continue"));
    }
    */
    p->first = 0;

    return OK;
}

static int32_t bar_run(CSOUND *csound, BAR *p)
{
    cs_double xofreq = TWOPI* (*p->kscan)/CS_ESR; /* kspan ~=0.23; */
    cs_double xo, xofrac;
    int32_t xoint;
    int32_t step = p->step;
    int32_t first = p->first;
    int32_t N = p->N, rr;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_double *w = p->w, *w1 = p->w1, *w2 = p->w2;
    cs_double s0 = p->s0, s1 = p->s1, s2 = p->s2, t0 = p->t0, t1 = p->t1;
    /*  boundary condition pair  1: clamped, 2: pivoting, 3: free */
    int32_t bcL = (int32_t)CS_FLOAT2LONG(*p->kbcL);
    int32_t bcR = (int32_t)CS_FLOAT2LONG(*p->kbcR);
    cs_double SINNW = sin(xofreq*step); /* these are to calculate sin/cos by */
    cs_double COSNW = cos(xofreq*step); /* formula rather than many calls    */
    cs_double SIN1W = sin(xofreq);      /* Wins in ksmps>4 */
    cs_double COS1W = cos(xofreq);
    cs_float *ar = p->ar;

    if (UNLIKELY((bcL|bcR)&(~3) && (bcL|bcR)!=0))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("Ends must be clamped(1), "
                                   "pivoting(2) or free(3)"));
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n = offset; n < nsmps; n++) {
      /* Fix ends */
      if (bcL == 3) {
        w1[1] = 2.0*w1[2]-w1[3];
        w1[0] = 3.0*w1[1]-3.0*w1[2]+w1[3];
      }
      else if (bcL == 1) {
        w1[2] = 0.0;
        w1[3] = 0.0;
      }
      else if (bcL == 2) {
        w1[2] = 0.0;
        w1[1] = -w1[3];
      }

      if (bcR == 3) {
        w1[N+3] = 2.0*w1[N+2]-w1[N+1];
        w1[N+4] = 3.0*w1[N+3]-3.0*w1[N+2]+w1[N+1];
      }
      else if (bcR == 1) {
        w1[N+1] = 0.0;
        w1[N+2] = 0.0;
      }
      else if (bcR == 2) {
        w1[N+2] = 0.0;
        w1[N+3] = -w1[N+1];
      }

      /* Iterate model */
      for (rr = 0; rr < N+1; rr++) { /* Is N+1 correct here?? */
        w[rr+2] = s0*w1[rr+2] + s1*(w1[rr+3]+w1[rr+1]) + s2*(w1[rr+4]+w1[rr]) +
                  t0*w2[rr+2] + t1*(w2[rr+3]+w2[rr+1]);
      }
      /*  strike inputs */

      if (first == 0) {
        p->first = first = 1;
        for (rr = 0; rr < N; rr++) {
          if (fabs(rr/(cs_double)N - *p->ipos) <= *p->iwid) {
            w[rr+2] += (1.0/CS_ESR)*(*p->ivel)*0.5*
                (1.0+cos(PI*fabs(rr/(cs_double)N-(*p->ipos))/(*p->iwid)));
          }
        }
      }

      /*  readouts */

      /*       xo = (1.0/3.0) + 0.5*sin(TWOPI*xofreq*(step+1)/CS_ESR); */
      /* sin((N+1)w) = sin(Nw)cos(w) + cos(Nw)sin(w) */
      /* cos((N+1)w) = cos(Nw)cos(w) - sin(Nw)sin(w) */
      /* so can calculate sin on next line by iteration at less cost */
      /* But is xofreq were to change could be difficult! */
      /*      xo = 0.5 + 0.5*sin(TWOPI*xofreq*(step+1)/CS_ESR); */
      {
        cs_double  xx = SINNW*COS1W + COSNW*SIN1W;
        cs_double  yy = COSNW*COS1W - SINNW*SIN1W;

        SINNW = xx;
        COSNW = yy;
      }
      xo = 0.5 + 0.5*SINNW;
      xoint = (int32_t) (xo*N) + 2;
      xofrac = xo*N - (int32_t)(xo*N);

/*       csound->Message(csound, "xo = %f (%d %f) w=(%f,%f) ",
                         xo, xoint, xofrac, w[xoint], w[xoint+1]); */
      ar[n] = (csound->Get0dBFS(csound))*((1.0-xofrac)*w[xoint] + xofrac*w[xoint+1]);
      step++;
      {
        cs_double *ww = w2;

        w2 = w1;
        w1 = w;
        w = ww;
      }
    }
    p->step = step;
    p->w = w;
    p->w1 = w1;
    p->w2 = w2;

    return OK;
}

/* Prepared Piano string */

/* Preparation records use the sample type of their function tables. */
typedef struct {
  cs_float pos;                    /* position along string of rattle */
  cs_float massden;                /* mass density ratio (rattle/string) */
  cs_float freq;                   /* fundamental freq. of rattle */
  cs_float length;                 /* vertical length of rattle */
} RATTLE;

typedef struct {
  cs_float pos;                    /* position along string of rubber */
  cs_float massden;                /* mass density ratio (rubber/string) */
  cs_float freq;                   /* fundamental freq. of rubber */
  cs_float loss;                   /* loss parameter of rubber */
} RUBBER;

typedef struct {
    OPDS   h;
    cs_float *ar;
    cs_float *ar1;
    cs_float *ifreq;
    cs_float *iNS;     /* number of strings */
    cs_float *iD;      /* detune parameter (multiple strings) in cents!!! */
    cs_float *K;       /* stiffness parameter, dimensionless...set around 1
                       for low notes, set closer to 100 in high register */
    cs_float *iT30;    /* 30 db decay time (s) */
    cs_float *ib;      /* high-frequency loss parameter (keep this small) */
    cs_float *kbcl,*kbcr; /* Boundary conditions */
    cs_float *ham_massden, *ham_freq, *ham_initial;
    cs_float *ipos;
    cs_float *vel;
    cs_float *scanfreq, *scanspread;
    cs_float *rattle_tab, *rubber_tab;

    cs_float *w, *w1, *w2;
    cs_float *rat, *rat1, *rat2;
    cs_float *rub, *rub1, *rub2;
    cs_float *s0, *s1, s2, t0, t1;
    cs_float *hammer_force;
    int32_t    stereo;
    uint32_t    NS;
    int32_t    N, init, step;
    uint32_t    rattle_num, rubber_num;
    int32_t    hammer_index, hammer_on, hammer_contact;
    cs_float  ham, ham1, ham2;
    AUXCH  auxchc;
    AUXCH  auxch;
    RATTLE *rattle;
    RUBBER *rubber;
} CSPP;

int32_t init_pp(CSOUND *csound, CSPP *p)
{
    if (*p->K >= FL(0.0)) {
      cs_double K = *p->K; /* stiffness parameter, dimensionless */
      cs_double f0 = *p->ifreq;      /* fundamental freq. (Hz) */
      cs_double T30 = *p->iT30;      /* 30 db decay time (s) */
      cs_double b = *p->ib;          /* high-frequency loss parameter (keep small) */
      uint32_t NS = p->NS = (int32_t)*p->iNS;       /* number of strings */
      cs_double D = *p->iD;  /* detune parameter (multiple strings) in cents */
                          /* I.e., a total of D cents diff between highest */
                          /* and lowest string in set */
                          /* initialize prepared objects and hammer */
                          /* derived parameters */
      cs_double dt = (cs_double)CS_ONEDSR;
      cs_double sig = (2.0*(cs_double)CS_ESR)*(pow(10.0,3.0*dt/T30)-1.0);

      uint32_t N, n;
      cs_double *c, /*dx,*/ dxmin = 0.0; /* for stability */
      FUNC  *ftp;

      csound->AuxAlloc(csound, NS*sizeof(cs_double), &p->auxchc);
      c = (cs_double *)p->auxchc.auxp;

      if (*p->rattle_tab==FL(0.0) ||
          (ftp=csound->FTFind(csound, p->rattle_tab)) == NULL) p->rattle_num = 0;
      else {
        p->rattle_num = (uint32_t)(*ftp->ftable);
        p->rattle = (RATTLE*)(&((cs_float*)ftp->ftable)[1]);
      }
      if (*p->rubber_tab==FL(0.0) ||
          (ftp=csound->FTFind(csound, p->rubber_tab)) == NULL) p->rubber_num = 0;
      else {
        p->rubber_num = (uint32_t)(*ftp->ftable);
        p->rubber = (RUBBER*)(&((cs_float*)ftp->ftable)[1]);
      }

      for (n=0; n<NS; n++) {
        cs_double detune_spread = NS == 1 ? 0.0 :
          (D*n/(NS-1.0) - D*0.5)/1200.0;
        c[n] = 2.0*f0*pow(2.0, detune_spread);
      }

      for (n=0; n<NS; n++) {
        cs_double y = c[n]*c[n]*dt*dt+2.0*b*dt;
        cs_double x = sqrt(y+hypot(y,4.0*K*dt))/ROOT2;
        if (x>dxmin) dxmin = x;
      }
      N = p->N = (uint32_t)(1.0/dxmin);
      //dx = 1.0/(double)N;

      csound->AuxAlloc(csound,
                       3*((1+(N+5))*NS+p->rattle_num+p->rubber_num)*sizeof(cs_float),
                       &p->auxch);
      //c = (double *)p->auxch.auxp;
      p->s0 = (cs_float*)p->auxch.auxp;
      p->s1 = &p->s0[NS];
      p->hammer_force = &p->s1[NS];

      for (n=0; n<NS; n++) {
        p->s0[n] = (2.0-6.0*K*K*dt*dt*N*N*N*N-2.0*b*dt*N*N-
                    2.0*c[n]*c[n]*dt*dt*N*N)/(1.0+sig*dt*0.5);
        p->s1[n] = (4*K*K*dt*dt*N*N*N*N+b*dt*N*N+
                    c[n]*c[n]*dt*dt*N*N)/(1.0+sig*dt*0.5);
        /* Above optimises to this */
/*         g9 = N*N*dt; */
/*         g15 = g9*K; */
/*         g8 = g15*g15; */
/*         g13 = g9*c[n]*c[n]*dt + g9*b; */
/*         g6 = 0.5 * dt*sig + 1.0; */
/*         p->s0[n] =  (2.0*(1.0 - g13 - 3.0*g8))/g6; */
/*         p->s1[n] = (g13 + 4.0*g8)/g4; */
      }
      p->s2 = -K*K*dt*dt*N*N*N*N/(1.0+sig*dt*0.5);
      p->t0 = (-1.0+2.0*b*dt*N*N+sig*dt*0.5)/(1.0+sig*dt*0.5);
      p->t1 = (-b*dt)*N*N/(1.0+sig*dt*0.5);

      /* note, each of these is an array, of size N+5 by NS...i.e., need a
         separate N+5 element array per string. */
      p->w = &p->hammer_force[NS];
      p->w1 = &p->w[(N+5)*NS];
      p->w2 = &p->w1[(N+5)*NS];
      p->rat = &p->w2[(N+5)*NS];
      p->rat1 = &p->rat[p->rattle_num];
      p->rat2 = &p->rat1[p->rattle_num];
      p->rub = &p->rat2[p->rattle_num];
      p->rub1 = &p->rub[p->rubber_num];
      p->rub2 = &p->rub1[p->rubber_num];
      p->ham = 0; p->ham1 = 0; p->ham2 = 0; /*only one hammer */
      p->step = 0;
    }
    p->init = 1;                /* So we start the hammer */
    if (p->OUTOCOUNT==1) p->stereo = 0;
    else p->stereo = 1;
    return OK;
}

int32_t play_pp(CSOUND *csound, CSPP *p)
{
    cs_float *ar = p->ar;
    cs_float *ar1 = p->ar1;
    uint32_t NS = p->NS;
    uint32_t N = p->N;
    int32_t step = p->step;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t t, n, nsmps = CS_KSMPS;
    cs_double dt = CS_ONEDSR;
    cs_float *w = p->w, *w1 = p->w1, *w2 = p->w2,
          *rub = p->rub, *rub1 = p->rub1, *rub2 = p->rub2,
          *rat = p->rat, *rat1 = p->rat1, *rat2 = p->rat2;
    cs_float *s0 = p->s0, *s1 = p->s1, s2 = p->s2, t0 = p->t0, t1 = p->t1;
    cs_double SINNW = 0;              /* these are to calculate sin/cos by */
    cs_double COSNW = 0;              /* formula rather than many calls    */
    cs_double SIN1W = 0;              /* Wins in ksmps>4 */
    cs_double COS1W = 0;
    cs_double SINNW2 = 0;
    cs_double COSNW2 = 0;
    cs_double SIN1W2 = 0;
    cs_double COS1W2 = 0;
    cs_float e0dbfs = csound->Get0dBFS(csound);

    if (p->stereo) {
      cs_double f1 = (*p->scanfreq - FL(0.5)* *p->scanspread)/CS_ESR;
      cs_double f2 = (*p->scanfreq + FL(0.5)* *p->scanspread)/CS_ESR;
      SINNW = sin(f1*TWOPI*step); /* these are to calculate sin/cos by */
      COSNW = cos(f1*TWOPI*step); /* formula rather than many calls    */
      SIN1W = sin(f1*TWOPI);      /* Wins in ksmps>4 */
      COS1W = cos(f1*TWOPI);
      SINNW2 = sin(f2*TWOPI*step);
      COSNW2 = cos(f2*TWOPI*step);
      SIN1W2 = sin(f2*TWOPI);
      COS1W2 = cos(f2*TWOPI);
    }
    else {
      cs_double f1 = *p->scanfreq/CS_ESR;
      SINNW = sin(f1*TWOPI*step); /* these are to calculate sin/cos by */
      COSNW = cos(f1*TWOPI*step); /* formula rather than many calls    */
      SIN1W = sin(f1*TWOPI);      /* Wins in ksmps>4 */
      COS1W = cos(f1*TWOPI);
    }

    if (p->init) {
      p->hammer_on = 1;          /*  turns on hammer updating */
      p->hammer_contact = 0;     /* hammer not in contact with string yet */
      p->hammer_index = 2+(int32_t)(*p->ipos*N); /* find location of hammerstrike */
      p->ham2 = *p->ham_initial;
      p->ham1 = *p->ham_initial+dt*(*p->vel);    /* initialize hammer */
      p->init = 0;
    }

    if (UNLIKELY(offset)) {
      memset(ar, '\0', offset*sizeof(cs_float));
      if (p->stereo) memset(ar1, '\0', offset*sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
      if (p->stereo) memset(&ar1[nsmps], '\0', early*sizeof(cs_float));
    }
    for (t=offset; t<nsmps; t++) {
      uint32_t qq;
      for (n=0; n<NS; n++) p->hammer_force[n] = 0.0;
      /* set boundary conditions on last state w1 */
      if ((int32_t)*p->kbcl==1) {
        for (n=0; n<NS; n++)
          w1[n+NS*2] = w1[n+NS*3] = 0.0;
      }
      else if ((int32_t)*p->kbcl==2) {
        for (qq=0; qq<NS; qq++) {
          w1[qq+NS*2] = 0.0;  w1[qq+NS*1] = -w1[qq+NS*3];
        }
      }
      if ((int32_t)*p->kbcr==1) {
        for (n=0; n<NS; n++)
          w1[n+NS*(N+2)] = w1[n+NS*(N+1)] = 0.0;
      }
      else if ((int32_t)*p->kbcr==2) {
        for (n=0; n<NS; n++) {
          w1[n+NS*(N+2)] = 0.0;  w1[n+NS*(N+3)] = -w1[n+NS*(N+1)];
        }
      }

      /* perform update, for each of the NS strings */
      for (n=0; n<N; n++)
        for (qq=0; qq<NS; qq++) {
          w[(n+2)*NS+qq] =
            s0[qq]*w1[(n+2)*NS+qq]+
            s1[qq]*(w1[(n+3)*NS+qq]+w1[(n+1)*NS+qq])+
            s2*(w1[(n+4)*NS+qq]+w1[n*NS+qq])+
            t0*w2[(n+2)*NS+qq]+
            t1*(w2[(n+3)*NS+qq]+w2[(n+1)*NS+qq]);
        }

      if (p->rattle_num)
        /* do this only if at least one rattle is specified */
        for (qq=0; qq<p->rattle_num; qq++) {
          int32_t rattle_index = (int32_t)(2+p->rattle[qq].pos*N);
          cs_float force_sum = 0.0;
          for (n=0; n<NS; n++) {
            cs_float pos, force, temp;
            /* calc. pos. diff between center of rattle and string */
            pos = w1[rattle_index*NS+n]-rat1[qq];
            temp = fabs(pos)-p->rattle[qq].length*0.5;
            /* calc force (nonzero only when in contact) */
            force = 0.5*(temp+fabs(temp))*(pos>0?1.0:-1.0);
            if (force!=0.0) {
              w[rattle_index*NS+n] += -dt*dt*(TWOPI*p->rattle[qq].freq)*
                (TWOPI*p->rattle[qq].freq)*p->rattle[qq].massden*force;
            }
            force_sum += force;
          }
          /* All strings act on the same preparation during this sample. */
          rat[qq] = 2*rat1[qq]-rat2[qq]+(TWOPI*p->rattle[qq].freq)*
            (TWOPI*p->rattle[qq].freq)*dt*dt*force_sum-dt*dt*9.8;
          rat2[qq] = rat1[qq];
          rat1[qq] = rat[qq];
        }
      if (p->rubber_num) {
        /* do this only if at least one rubber is specified */
        for (qq=0; qq<p->rubber_num; qq++) {
          int32_t rubber_index = (int32_t)(2+p->rubber[qq].pos*N);
          cs_float force_sum = 0.0;
          for (n=0; n<NS; n++) {
            cs_float pos, force;
            /* calc. pos. diff between rubber and string */
            pos = w1[rubber_index*NS+n]-rub1[qq];
            /* Each string receives only its own contact force. */
            force = 0.5*(pos-fabs(pos));
            w[rubber_index*NS+n] += -dt*dt*(TWOPI*p->rubber[qq].freq)*
              (TWOPI*p->rubber[qq].freq)*p->rubber[qq].massden*force;
            force_sum += force;
          }
          rub[qq] = 2*rub1[qq]/(1+p->rubber[qq].loss*dt/2)-
            (1-p->rubber[qq].loss*dt/2)*rub2[qq]/(1+p->rubber[qq].loss*dt*0.5)+
            (TWOPI*p->rubber[qq].freq)*(TWOPI*p->rubber[qq].freq)*
            dt*dt*(-rub1[qq]+force_sum)/(1+p->rubber[qq].loss*dt*0.5);
          rub2[qq] = rub1[qq];
          rub1[qq] = rub[qq];
        }
      }
      if (p->hammer_on) {
        cs_float min_pos = 100;
        cs_float hammer_force_sum = 0.0;

        /* do this while a strike is occurring */
        for (qq=0; qq<NS; qq++) { /* Over each string */
          cs_float pos = w1[p->hammer_index*NS+qq]-p->ham1;
          if (pos>0.0) pos = 0.0;
          if (pos<min_pos) min_pos = pos;
          if (min_pos<0.0) p->hammer_contact = 1;
          p->hammer_force[qq] = -pos*pos*pos;
          hammer_force_sum += p->hammer_force[qq];
        }
     /* if (min_pos<0.0) p->hammer_contact = 1; */
        if (p->hammer_contact && min_pos>=0.0) {
          /* if hammer has been in contact, but now no longer is, turn off */
          /* hammer updating */
          p->hammer_on = p->hammer_contact=0;
        }
        p->ham = 2.0*p->ham1-p->ham2-dt*dt*hammer_force_sum*
          (TWOPI*(*p->ham_freq))*(TWOPI*(*p->ham_freq));
        p->ham2 = p->ham1;
        p->ham1 = p->ham;
          for (qq=0; qq<NS; qq++)
            w[p->hammer_index*NS+qq] += dt*dt*(*p->ham_massden)*
              (TWOPI*(*p->ham_freq))*(TWOPI*(*p->ham_freq))*p->hammer_force[qq];
      }
      {
#define       xoamp  FL(0.3333)
#define       xoctr  FL(0.3333)
        int32_t xoint;
        cs_float xofrac, xo;
        cs_float out = 0.0;
        cs_double  xx = SINNW*COS1W + COSNW*SIN1W;
        cs_double  yy = COSNW*COS1W - SINNW*SIN1W;

        SINNW = xx;
        COSNW = yy;
        xo = xoctr + xoamp*SINNW;
        /*        xo = xoctr+xoamp*(cs_float)sin(TWOPI*(*p->scanfreq)*n*dt); */
        xoint = (int32_t)(xo*N)+2;
        xofrac = xo*N - xoint + FL(2.0);
        for (qq=0; qq<NS; qq++) {
          out += (1-xofrac)*w[xoint*NS+qq]+xofrac*w[(xoint+1)*NS+qq];
        }
        ar[t] = FL(200.0)*out*e0dbfs;
        if (p->stereo) {
          xx = SINNW2*COS1W2 + COSNW2*SIN1W2;
          yy = COSNW2*COS1W2 - SINNW2*SIN1W2;
          SINNW2 = xx;
          COSNW2 = yy;
          xo = xoctr + xoamp*SINNW2;
          /*        xo = xoctr+xoamp*(cs_float)sin(TWOPI*(*p->scanfreq)*n*dt); */
          xoint = (int32_t)(xo*N)+2;
          xofrac = xo*N - xoint + FL(2.0);
          out = FL(0.0);
          for (qq=0; qq<NS; qq++) {
            out += (1-xofrac)*w[xoint*NS+qq]+xofrac*w[(xoint+1)*NS+qq];
          }
          ar1[t] = FL(200.0)*out*e0dbfs;
        }
        step++;
      }
      {
        uint32_t i;
        void *w3 = w2;
        w2 = w1;
        w1 = w;
        w = w3;
        for (i=0; i<NS; i++)  /* Need to finish bndry conditions */
          w[NS+i] = w[i+NS*(N+3)] = FL(0.0);
      }
    } /* End of main loop */
    p->w = w; p->w1 = w1; p->w2 = w2;
    p->rub = rub; p->rub1 = rub1; p->rub2 = rub2;
    p->rat = rat; p->rat1 = rat1; p->rat2 = rat2;
    p->step = step;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY bilbar_localops[] = {
  { "barmodel", S(BAR), 0,  "a", "kkiikiiii", (SUBR) bar_init,
                                               (SUBR) bar_run},
  { "prepiano", S(CSPP), 0,  "mm", "iiiiiikkiiiiiiioo",
                                (SUBR)init_pp, (SUBR)play_pp },
};

LINKAGE_BUILTIN(bilbar_localops)
