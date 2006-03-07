/* Copyright 2006: Stefan Bilbao and John ffitch */

#include "csdl.h"
#include <math.h>

/* %% bar sound synthesis script */

/* %%%%%%%%%%%%%%%%% some input parameters */

/*  Note: each row of eventlist contains: time of strike (s), position of strike */
/*  along bar (0 - 1), normalized strike velocity, and spatial width of strike.  */

typedef struct {
    OPDS        h;
    MYFLT       *ar;                  /* Output */

    MYFLT       *kbcL, *kbcR, *iK, *ib, *kscan, *iT30;
    MYFLT       *ipos, *ivel, *iwid;

    double      *w, *w1, *w2;
    int         step, first;
    double      s0, s1, s2, t0, t1;
    int         bcL, bcR, N;
    AUXCH       w_aux;
} BAR;

static int bar_init(CSOUND *csound, BAR *p)
{
    if (*p->iK>=FL(0.0)) {
      double K = *p->iK;          /* 3.0  stiffness parameter, dimensionless */
      double T30 = *p->iT30;      /* 5.0; 30 db decay time (s) */
      double b = *p->ib; /* 0.001 high-frequency loss parameter (keep this small) */
/* %%%%%%%%%%%%%%%%%% derived parameters */
      double dt = 1.0/csound->esr;
      double sig = (2.0/dt)*(pow(10.0,3.0*dt/T30)-1.0);
      double dxmin = sqrt(dt*(b+sqrt(b*b+4*K*K)));
      int N = (int) (1.0/dxmin);
      double dx = 1.0/N;
/* %%%%%%%%%%%%%%%%%%% scheme coefficients */
      p->s0 = (2.0-6.0*K*K*dt*dt/(dx*dx*dx*dx)-2.0*b*dt/(dx*dx))/(1.0+sig*dt*0.5);
      p->s1 = (4.0*K*K*dt*dt/(dx*dx*dx*dx)+b*dt/(dx*dx))/(1.0+sig*dt*0.5);
      p->s2 = -K*K*dt*dt/((dx*dx*dx*dx)*(1.0+sig*dt*0.5));
      p->t0 = (-1.0+2.0*b*dt/(dx*dx)+sig*dt*0.5)/(1+sig*dt*0.5);
      p->t1 = (-b*dt)/(dx*dx*(1.0+sig*dt*0.5));

/*     csound->Message(csound,"Scheme: %f %f %f ; %f %f\n",
                       p->s0, p->s1, p->s2, p->t0, p->t1); */

    /* %%%%%%%%%%%%%%%%%%%%% create grid functions */

      csound->AuxAlloc(csound, 3L * (long) ((N + 5) * sizeof(double)),
                       &(p->w_aux));
      p->w = (double*) p->w_aux.auxp;
      p->w1 = &(p->w[N + 5]);
      p->w2 = &(p->w1[N + 5]);
      p->step = p->first = 0;
      p->N = N;
    }
    else {
      if (p->w_aux.auxp==NULL)
        return csound->InitError(csound, "No data to continue");
    }
    p->first = 0;
    return OK;
}

static int bar_run(CSOUND *csound, BAR* p)
{
    double xofreq = *p->kscan; /* 0.23; */
    double xo, xofrac;
    int xoint;
    int step = p->step;
    int first = p->first;
    int n, N = p->N, rr;
    double *w = p->w, *w1 = p->w1, *w2 = p->w2;
    double s0 = p->s0, s1 = p->s1, s2 = p->s2, t0 = p->t0, t1 = p->t1;
    int bcL = (int)(*p->kbcL+FL(0.5));    /*  boundary condition pair */
    int bcR = (int)(*p->kbcR+FL(0.5));    /*  1: clamped, 2: pivoting, 3: free */
    if ((bcL|bcR)&(~3))
      return csound->PerfError(csound, "Ends but be clamped, piviting or free");

    for (n=0; n<csound->ksmps; n++) {
      /* Fix ends */
      if (bcL==3) {
        w1[1] = 2.0*w1[2]-w1[3];
        w1[0] = 3.0*w1[1]-3*w1[2]+w1[3];
      }
      else if (bcL==1) {
        w1[2] = 0.0;
        w1[3] = 0.0;
      }
      else if (bcL==2) {
        w1[2] = 0.0;
        w1[1] = -w1[3];
      }

      if (bcR==3) {
        w1[N+3] = 2.0*w1[N+2]-w1[N+1];
        w[N+4] = 3.0*w1[N+3]-3.0*w1[N+2]+w1[N+1];
      }
      else if (bcR==1) {
        w1[N+1] = 0.0;
        w1[N+1] = 0.0;
      }
      else if(bcR==2) {
        w1[N+2] = 0.0;
        w1[N+3] = -w1[N+1];
      }

      /* Iterate model */
      for (rr=0; rr<N; rr++) {
        w[rr+2] =
          s0*w1[rr+2] + s1*(w1[rr+3]+w1[rr+1]) +
          s2*(w1[rr+4]+w1[rr]) + t0*w2[rr+2] +
          t1*(w2[rr+3]+w2[rr+1]);
      }
      /*  strike inputs */

      if (first==0) {
        p->first = first = 1;
        for (rr=0; rr<N; rr++) {
          if (fabs(rr/(double)N-*p->ipos)<=*p->iwid) {
/*             csound->Message(csound, "w[%d] = %f -> ", rr+2, w[rr+2]); */
            w[rr+2] += (1.0/csound->esr)*(*p->ivel)*0.5*
              (1.0+cos(PI*fabs(rr/(double)N-(*p->ipos))/(*p->iwid)));
/*            csound->Message(csound, " %f\n", w[rr+2]); */
          }
        }
      }

      /*  readouts */

/*       xo = (1.0/3.0) + 0.5*sin(TWOPI*xofreq*(step+1)/csound->esr); */
      xo = 0.5 + 0.5*sin(TWOPI*xofreq*(step+1)/csound->esr);
      xoint = (int)(xo*N)+2;
      xofrac = xo*N-(int)(xo*N);

/*       csound->Message(csound, "xo = %f (%d %f) w=(%f,%f) ", xo, xoint, xofrac, w[xoint], w[xoint+1]); */
      p->ar[n] = (csound->e0dbfs)*((1.0-xofrac)*w[xoint] + xofrac*w[xoint+1]);
      step++;
      {
        double *ww = w2;
        w2 = w1;
        w1 = w;
        w = ww;
      }
    }
    p->step = step;
    p->w = w; p->w1 = w1; p->w2 = w2;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "barmodel",  S(BAR), 5, "a", "kkiikiiii", (SUBR)bar_init, NULL, (SUBR)bar_run }
};

LINKAGE

