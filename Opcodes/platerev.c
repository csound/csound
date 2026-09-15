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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

/* #undef CS_KSMPS */
/* #define CS_KSMPS     (csound->GetKsmps(csound)) */

#define PLATEREV_MAX_OUTPUTS 40

typedef struct {
    OPDS        h;
    MYFLT       *aout[PLATEREV_MAX_OUTPUTS];
    MYFLT       *tabins;
    MYFLT       *tabout;
    MYFLT       *bndry;
    MYFLT       *asp;
    MYFLT       *stiff;
    MYFLT       *decay;
    MYFLT       *loss;
    MYFLT       *ain[VARGMAX];
 // Internals
    double       s00, s10, s01, s11, s20, s02, t00, t01, t10;
    uint32_t     nin, nout, Nx, Ny;
    double       *u, *u1, *u2;
    double       *ci, *si, *wi, *cdi, *sdi;
    double       *co, *so, *wo, *cdo, *sdo;
    AUXCH        auxch, state;
    double       L, dy, dt;
    MYFLT        *in_param, *out_param;
 } PLATE;


static int32_t platerev_init(CSOUND *csound, PLATE *p)
{
    FUNC *inp, *outp;
    double a = *p->asp;
    double dt = (p->dt = 1.0/CS_ESR); /* time step */
    double stiffness = *p->stiff;
    double decay = *p->decay;
    double loss = *p->loss;
    double nx, ny, dxmin;
    size_t Nx5, Ny5, gridpoints;
    double *state;
    uint32_t qq;

    if (UNLIKELY(!(a > 0.0 && a <= 1.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: aspect ratio must be in (0, 1]"));
    if (UNLIKELY(!(decay > 0.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: decay time must be positive"));
    if (UNLIKELY(!(stiffness >= 0.0) || !(loss >= 0.0) ||
                 (stiffness == 0.0 && loss == 0.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: stiffness and loss must define "
                                   "a positive grid spacing"));

    double sig = (CS_ESR+CS_ESR)*
                 (POWER(10.0, FL(3.0)*dt/decay)-FL(1.0)); /* loss constant */
    if (UNLIKELY(!isfinite(sig)))
      return csound->InitError(csound, "%s",
                               Str("platerev: decay time is too small"));
    double b2 = loss;
    dxmin = 2.0*sqrt(dt*(b2+hypot(loss, stiffness)));
    nx = floor(1.0/dxmin);
    if (UNLIKELY(!(nx >= 1.0 && nx <= (double)UINT32_MAX - 5.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: physical parameters produce an "
                                   "invalid grid size"));
    uint32_t Nx = (p->Nx = (uint32_t)nx);
    Nx5 = (size_t)Nx + 5u;
    double dx = 1.0/(double)Nx;
    ny = floor(a*nx);
    if (UNLIKELY(!(ny >= 1.0 && ny <= (double)UINT32_MAX - 5.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: aspect ratio produces an invalid "
                                   "grid size"));
    uint32_t Ny = (p->Ny = (uint32_t)ny);
    Ny5 = (size_t)Ny + 5u;
    double dy = (p->dy = *p->asp/Ny);
    double alf = dx/dy;
    double mu = dt*(*p->stiff)*Nx*Nx;
    double mu2 = mu*mu;
    double eta = 1.0/(1.0+sig*dt);
    double V = 2.0*b2*dt*Nx*Nx;

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
    p->L = a;
    if (UNLIKELY(Nx5 > SIZE_MAX / Ny5 ||
                 Nx5 * Ny5 > SIZE_MAX / (3u * sizeof(double))))
      return csound->InitError(csound, "%s",
                               Str("platerev: grid is too large"));
    gridpoints = Nx5 * Ny5;
    csound->AuxAlloc(csound, 3u * gridpoints * sizeof(double), &p->auxch);
    p->u = (double*)p->auxch.auxp;
    p->u1 = p->u + gridpoints;
    p->u2 = p->u1 + gridpoints;

    csound->AuxAlloc(csound,
                     5u * ((size_t)p->nin + p->nout) * sizeof(double),
                     &p->state);
    state = (double*)p->state.auxp;
    p->ci = state;             state += p->nin;
    p->si = state;             state += p->nin;
    p->wi = state;             state += p->nin;
    p->cdi = state;            state += p->nin;
    p->sdi = state;            state += p->nin;
    p->co = state;             state += p->nout;
    p->so = state;             state += p->nout;
    p->wo = state;             state += p->nout;
    p->cdo = state;            state += p->nout;
    p->sdo = state;
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
      if (UNLIKELY(!isfinite((double)p->in_param[3*qq+2])))
        return csound->InitError(csound, "%s",
                                 Str("platerev: input phase must be finite"));
      p->ci[qq] = cos((double)p->in_param[3*qq+2]);
      p->si[qq] = sin((double)p->in_param[3*qq+2]);
    }
    for (qq=0; qq<p->nout; qq++) {
      if (UNLIKELY(!isfinite((double)p->out_param[3*qq+2])))
        return csound->InitError(csound, "%s",
                                 Str("platerev: output phase must be finite"));
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
    size_t Nx5 = (size_t)Nx + 5u;
    MYFLT boundary = *p->bndry;
    int32_t bc = (boundary >= FL(0.5) && boundary < FL(1.5) ? 1 :
                  (boundary >= FL(1.5) && boundary < FL(2.5) ? 2 : 0));
    double *u = p->u, *u1 = p->u1, *u2 = p->u2;
    double s00 = p->s00, s10 = p->s10, s01 = p->s01,
           s11 = p->s11, s20 = p->s20, s02 = p->s02,
           t00 = p->t00, t10 = p->t10, t01 = p->t01;
    double dt = p->dt, dy = p->dy;
    uint32_t n, qq;

    if (UNLIKELY(early)) nsmps -= early;
    for (qq=0; qq<(uint32_t)p->nin; qq++) {
      double delta = TWOPI*(double)p->in_param[3*qq]*dt;
      double radius = (double)p->in_param[3*qq+1];
      if (UNLIKELY(!isfinite(delta) || !(radius > -1.0 && radius < 1.0)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("platerev: input frequency must be finite "
                                     "and radius must be in (-1, 1)"));
      p->cdi[qq] = cos(delta);
      p->sdi[qq] = sin(delta);
      p->wi[qq] = p->L*0.5*radius;
    }
    for (qq=0; qq<(uint32_t)p->nout; qq++) {
      double delta = TWOPI*(double)p->out_param[3*qq]*dt;
      double radius = (double)p->out_param[3*qq+1];
      if (UNLIKELY(!isfinite(delta) || !(radius > -1.0 && radius < 1.0)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("platerev: output frequency must be finite "
                                     "and radius must be in (-1, 1)"));
      p->cdo[qq] = cos(delta);
      p->sdo[qq] = sin(delta);
      p->wo[qq] = p->L*0.5*radius;
      if (UNLIKELY(offset)) memset(p->aout[qq], '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&p->aout[qq][nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      /* interior grid points*/
      /*u = conv2(u1,S,'same')+conv2(u2,T,'same'); */
      for (j=2; j<Ny+3; j++)  /* Loop from 2,3,...Nf, Nf+1, Nf+2 */
        for (i=2; i<Nx+3; i++) {
          size_t ij = (size_t)i + Nx5*j;
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
        for (j=0; j<Ny+5; j++) {
          size_t jj = Nx5*j;
          u[0+jj] = u[2+jj] = u[Nx+2+jj] = u[Nx+4+jj] = 0.0;
        }
        for (j=2; j<Ny+3; j++) {
          size_t jj = Nx5*j;
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
        for (j=0; j<Ny+5; j++) {
          size_t jj = Nx5*j;
          u[0+jj] = u[2+jj] = u[Nx+2+jj] = u[Nx+4+jj] = 0.0;
        }
        for (j=2; j<Ny+3; j++) {
          size_t jj = Nx5*j;
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
        double w = p->wi[qq];
        double cv = p->ci[qq]*p->cdi[qq] - p->si[qq]*p->sdi[qq];
        double sv = p->ci[qq]*p->sdi[qq] + p->si[qq]*p->cdi[qq];
        double xid = (0.5+w*cv)*Nx;
        double yid = ((*p->asp)*0.5+w*sv)/dy;
        uint32_t xi = (uint32_t)floor(xid)+2u;
        uint32_t yi = (uint32_t)floor(yid)+2u;
        double xf = xid-(double)(xi-2);
        double yf = yid-(double)(yi-2);
        double xyf = xf*yf;
        double uin=(p->ain[qq])[n];
        size_t uidx;
        p->ci[qq] = cv; p->si[qq] = sv;
        uidx = Nx5*yi + xi;
        u[uidx]       += (1.0-xf-yf+xyf)*uin;
        u[1+uidx]     += (xf-xyf)*uin;
        u[1+Nx5+uidx] += xyf*uin;
        u[Nx5+uidx]   += (yf-xyf)*uin;
      }
      /*        %%%% readout */
      for (qq=0; qq<p->nout; qq++) {
        double w = p->wo[qq];
        double cv = p->co[qq]*p->cdo[qq] - p->so[qq]*p->sdo[qq];
        double sv = p->co[qq]*p->sdo[qq] + p->so[qq]*p->cdo[qq];
        double xod = (0.5+w*cv)*Nx;
        double yod = (*p->asp*0.5+w*sv)/dy;
        uint32_t xo = (uint32_t)floor(xod)+2u;
        uint32_t yo = (uint32_t)floor(yod)+2u;
        double xf = xod-(double)(xo-2);
        double yf = yod-(double)(yo-2);
        double xyf = xf*yf;
        size_t uidx;
        p->co[qq] = cv; p->so[qq] = sv;
        uidx = Nx5*yo + xo;
        (p->aout[qq])[n] = (MYFLT)((1.0-xf-yf+xyf)*u[uidx]+
                                   (yf-xyf)*u[Nx5+uidx]+
                                   (xf-xyf)*u[1+uidx]+
                                   xyf*u[1+Nx5+uidx])/FL(25.0);
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
