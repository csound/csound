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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

/* #undef CS_KSMPS */
/* #define CS_KSMPS     (csound->GetKsmps(csound)) */

typedef struct {
    OPDS        h;
    MYFLT       *aout[40];
    MYFLT       *tabins;
    MYFLT       *tabout;
    MYFLT       *bndry;
    MYFLT       *asp;
    MYFLT       *stiff;
    MYFLT       *decay;
    MYFLT       *loss;
    MYFLT       *ain[40];
 // Internals
    double       s00, s10, s01, s11, s20, s02, t00, t01, t10;
    uint32_t     nin, nout, Nx, Ny;
    double       *u, *u1, *u2;
    AUXCH        auxch;
    double       L, dy, dt;
    MYFLT        *in_param, *out_param;
    double       ci[40], si[40], co[40], so[40];
 } PLATE;


static int32_t platerev_init(CSOUND *csound, PLATE *p)
{
    FUNC *inp, *outp;
    double a = *p->asp;
    double dt = (p->dt = 1.0/CS_ESR); /* time step */
    double sig = (CS_ESR+CS_ESR)*
                 (POWER(10.0, FL(3.0)*dt/(*p->decay))-FL(1.0)); /* loss constant */
    double b2 = *p->loss;
    double dxmin = 2.0*sqrt(dt*(b2+hypot(*p->loss, *p->stiff)));
    uint32_t Nx = (p->Nx = (uint32_t)floor(1.0/dxmin));
    uint32_t Nx5 = Nx+5;
    double dx = 1.0/(double)Nx;
    uint32_t Ny = (p->Ny = (uint32_t)floor(a*Nx));
    double dy = (p->dy = *p->asp/Ny);
    double alf = dx/dy;
    double mu = dt*(*p->stiff)*Nx*Nx;
    double mu2 = mu*mu;
    double eta = 1.0/(1.0+sig*dt);
    double V = 2.0*b2*dt*Nx*Nx;
    uint32_t qq;

    p->nin = (int32_t) (p->INOCOUNT) - 7; p->nout = (int32_t) (p->OUTOCOUNT);
    if (UNLIKELY((inp = csound->FTFind(csound,p->tabins)) == NULL ||
                 inp->flen < (uint32_t)3*p->nin)) {
      return csound->InitError(csound, "%s",
                               Str("Missing input table or too short"));
    }
    if (UNLIKELY((outp = csound->FTFind(csound,p->tabout)) == NULL ||
                 outp->flen < (uint32_t)3*p->nout)) {
      return csound->InitError(csound, "%s",
                               Str("Missing output table or too short"));
    }
    p->in_param = inp->ftable;
    p->out_param = outp->ftable;
    p->L = (a<1.0 ? a : 1.0);
    csound->AuxAlloc(csound, 3*Nx5*(Ny+5)*sizeof(double), &p->auxch);
    p->u = (double*)p->auxch.auxp;
    p->u1 = p->u+Nx5*(Ny+5);
    p->u2 = p->u1+Nx5*(Ny+5);
    p->s00 = 2.0*eta*(1.0-mu2*(3.0+4.0*alf*alf+3.0*alf*alf*alf*alf)-
                            V*(1.0+alf*alf));
    p->s10 = (4.0*mu2*(1.0+alf*alf)+V)*eta;
    p->s01 = alf*alf*(4.0*mu2*(1.0+alf*alf)+V)*eta;
    p->s11 = -2.0*mu2*eta*alf*alf;
    p->s02 = (p->s20 = -eta*mu2)*alf*alf*alf*alf;
    p->t00 = (-(1.0-sig*dt)+2.0*V*(1.0+alf*alf))*eta;
    p->t10 = -V*eta;
    p->t01 = -V*eta*alf*alf;
    for (qq=0; qq<p->nin; qq++) {
      p->ci[qq] = cos((double)p->in_param[3*qq+2]);
      p->si[qq] = sin((double)p->in_param[3*qq+2]);
    }
    for (qq=0; qq<p->nout; qq++) {
      p->co[qq] = cos((double)p->out_param[3*qq+2]);
      p->so[qq] = sin((double)p->out_param[3*qq+2]);
    }

    return OK;
}


static int32_t platerev(CSOUND *csound, PLATE *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, j, nsmps = CS_KSMPS;
    uint32_t Ny = p->Ny, Nx = p->Nx;
    uint32_t Nx5 = Nx+5;
    int32_t bc =  (int32_t) MYFLT2LONG(*p->bndry);
    double *u = p->u, *u1 = p->u1, *u2 = p->u2;
    double s00 = p->s00, s10 = p->s10, s01 = p->s01,
           s11 = p->s11, s20 = p->s20, s02 = p->s02,
           t00 = p->t00, t10 = p->t10, t01 = p->t01;
    double dt = p->dt, dy = p->dy;
    uint32_t n, qq;
    double wi[40], wo[40], sdi[40], cdi[40], sdo[40], cdo[40];

    if (UNLIKELY(early)) nsmps -= early;
    for (qq=0; qq<(uint32_t)p->nin; qq++) {
      double delta = TWOPI*(double)p->in_param[3*qq]*dt;
      cdi[qq] = cos(delta);
      sdi[qq] = sin(delta);
      wi[qq] = p->L*0.5*(double)p->in_param[3*qq+1];
      delta = TWOPI*(double)p->out_param[3*qq]*dt;
      cdo[qq] = cos(delta);
      sdo[qq] = sin(delta);
      wo[qq] = (p->L*0.5)*(double)p->out_param[3*qq+1];
      if (UNLIKELY(offset)) memset(p->aout[qq], '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&p->aout[qq][nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      /* interior grid points*/
      /*u = conv2(u1,S,'same')+conv2(u2,T,'same'); */
      for (j=2; j<Ny+3; j++)  /* Loop from 2,3,...Nf, Nf+1, Nf+2 */
        for (i=2; i<Nx+3; i++) {
          int32_t ij = i+Nx5*j;
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
        int32_t jj = Nx5*j;
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
        int32_t jj = Nx5*j;
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
      /* else completely free */
      /* insert excitation*/
      for (qq=0; qq<p->nin; qq++) {
        double w = wi[qq];
        double cv = p->ci[qq]*cdi[qq] - p->si[qq]*sdi[qq];
        double sv = p->ci[qq]*sdi[qq] + p->si[qq]*cdi[qq];
        double xid = (0.5+w*cv)*Nx;
        double yid = ((*p->asp)*0.5+w*sv)/dy;
        int32_t xi = (int32_t)(floor(xid))+2;
        int32_t yi = (int32_t)(floor(yid))+2;
        double xf = xid-(double)(xi-2);
        double yf = yid-(double)(yi-2);
        double xyf = xf*yf;
        double uin=(p->ain[qq])[n];
        p->ci[qq] = cv; p->si[qq] = sv;
        yi = Nx5*yi + xi;
        u[yi]       += (1.0-xf-yf+xyf)*uin;
        u[1+yi]     += (xf-xyf)*uin;
        u[1+Nx5+yi] += xyf*uin;
        u[Nx5+yi]   += (yf-xyf)*uin;
      }
      /*        %%%% readout */
      for (qq=0; qq<p->nout; qq++) {
        double w = wo[qq];
        double cv = p->co[qq]*cdo[qq] - p->so[qq]*sdo[qq];
        double sv = p->co[qq]*sdo[qq] + p->so[qq]*cdo[qq];
        double xod = (0.5+w*cv)*Nx;
        double yod = (*p->asp*0.5+w*sv)/dy;
        int32_t xo = (int32_t)(floor(xod))+2;
        int32_t yo = (int32_t)(floor(yod))+2;
        double xf = xod-(double)(xo-2);
        double yf = yod-(double)(yo-2);
        double xyf = xf*yf;
        p->co[qq] = cv; p->so[qq] = sv;
        yo = yo*Nx5 + xo;
        (p->aout[qq])[n] = (MYFLT)((1.0-xf-yf+xyf)*u[yo]+
                                   (yf-xyf)*u[Nx5+yo]+
                                   (xf-xyf)*u[1+yo]+
                                   xyf*u[1+Nx5+yo])/FL(25.0);
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

static OENTRY platerev_localops[] =
  {
   { "platerev", sizeof(PLATE), 0,  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
     "iikiiiiy",
     (SUBR) platerev_init, (SUBR) platerev
  },
};

LINKAGE_BUILTIN(platerev_localops)
