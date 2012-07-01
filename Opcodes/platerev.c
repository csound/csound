/*
    platerev.c:

    Copyright (C) 2006 by Stefan Bilbao
                  2012    John ffitch

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
#include <math.h>

typedef struct {
    OPDS	h;
    MYFLT       *al, *ar;
    MYFLT       *ainl, *ainr;
    MYFLT       *bndry;
    MYFLT       *asp;
    MYFLT       *stiff;
    MYFLT       *decay;
    MYFLT       *loss;
    MYFLT       *lfreq, *lradius, *lphase;
    MYFLT       *rfreq, *rradius, *rphase;
    MYFLT       *olfreq, *olradius, *olphase;
    MYFLT       *orfreq, *orradius, *orphase;
 // Internals
    double       s00, s10, s01, s11, s20, s02, t00, t01, t10;
    int          Nx, Ny;
    double       *u, *u1, *u2;
    AUXCH        auxch;
    double       bc, L, dy, dt;
    double       in_param[6], out_param[6]; 
 } PLATE;


static int platerev_init(CSOUND *csound, PLATE *p)
{
    double a = *p->asp;
    double dt = (p->dt = 1.0/csound->esr); /* time step */
    double sig = (csound->esr+csound->esr)*
      (POWER(10.0, FL(3.0)*dt/(*p->decay))-FL(1.0)); /* loss constant */
    double b2 = *p->loss;
    double dxmin = 2.0*sqrt(dt*(b2+hypot(*p->loss, *p->stiff)));
    int Nx = (p->Nx = floor(1.0/dxmin));
    int Nx5 = Nx+5;
    double dx = 1.0/Nx;
    int Ny = (p->Ny = floor(a*Nx));
    double dy = (p->dy = *p->asp/Ny);
    double alf = dx/dy;
    double mu = dt*(*p->stiff)*Nx*Nx;
    double eta = 1.0/(1.0+sig*dt);
    double V = 2.0*b2*dt*Nx*Nx;

    p->in_param[0] = *p->lfreq; 
    p->in_param[1] = *p->lradius;
    p->in_param[2] = *p->lphase;
    p->in_param[3] = *p->rfreq;
    p->in_param[4] = *p->rradius;
    p->in_param[5] = *p->rphase;
    p->out_param[0] = *p->olfreq; p->out_param[1] = *p->olradius;
    p->out_param[2] = *p->olphase; p->out_param[3] = *p->orfreq;
    p->out_param[4] = *p->orradius; p->out_param[5] = *p->orphase;
    p->L = (a<1.0 ? a : 1.0);
    csound->AuxAlloc(csound, 3*Nx5*(Ny+5)*sizeof(double), &p->auxch);
    p->u = (double*)p->auxch.auxp;
    p->u1 = p->u+Nx5*(Ny+5);
    p->u2 = p->u1+Nx5*(Ny+5);
    p->s00 = 2.0*eta*(1.0-mu*mu*(3.0+4.0*alf*alf+3.0*alf*alf*alf*alf)-
                            V*(1.0+alf*alf));
    p->s10 = (4.0*mu*mu*(1.0+alf*alf)+V)*eta;
    p->s01 = alf*alf*(4.0*mu*mu*(1.0+alf*alf)+V)*eta;
    p->s11 = -2.0*mu*mu*eta*alf*alf;
    p->s20 = -eta*mu*mu;
    p->s02 = -eta*alf*alf*alf*alf*mu*mu;
    p->t00 = (-(1.0-sig*dt)+2.0*V*(1.0+alf*alf))*eta;
    p->t10 = -V*eta;
    p->t01 = -V*eta*alf*alf;
    return OK;
}


static int platerev(CSOUND *csound, PLATE *p)
{
    int i,j, nsmps = csound->ksmps;
    int Ny = p->Ny, Nx = p->Nx;
    int Nx5 = Nx+5;
    int bc =  *p->bndry;
    double *u = p->u, *u1 = p->u1, *u2 = p->u2;
    double s00 = p->s00, s10 = p->s10, s01 = p->s01,
      s11 = p->s11, s20 = p->s20, s02 = p->s02, 
      t00 = p->t00, t10 = p->t10, t01 = p->t01;
    double dt = p->dt, dy = p->dy;
    int n, qq;
    double uin[2];

    for (n=0; n<nsmps; n++) {
      /* interior grid points*/
      /*u = conv2(u1,S,'same')+conv2(u2,T,'same'); */
      for (j=2; j<Ny+3; j++)  /* Loop from 2,3,...Nf, Nf+1, Nf+2 */
         for (i=2; i<Nx+3; i++) {
           int ij = i+Nx5*j;
           u[ij] = s00*u1[ij]+
                   s10*(u1[ij-Nx5]+u1[ij+Nx5])+
                   s20*(u1[ij-2*Nx5]+u1[ij+2*Nx5])+
                   s01*(u1[ij-1]+u1[ij+1])+
                   s02*(u1[ij-2]+u1[ij+2])+
                   s11*(u1[ij-1-Nx5]+u1[ij+1+Nx5]+
                        u1[ij-1+Nx5]+u1[ij+1-Nx5]);
           u[ij] += t00*u2[ij]+
                    t10*(u2[ij-Nx5]+u2[ij+Nx5])+
                    t01*(u2[ij-1]+u2[ij+1]);
         }
                                /* boundary grid points*/
       if (bc==1) {             /* clamped*/
         int jj = Nx5*j;
         for (j=0; j<Ny+5; j++) {
           u[0+jj] = u[2+jj] = u[Nx+2+jj] = u[Nx+4+jj] = 0.0;
         }
         for (j=2; j<Ny+3; j++) {
           u[1+jj] = u[3+jj];
           u[Nx+3+jj] = u[Nx+1+jj];
         }
         for (i=0; i<Nx+5; i++) {
           u[i+Nx5*0] = u[i+Nx5*2] = u[i+Nx5*(Ny+2)] = u[i+Nx5*(Ny+4)] = 0.0;
         }
         for (i=2; i<Nx+3; i++) {
           u[i+Nx5*1] = u[i+Nx5*3]; 
           u[i+Nx5*(Ny+3)] = u[i+Nx5*(Ny+1)]; 
         }
         u[1+Nx5*1] = u[1+Nx5*(Ny+3)] = u[Nx+3+Nx5*1] = u[Nx+3+Nx5*(Ny+3)] = 0.0;
       }
       else if (bc==2) {           /* pivoting*/
         int jj = Nx5*j;
         for (j=0; j<Ny+5; j++) {
           u[0+jj] = u[2+jj] = u[Nx+2+jj] = u[Nx+4+jj] = 0.0;
         }
         for (j=2; j<Ny+3; j++) {
           u[1+jj] = -u[3+jj];
           u[Nx+3+jj] = -u[Nx+1+jj];
         }
         for (i=0; i<Nx+5; i++) {
           u[i+Nx5*0] = u[i+Nx5*2] = u[i+Nx5*(Ny+2)] = u[i+Nx5*(Ny+4)] = 0.0;
         }
         for (i=2; i<Nx+3; i++) {
           u[i+Nx5*1] = -u[i+Nx5*3]; 
           u[i+Nx5*(Ny+3)] = -u[i+Nx5*(Ny+1)]; 
         }
       }
       /* insert excitation*/
       uin[0]=p->ainl[n]; 
       uin[1]=p->ainr[n]; 
       for (qq=0; qq<2; qq++) {
         double w = p->L*0.5*p->in_param[3*qq+1];
         double v = 2.0*PI*p->in_param[3*qq]*(n+1)*dt+p->in_param[3*qq+2];
         double xid = 0.5+w*cos(v);
         double yid = (*p->asp)*0.5+w*sin(v);
         int xi = (int)(floor(xid*Nx))+2;
         int yi = (int)(floor(yid/dy))+2;
         double xf = xid*Nx-(double)(xi-2);
         double yf = yid/dy-(double)(yi-2);
         double xyf = xf*yf;
         yi = Nx5*yi;
         u[xi+yi]       += (1.0-xf-yf+xyf)*uin[qq];
         u[xi+1+yi]     += (xf-xyf)*uin[qq];
         u[xi+1+Nx5+yi] += xyf*uin[qq];
         u[xi+Nx5+yi]   += (yf-xyf)*uin[qq];
       }
       //        %%%% readout */
        for (qq=0; qq<2; qq++) {
          double w = (p->L*0.5)*p->out_param[3*qq+1];
          double v = 2.0*PI*p->out_param[3*qq]*(n+1)*dt+p->out_param[3*qq+2];
          double xod = 0.5+w*cos(v);
          double yod = *p->asp*0.5+w*sin(v);
          int xo = (int)(floor(xod*Nx))+2;
          int yo = (int)(floor(yod/dy))+2;
          double xf = xod*Nx-(double)(xo-2);
          double yf = yod/dy-(double)(yo-2);
          double xyf = xf*yf;
          yo = yo*Nx5;
          (qq==0?p->al:p->ar)[n] = ((1.0-xf-yf+xyf)*u[xo+yo]+
                                    (yf-xyf)*u[xo+Nx5+yo]+
                                    (xf-xyf)*u[xo+1+yo]+
                                    xyf*u[xo+1+Nx5+yo])/5.0;
        }
       {
         double *tmp = u2;      /* cycle U*/
         u2 = u1;
         u1 = u;
         u = tmp;
       }
    }
    p->u = u; p->u1 = u1; p->u2 = u2;
    return OK;
}

static OENTRY localops[] = {
  { "platerev", sizeof(PLATE), 5, "aa", "aakiiiiiiiiiiiiiiii",
    (SUBR) platerev_init, NULL, (SUBR) platerev
  },
};

LINKAGE
