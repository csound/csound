#include "csdl.h"
#include <math.h>

/* %% bar sound synthesis script */

/* %%%%%%%%%%%%%%%%% some input parameters */

/*  Note: each row of eventlist contains: time of strike (s), position of strike */
/*  along bar (0 - 1), normalized strike velocity, and spatial width of strike.  */

typedef struct EVNT {
  double *time;
  double *pos;
  double *vel;
  double *wid;
} EVENT;

typedef struct {
    OPDS        h;
    MYFLT       *ar;                  /* Output */

    MYFLT       *ibcL, *ibcR, *iK, *ib, *kscan, *iT30;
    EVENT       eventlist[10];

    double      *w, *w1, *w2;
    int         eventnum, eventindex;
    int         step;
    double      s0, s1, s2, t0, t1;
    int         bcL, bcR, N;
} BAR;

int bar_init(CSOUND *csound, BAR *p)
{
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

      /* %%%%%%%%%%%%%%%%%%% create event strike profiles */
    p->eventnum = 0;
    p->eventindex = (int)(*p->eventlist[0].time*csound->esr);

    /* %%%%%%%%%%%%%%%%%%%%% create grid functions */
    
    p->w = (double*)calloc(N+5,sizeof(double)); 
    p->w1 = (double*)calloc(N+5,sizeof(double)); 
    p->w2 = (double*)calloc(N+5,sizeof(double)); 

    p->step = 0;
    p->bcL = (int)*p->ibcL;    /*  boundary condition pair */
    p->bcR = (int)*p->ibcR;    /*  1: clamped, 2: pivoting, 3: free */
    p->N = N;
    return OK;
}

int bar_run(CSOUND *csound, BAR* p)
{
    double xofreq = *p->kscan; /* 0.23; */
    double xo, xofrac;
    int xoint;
    int bcL = p->bcL, bcR = p->bcR;
    int step = p->step;
    int n, N = p->N, rr;
    double *w = p->w, *w1 = p->w1, *w2 = p->w2;
    double s0 = p->s0, s1 = p->s1, s2 = p->s2, t0 = p->t0, t1 = p->t1;

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

      if (step==p->eventindex) {
/*         csound->Message(csound, "Strike %d(%f %f %f %f) at step %d\n", */
/*                         p->eventnum,  */
/*                         *(p->eventlist[p->eventnum].time), */
/*                         *(p->eventlist[p->eventnum].pos), */
/*                         *(p->eventlist[p->eventnum].vel), */
/*                         *(p->eventlist[p->eventnum].wid), */
/*                         step); */
        for (rr=0; rr<N; rr++) {
          if (fabs(rr/(double)N-*p->eventlist[p->eventnum].pos)<=*p->eventlist[p->eventnum].wid) {
/*             csound->Message(csound, "w[%d] = %f -> ", rr+2, w[rr+2]); */
            w[rr+2] += (1.0/csound->esr)*(*(p->eventlist[p->eventnum].vel))*0.5*
            (1.0+cos(PI*fabs(rr/(double)N-*(p->eventlist[p->eventnum].pos))/
                     (*p->eventlist[p->eventnum].wid)));
/*            csound->Message(csound, " %f\n", w[rr+2]); */
          }
        }
        p->eventindex = *p->eventlist[++p->eventnum].time*csound->esr;
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
{ "barmodel",  S(BAR), 5, "a", "iiiikiz", (SUBR)bar_init, NULL, (SUBR)bar_run }
};

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    OENTRY  *ep = (OENTRY*) &(localops[0]);
    int     err = 0;

    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname, ep->dsblksiz, ep->thread,
                                  ep->outypes, ep->intypes,
                                  (int (*)(CSOUND *, void*)) ep->iopadr,
                                  (int (*)(CSOUND *, void*)) ep->kopadr,
                                  (int (*)(CSOUND *, void*)) ep->aopadr);
      ep++;
    }
    return err;
}

