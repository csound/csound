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
    cs_float       *aout[PLATEREV_MAX_OUTPUTS];
    cs_float       *tabins;
    cs_float       *tabout;
    cs_float       *bndry;
    cs_float       *asp;
    cs_float       *stiff;
    cs_float       *decay;
    cs_float       *loss;
    cs_float       *ain[VARGMAX];
 // Internals
    cs_double       s00, s10, s01, s11, s20, s02, t00, t01, t10;
    uint32_t     nin, nout, Nx, Ny;
    cs_double       *u, *u1, *u2;
    cs_double       *ci, *si, *wi, *cdi, *sdi;
    cs_double       *co, *so, *wo, *cdo, *sdo;
    AUXCH        auxch, state;
    cs_double       L, dy, dt;
    cs_float        *in_param, *out_param;
 } PLATE;


static int32_t platerev_init(CSOUND *csound, PLATE *p)
{
    FUNC *inp, *outp;
    cs_double a = *p->asp;
    cs_double dt = (p->dt = 1.0/CS_ESR); /* time step */
    cs_double stiffness = *p->stiff;
    cs_double decay = *p->decay;
    cs_double loss = *p->loss;
    cs_double nx, ny, dxmin;
    size_t Nx5, Ny5, gridpoints;
    cs_double *state;
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

    cs_double sig = (CS_ESR+CS_ESR)*
                 (POWER(10.0, FL(3.0)*dt/decay)-FL(1.0)); /* loss constant */
    if (UNLIKELY(!isfinite(sig)))
      return csound->InitError(csound, "%s",
                               Str("platerev: decay time is too small"));
    cs_double b2 = loss;
    dxmin = 2.0*sqrt(dt*(b2+hypot(loss, stiffness)));
    nx = floor(1.0/dxmin);
    if (UNLIKELY(!(nx >= 1.0 && nx <= (UINT32_MAX + 0.0) - 5.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: physical parameters produce an "
                                   "invalid grid size"));
    uint32_t Nx = (p->Nx = (uint32_t)nx);
    Nx5 = (size_t)Nx + 5u;
    cs_double dx = 1.0/(cs_double)Nx;
    ny = floor(a*nx);
    if (UNLIKELY(!(ny >= 1.0 && ny <= (UINT32_MAX + 0.0) - 5.0)))
      return csound->InitError(csound, "%s",
                               Str("platerev: aspect ratio produces an invalid "
                                   "grid size"));
    uint32_t Ny = (p->Ny = (uint32_t)ny);
    Ny5 = (size_t)Ny + 5u;
    cs_double dy = (p->dy = *p->asp/Ny);
    cs_double alf = dx/dy;
    cs_double mu = dt*(*p->stiff)*Nx*Nx;
    cs_double mu2 = mu*mu;
    cs_double eta = 1.0/(1.0+sig*dt);
    cs_double V = 2.0*b2*dt*Nx*Nx;

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
                 Nx5 * Ny5 > SIZE_MAX / (3u * sizeof(cs_double))))
      return csound->InitError(csound, "%s",
                               Str("platerev: grid is too large"));
    gridpoints = Nx5 * Ny5;
    csound->AuxAlloc(csound, 3u * gridpoints * sizeof(cs_double), &p->auxch);
    p->u = (cs_double*)p->auxch.auxp;
    p->u1 = p->u + gridpoints;
    p->u2 = p->u1 + gridpoints;

    csound->AuxAlloc(csound,
                     5u * ((size_t)p->nin + p->nout) * sizeof(cs_double),
                     &p->state);
    state = (cs_double*)p->state.auxp;
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
      if (UNLIKELY(!isfinite((cs_double)p->in_param[3*qq+2])))
        return csound->InitError(csound, "%s",
                                 Str("platerev: input phase must be finite"));
      p->ci[qq] = cos((cs_double)p->in_param[3*qq+2]);
      p->si[qq] = sin((cs_double)p->in_param[3*qq+2]);
    }
    for (qq=0; qq<p->nout; qq++) {
      if (UNLIKELY(!isfinite((cs_double)p->out_param[3*qq+2])))
        return csound->InitError(csound, "%s",
                                 Str("platerev: output phase must be finite"));
      p->co[qq] = cos((cs_double)p->out_param[3*qq+2]);
      p->so[qq] = sin((cs_double)p->out_param[3*qq+2]);
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
    cs_float boundary = *p->bndry;
    int32_t bc = (boundary >= FL(0.5) && boundary < FL(1.5) ? 1 :
                  (boundary >= FL(1.5) && boundary < FL(2.5) ? 2 : 0));
    cs_double *u = p->u, *u1 = p->u1, *u2 = p->u2;
    cs_double s00 = p->s00, s10 = p->s10, s01 = p->s01,
           s11 = p->s11, s20 = p->s20, s02 = p->s02,
           t00 = p->t00, t10 = p->t10, t01 = p->t01;
    cs_double dt = p->dt, dy = p->dy;
    uint32_t n, qq;

    if (UNLIKELY(early)) nsmps -= early;
    for (qq=0; qq<(uint32_t)p->nin; qq++) {
      cs_double delta = TWOPI*(cs_double)p->in_param[3*qq]*dt;
      cs_double radius = (cs_double)p->in_param[3*qq+1];
      if (UNLIKELY(!isfinite(delta) || !(radius > -1.0 && radius < 1.0)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("platerev: input frequency must be finite "
                                     "and radius must be in (-1, 1)"));
      p->cdi[qq] = cos(delta);
      p->sdi[qq] = sin(delta);
      p->wi[qq] = p->L*0.5*radius;
    }
    for (qq=0; qq<(uint32_t)p->nout; qq++) {
      cs_double delta = TWOPI*(cs_double)p->out_param[3*qq]*dt;
      cs_double radius = (cs_double)p->out_param[3*qq+1];
      if (UNLIKELY(!isfinite(delta) || !(radius > -1.0 && radius < 1.0)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("platerev: output frequency must be finite "
                                     "and radius must be in (-1, 1)"));
      p->cdo[qq] = cos(delta);
      p->sdo[qq] = sin(delta);
      p->wo[qq] = p->L*0.5*radius;
      if (UNLIKELY(offset)) memset(p->aout[qq], '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) memset(&p->aout[qq][nsmps], '\0', early*sizeof(cs_float));
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
        cs_double w = p->wi[qq];
        cs_double cv = p->ci[qq]*p->cdi[qq] - p->si[qq]*p->sdi[qq];
        cs_double sv = p->ci[qq]*p->sdi[qq] + p->si[qq]*p->cdi[qq];
        cs_double xid = (0.5+w*cv)*Nx;
        cs_double yid = ((*p->asp)*0.5+w*sv)/dy;
        uint32_t xi = (uint32_t)floor(xid)+2u;
        uint32_t yi = (uint32_t)floor(yid)+2u;
        cs_double xf = xid-(cs_double)(xi-2);
        cs_double yf = yid-(cs_double)(yi-2);
        cs_double xyf = xf*yf;
        cs_double uin=(p->ain[qq])[n];
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
        cs_double w = p->wo[qq];
        cs_double cv = p->co[qq]*p->cdo[qq] - p->so[qq]*p->sdo[qq];
        cs_double sv = p->co[qq]*p->sdo[qq] + p->so[qq]*p->cdo[qq];
        cs_double xod = (0.5+w*cv)*Nx;
        cs_double yod = (*p->asp*0.5+w*sv)/dy;
        uint32_t xo = (uint32_t)floor(xod)+2u;
        uint32_t yo = (uint32_t)floor(yod)+2u;
        cs_double xf = xod-(cs_double)(xo-2);
        cs_double yf = yod-(cs_double)(yo-2);
        cs_double xyf = xf*yf;
        size_t uidx;
        p->co[qq] = cv; p->so[qq] = sv;
        uidx = Nx5*yo + xo;
        (p->aout[qq])[n] = (cs_float)((1.0-xf-yf+xyf)*u[uidx]+
                                   (yf-xyf)*u[Nx5+uidx]+
                                   (xf-xyf)*u[1+uidx]+
                                   xyf*u[1+Nx5+uidx])/FL(25.0);
      }
      {
        cs_double *tmp = u2;      /* cycle U*/
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
