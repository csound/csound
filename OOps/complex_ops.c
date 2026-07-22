/* complex_ops.c: complex operators

   Copyright (C) 2024 V Lazzarini

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

#include "csoundCore.h"
#include "aops.h"
#include "complex_ops.h"
#include <math.h>

static inline int32_t smallest(int32_t a, int32_t b, int32_t c) {
  if(c == 0) return a < b ? a : b;
  if(a < b && a < c) return a;
  if(b < a && b < c) return b;
  return c;
}

static inline int32_t smallest2(int32_t a, int32_t b) {
  return a < b ? a : b;
}

/* magnitude from complex number */
static inline MYFLT complex_to_mag(COMPLEXDAT *p) {
  return !p->isPolar ? SQRT(p->real * p->real + p->imag * p->imag) : p->real;
}

/* argument from complex number */
static inline MYFLT complex_to_arg(COMPLEXDAT *p) {
  return !p->isPolar ? ATAN2(p->imag, p->real) : p->imag;
}

/* real part of a polar complex number */
static inline MYFLT polar_to_real(COMPLEXDAT *p) {
  return p->isPolar ? p->real*COS(p->imag) : p->real;
}

/* imag part of a polar complex number */
static inline MYFLT polar_to_imag(COMPLEXDAT *p) {
  return p->isPolar ? p->real*SIN(p->imag) : p->imag;
}

/* complex to polar */
static inline COMPLEXDAT polar(COMPLEXDAT *p) {
  COMPLEXDAT ans;
  ans.real =  complex_to_mag(p);
  ans.imag =  complex_to_arg(p);
  ans.isPolar = 1;
  return ans;
}

/* polar to complex */
static inline COMPLEXDAT complex(COMPLEXDAT *p) {
  COMPLEXDAT ans;
  ans.real =  polar_to_real(p);
  ans.imag =  polar_to_imag(p);
  ans.isPolar = 0;
  return ans;
}

int32_t complex_to_polar(CSOUND *csound, CXOP *p) {
  *p->ans = polar(p->a);
  return OK;
}

int32_t polar_to_complex(CSOUND *csound, CXOP *p) {
  *p->ans = complex(p->a);
  return OK;
}

int32_t complex_assign(CSOUND *csound, R2CXOP *p){
  p->ans->real =  *p->a;
  p->ans->imag =  *p->b;
  p->ans->isPolar = *p->isPolar;
  return OK;
}

int32_t complex_init(CSOUND *csound, CXOP *p){
  memcpy(p->ans, p->a, sizeof(COMPLEXDAT));
  return OK;
}

// complex arithmetic
// supporting polar representation
// returns a polar number only if both operands are polar
static inline int32_t complex_add_rr(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real + p->b->real;
  p->ans->imag =  p->a->imag + p->b->imag;
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_add_rp(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real + polar_to_real(p->b);
  p->ans->imag =  p->a->imag + polar_to_imag(p->b);
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_add_pr(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->b->real + polar_to_real(p->a);
  p->ans->imag =  p->b->imag + polar_to_imag(p->a);
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_add_pp(CSOUND *csound, CXOP *p) {
  p->ans->real =  polar_to_real(p->a) + polar_to_real(p->b);
  p->ans->imag =  polar_to_imag(p->a) + polar_to_imag(p->b);
  *p->ans = polar(p->ans);
  return OK;
}

int32_t complex_add(CSOUND *csound, CXOP *p) {
  return (!p->a->isPolar ? (!p->b->isPolar ? complex_add_rr(csound,p)
                            : complex_add_rp(csound, p))
          : (!p->b->isPolar ? complex_add_pr(csound,p)
             : complex_add_pp(csound, p)));
}

static inline int32_t complex_addin_rr(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->ans->real + p->a->real;
  p->ans->imag =  p->ans->imag + p->a->imag;
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_addin_rp(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->ans->real + polar_to_real(p->a);
  p->ans->imag =  p->ans->imag + polar_to_imag(p->a);
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_addin_pr(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real + polar_to_real(p->ans);
  p->ans->imag =  p->a->imag + polar_to_imag(p->ans);
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_addin_pp(CSOUND *csound, CXOP *p) {
  p->ans->real =  polar_to_real(p->ans) + polar_to_real(p->a);
  p->ans->imag =  polar_to_imag(p->ans) + polar_to_imag(p->a);
  *p->ans = polar(p->ans);
  return OK;
}

int32_t complex_addin(CSOUND *csound, CXOP *p) {
  return (!p->ans->isPolar ? (!p->a->isPolar ? complex_addin_rr(csound,p)
                            : complex_addin_rp(csound, p))
          : (!p->a->isPolar ? complex_addin_pr(csound,p)
             : complex_addin_pp(csound, p)));
}

static int32_t complex_sub_rr(CSOUND *csound, CXOP *p) {
  p->ans->real = p->a->real - p->b->real;
  p->ans->imag =  p->a->imag - p->b->imag;
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_sub_rp(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real - polar_to_real(p->b);
  p->ans->imag =  p->a->imag - polar_to_imag(p->b);
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_sub_pr(CSOUND *csound, CXOP *p) {
  p->ans->real =  polar_to_real(p->a) - p->b->real;
  p->ans->imag =  polar_to_imag(p->a) - p->b->imag;
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_sub_pp(CSOUND *csound, CXOP *p) {
  p->ans->real =  polar_to_real(p->a) - polar_to_real(p->b);
  p->ans->imag =  polar_to_imag(p->a) - polar_to_imag(p->b);
  *p->ans = polar(p->ans);
  return OK;
}

int32_t complex_sub(CSOUND *csound, CXOP *p) {
  return (!p->a->isPolar ? (!p->b->isPolar ? complex_sub_rr(csound,p)
                            : complex_sub_rp(csound, p))
          : (!p->b->isPolar ? complex_sub_pr(csound,p)
             : complex_sub_pp(csound, p)));
}

static int32_t complex_subin_rr(CSOUND *csound, CXOP *p) {
  p->ans->real = p->ans->real - p->a->real;
  p->ans->imag =  p->ans->imag - p->a->imag;
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_subin_rp(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->ans->real - polar_to_real(p->a);
  p->ans->imag =  p->ans->imag - polar_to_imag(p->a);
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_subin_pr(CSOUND *csound, CXOP *p) {
  p->ans->real =  polar_to_real(p->ans) - p->a->real;
  p->ans->imag =  polar_to_imag(p->ans) - p->a->imag;
  p->ans->isPolar = 0;
  return OK;
}

static inline  int32_t complex_subin_pp(CSOUND *csound, CXOP *p) {
  p->ans->real =  polar_to_real(p->ans) - polar_to_real(p->a);
  p->ans->imag =  polar_to_imag(p->ans) - polar_to_imag(p->a);
  *p->ans = polar(p->ans);
  return OK;
}

int32_t complex_subin(CSOUND *csound, CXOP *p) {
  return (!p->ans->isPolar ? (!p->a->isPolar ? complex_subin_rr(csound,p)
                            : complex_subin_rp(csound, p))
          : (!p->a->isPolar ? complex_subin_pr(csound,p)
             : complex_subin_pp(csound, p)));
}

static inline int32_t complex_prod_rr(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->a->real;
  MYFLT ia = p->a->imag;
  MYFLT rb = p->b->real;
  MYFLT ib = p->b->imag;
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_prod_rp(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->a->real;
  MYFLT ia = p->a->imag;
  MYFLT rb = polar_to_real(p->b);
  MYFLT ib = polar_to_imag(p->b);
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_prod_pr(CSOUND *csound, CXOP *p) {
  MYFLT ra = polar_to_real(p->a);
  MYFLT ia = polar_to_imag(p->a);
  MYFLT rb = p->b->real;
  MYFLT ib = p->b->imag;
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_prod_pp(CSOUND *csound, CXOP *p) {
  p->ans->real = p->a->real * p->b->real;
  p->ans->imag = p->a->imag + p->b->imag;
  p->ans->isPolar = 1;
  return OK;
}


int32_t complex_prod(CSOUND *csound, CXOP *p) {
  return (!p->a->isPolar ? (!p->b->isPolar ? complex_prod_rr(csound,p)
                            : complex_prod_rp(csound, p))
          : (!p->b->isPolar ? complex_prod_pr(csound,p)
             : complex_prod_pp(csound, p)));
}


static inline int32_t complex_mulin_rr(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->ans->real;
  MYFLT ia = p->ans->imag;
  MYFLT rb = p->a->real;
  MYFLT ib = p->a->imag;
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_mulin_rp(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->ans->real;
  MYFLT ia = p->ans->imag;
  MYFLT rb = polar_to_real(p->a);
  MYFLT ib = polar_to_imag(p->a);
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_mulin_pr(CSOUND *csound, CXOP *p) {
  MYFLT ra = polar_to_real(p->ans);
  MYFLT ia = polar_to_imag(p->ans);
  MYFLT rb = p->a->real;
  MYFLT ib = p->a->imag;
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_mulin_pp(CSOUND *csound, CXOP *p) {
  p->ans->real = p->ans->real * p->a->real;
  p->ans->imag = p->ans->imag + p->a->imag;
  p->ans->isPolar = 1;
  return OK;
}


int32_t complex_mulin(CSOUND *csound, CXOP *p) {
  return (!p->ans->isPolar ? (!p->a->isPolar ? complex_mulin_rr(csound,p)
                            : complex_mulin_rp(csound, p))
          : (!p->a->isPolar ? complex_mulin_pr(csound,p)
             : complex_mulin_pp(csound, p)));
}

static inline int32_t complex_div_rr(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->a->real;
  MYFLT ia = p->a->imag;
  MYFLT rb = p->b->real;
  MYFLT ib = p->b->imag;
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ia*rb - ra*ib)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_div_rp(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->a->real;
  MYFLT ia = p->a->imag;
  MYFLT rb = polar_to_real(p->b);
  MYFLT ib = polar_to_imag(p->b);
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ia*rb - ra*ib)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_div_pr(CSOUND *csound, CXOP *p) {
  MYFLT ra = polar_to_real(p->a);
  MYFLT ia = polar_to_imag(p->a);
  MYFLT rb = p->b->real;
  MYFLT ib = p->b->imag;
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ia*rb - ra*ib)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_div_pp(CSOUND *csound, CXOP *p) {
  if (p->b->real != FL(0.0)) {
    p->ans->real = p->a->real / p->b->real;
    p->ans->imag = p->a->imag - p->b->imag;
  } else {
    csound->Message(csound, "complex polar div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 1;
  return OK;
}


int32_t complex_div(CSOUND *csound, CXOP *p) {
  return (!p->a->isPolar ? (!p->b->isPolar ? complex_div_rr(csound,p)
                            : complex_div_rp(csound, p))
          : (!p->b->isPolar ? complex_div_pr(csound,p)
             : complex_div_pp(csound, p)));
}

static inline int32_t complex_divin_rr(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->ans->real;
  MYFLT ia = p->ans->imag;
  MYFLT rb = p->a->real;
  MYFLT ib = p->a->imag;
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ia*rb - ra*ib)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_divin_rp(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->ans->real;
  MYFLT ia = p->ans->imag;
  MYFLT rb = polar_to_real(p->a);
  MYFLT ib = polar_to_imag(p->a);
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ia*rb - ra*ib)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_divin_pr(CSOUND *csound, CXOP *p) {
  MYFLT ra = polar_to_real(p->ans);
  MYFLT ia = polar_to_imag(p->ans);
  MYFLT rb = p->a->real;
  MYFLT ib = p->a->imag;
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ia*rb - ra*ib)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 0;
  return OK;
}

static inline int32_t complex_divin_pp(CSOUND *csound, CXOP *p) {
  if (p->b->real != FL(0.0)) {
    p->ans->real = p->ans->real / p->b->real;
    p->ans->imag = p->ans->imag - p->b->imag;
  } else {
    csound->Message(csound, "complex polar div by zero\n");
    return NOTOK;
  }
  p->ans->isPolar = 1;
  return OK;
}


int32_t complex_divin(CSOUND *csound, CXOP *p) {
  return (!p->ans->isPolar ? (!p->a->isPolar ? complex_divin_rr(csound,p)
                            : complex_divin_rp(csound, p))
          : (!p->a->isPolar ? complex_divin_pr(csound,p)
             : complex_divin_pp(csound, p)));
}

int32_t complex_conj(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real;
  p->ans->imag = - p->a->imag;
  p->ans->isPolar = p->a->isPolar;
  return OK;
}

int32_t complex_abs(CSOUND *csound, CXOP2R *p) {
  *p->ans =  !p->a->isPolar ? complex_to_mag(p->a) : p->a->real;
  return OK;
}

int32_t complex_arg(CSOUND *csound, CXOP2R *p) {
  *p->ans =  !p->a->isPolar ?  complex_to_arg(p->a) : p->a->imag;
  return OK;
}

int32_t complex_real(CSOUND *csound, CXOP2R *p) {
  *p->ans = !p->a->isPolar ? p->a->real : polar_to_real(p->a);
  return OK;
}

int32_t complex_imag(CSOUND *csound, CXOP2R *p) {
  *p->ans = !p->a->isPolar ? p->a->imag : polar_to_imag(p->a);
  return OK;
}

// intermixed real & complex
// returns polar if polar input otherwise complex
int32_t complex_add_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT cmpx = !((COMPLEXDAT *) p->a)->isPolar ?
    *((COMPLEXDAT *)p->a) : complex((COMPLEXDAT *)p->a);
  cmpx.real += *p->b;
  *ans = !((COMPLEXDAT *)p->a)->isPolar ? cmpx :
    polar(&cmpx);
  return OK;
}

int32_t real_add_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT cmpx = !((COMPLEXDAT *) p->b)->isPolar ?
    *((COMPLEXDAT *)p->b) : complex((COMPLEXDAT *)p->b);
  cmpx.real += *p->a;
  *ans = !((COMPLEXDAT *)p->b)->isPolar ? cmpx :
    polar(&cmpx);
  return OK;
}

int32_t complex_sub_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT cmpx = !((COMPLEXDAT *) p->a)->isPolar ?
    *((COMPLEXDAT *)p->a) : complex((COMPLEXDAT *)p->a);
  cmpx.real -= *p->b;
  *ans = !((COMPLEXDAT *)p->a)->isPolar ? cmpx :
    polar(&cmpx);
  return OK;
}

int32_t real_sub_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT cmpx = !((COMPLEXDAT *) p->b)->isPolar ?
    *((COMPLEXDAT *)p->b) : complex((COMPLEXDAT *)p->b);
  cmpx.real -= *p->a;
  *ans = !((COMPLEXDAT *)p->b)->isPolar ? cmpx :
    polar(&cmpx);
  return OK;
}


int32_t complex_mul_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->a;
  ans->real =  cmpx->real * *p->b;
  ans->imag = !cmpx->isPolar ? cmpx->imag * *p->b : cmpx->imag;
  ans->isPolar = cmpx->isPolar;
  return OK;
}

int32_t real_mul_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->b;
  ans->real =  cmpx->real * *p->a;
  ans->imag = !cmpx->isPolar ? cmpx->imag  * *p->a : cmpx->imag;
  ans->isPolar = cmpx->isPolar;
  return OK;
}

int32_t complex_div_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->a;
  ans->real =  cmpx->real / *p->b;
  ans->imag = !cmpx->isPolar ? cmpx->imag / *p->b : cmpx->imag;
  ans->isPolar = cmpx->isPolar;
  return OK;
}

int32_t real_div_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->b;
  if(!cmpx->isPolar) {
    MYFLT den = cmpx->real*cmpx->real + cmpx->imag*cmpx->imag;
    ans->real =  (*p->a * cmpx->real)/den;
    ans->imag = - (*p->a  * cmpx->imag)/den;
    ans->isPolar = 0;
  } else {
    ans->real = *p->a / cmpx->real;
    ans->imag = - cmpx->imag;
    ans->isPolar = 1;
  }
  return OK;
}


int32_t complex_exp_real(CSOUND *csond, CXOP *p) {
 COMPLEXDAT *ans = p->ans;
 MYFLT angle =  *((MYFLT *) p->a);
 if(!ans->isPolar) {
   ans->real = COS(angle);
   ans->imag = SIN(angle);
 } else {
   ans->real = 1.;
   ans->imag = angle;
 }
 return OK;
}


int32_t complex_exp(CSOUND *csond, CXOP *p) {
 COMPLEXDAT *ans = p->ans;
 COMPLEXDAT *cmpx =  p->a;
 if(!cmpx->isPolar) {
   ans->real = EXP(cmpx->real)*COS(cmpx->imag);
   ans->imag = EXP(cmpx->real)*SIN(cmpx->imag);
   ans->isPolar = 0;
 } else {
   // exp(Rexp(jw)) = exp(Rcos(w) + Rjsin(w)) = exp(Rcos(w))exp(jRsin(w))
   ans->real = EXP(cmpx->real*COS(cmpx->imag));
   ans->imag = cmpx->real*SIN(cmpx->imag);
   ans->isPolar = 1;
 }
 return OK;
}

int32_t complex_log(CSOUND *csond, CXOP *p) {
 COMPLEXDAT *ans =  p->ans;
 COMPLEXDAT *cmpx =  p->a;
 if(!cmpx->isPolar) {
   ans->real = LOG(HYPOT(cmpx->real,cmpx->imag));
   ans->imag = ATAN2(cmpx->imag,cmpx->real);
   ans->isPolar = 0;
 } else {
   //log(Rexp(jw)) = log(R) + jw = HYPOT(log(R), w)*atan2(w, log(R))
   MYFLT logr = LOG(cmpx->real);
   ans->real = HYPOT(logr, cmpx->imag);
   ans->imag = ATAN2(cmpx->imag, logr);
   ans->isPolar = 1;
 }
 return OK;
}





/** Complex array operators
 */

/** complex array op real scalar
 */

int32_t cops_init(CSOUND *csound, COPS1 *p) {
  int32_t size = 0;

  if(p->INOCOUNT == 2) {
  if(IS_ARRAY_ARG(p->a) &&
     IS_ARRAY_ARG(p->b)){
    ARRAYDAT *aa = (ARRAYDAT *) p->a;
    ARRAYDAT *ab = (ARRAYDAT *) p->b;
    if(aa->dimensions == 0)
      return csound->InitError(csound, "array1 unitialised\n");
    if(ab->dimensions == 0)
      return csound->InitError(csound, "array2 unitialised\n");
    // Use the maximum size, or CS_KSMPS if both are 0
    size = aa->sizes[0] > ab->sizes[0] ? aa->sizes[0] : ab->sizes[0];
    if (size == 0) size = CS_KSMPS;
  } else if (IS_ARRAY_ARG(p->a)) {
    if(((ARRAYDAT *)p->a)->sizes == NULL)
      return csound->InitError(csound, "array unitialised\n");
    size = ((ARRAYDAT *)p->a)->sizes[0];
  }
  else if (IS_ARRAY_ARG(p->b)) {
    if(((ARRAYDAT *)p->b)->sizes == NULL)
      return csound->InitError(csound, "array unitialised\n");
    size = ((ARRAYDAT *)p->b)->sizes[0];
  }
  } else {
    ARRAYDAT *aa = (ARRAYDAT *) p->a;
    if(aa->dimensions == 0)
      return csound->InitError(csound, "array unitialised\n");
    size = aa->sizes[0];
  }
  tabinit(csound, p->out, size, p->h.insdshead);
  return OK;
}



static inline void
cmplx_sc_prod(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real*num;
      out[i].imag = in[i].imag*num;
    } else {
      out[i].real = in[i].real*num;
      out[i].imag = in[i].imag;
    }
  }
}

int32_t complex_x_scalar(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array;
  MYFLT num;
  if(IS_ARRAY_ARG(p->b)) {
    array = (ARRAYDAT *) p->b;
    num = *p->a;
  } else {
    array = (ARRAYDAT *) p->a;
    num = *p->b;
  }
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_sc_prod(out,in,num,len);
  return OK;
}

static inline void
cmplx_sc_div(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real/num;
      out[i].imag = in[i].imag/num;
    } else {
      out[i].real = in[i].real/num;
      out[i].imag = in[i].imag;
    }
  }
}

int32_t complex_div_scalar(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->a;
  MYFLT num = *p->b;
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_sc_div(out,in,num,len);
  return OK;
}

static inline void
cmplx_sc_add(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real + num;
      out[i].imag = in[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re += num;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
}

int32_t complex_plus_scalar(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array;
  MYFLT num;
  if(IS_ARRAY_ARG(p->b)) {
    array = (ARRAYDAT *) p->b;
    num = *p->a;
  } else {
    array = (ARRAYDAT *) p->a;
    num = *p->b;
  }
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_sc_add(out,in,num,len);
  return OK;
}

int32_t scalar_minus_complex(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->b;
  MYFLT num = *p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = num - in[i].real;
      out[i].imag = -in[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re = num - re;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
  return OK;
}

int32_t complex_minus_scalar(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->a;
  MYFLT num = *p->b;
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real - num;
      out[i].imag = in[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re -= num;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
  return OK;
}


static inline void
cmplx_cmplx_prod(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real*num.real - in[i].imag*num.imag;
      out[i].imag = in[i].imag*num.real + in[i].real*num.imag;
    } else {
      out[i].real = in[i].real*num.real;
      out[i].imag = in[i].imag + num.imag;
    }
  }
}



int32_t complex_x_complex(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array;
  COMPLEXDAT num;
  if(IS_ARRAY_ARG(p->b)) {
    array = (ARRAYDAT *) p->b;
    num = *(COMPLEXDAT *)p->a;
  } else {
    array = (ARRAYDAT *) p->a;
    num = *(COMPLEXDAT *)p->b;
  }
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_prod(out,in,num,len);
  return OK;
}




static inline void
cmplx_cmplx_div(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT num, int32_t n) {
  MYFLT d;
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      d = num.real*num.real + num.imag*num.imag;
      out[i].real = (in[i].real*num.real + in[i].imag*num.imag)/d;
      out[i].imag = (in[i].imag*num.real - in[i].real*num.imag)/d;
    } else {
      out[i].real = in[i].real/num.real;
      out[i].imag = in[i].imag - num.imag;
    }
  }
}

int32_t complex_div_complex(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->a;
  COMPLEXDAT num = *(COMPLEXDAT *)p->b;
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_div(out,in,num,len);
  return OK;
}

static inline void
cmplx_cmplx_add(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real + num.real;
      out[i].imag = in[i].imag + num.imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re += num.real;
      im += num.imag;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
}

int32_t complex_plus_complex(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array;
  COMPLEXDAT num;
  if(IS_ARRAY_ARG(p->b)) {
    array = (ARRAYDAT *) p->b;
    num = *(COMPLEXDAT *)p->a;
  } else {
    array = (ARRAYDAT *) p->a;
    num = *(COMPLEXDAT *)p->b;
  }
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_add(out,in,num,len);
  return OK;
}

int32_t complex_minus_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->b;
  COMPLEXDAT num = *(COMPLEXDAT *)p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = num.real - in[i].real;
      out[i].imag = num.imag - in[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re = num.real - re;
      im = num.imag - im;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
  return OK;
}

int32_t complexa_minus_complex(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->a;
  COMPLEXDAT num = *(COMPLEXDAT *)p->b;
  int32_t len =
    smallest2(p->out->sizes[0], array->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real - num.real;
      out[i].imag = in[i].imag - num.imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re -= num.real;
      im -= num.imag;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
  return OK;
}

static inline void
cmplx_cmplx_proda(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT *num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real*num[i].real - in[i].imag*num[i].imag;
      out[i].imag = in[i].imag*num[i].real + in[i].real*num[i].imag;
    } else {
      out[i].real = in[i].real*num[i].real;
      out[i].imag = in[i].imag + num[i].imag;
    }
  }
}


int32_t complexa_x_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_proda(out, in1, in2, len);
  return OK;
}


int32_t complexa_mulin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_proda(out, out, in1, len);
  return OK;
}

static inline void
cmplx_cmplx_diva(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT* num, int32_t n) {
  MYFLT d;
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      d = num[i].real*num[i].real + num[i].imag*num[i].imag;
      out[i].real = (in[i].real*num[i].real + in[i].imag*num[i].imag)/d;
      out[i].imag = (in[i].imag*num[i].real - in[i].real*num[i].imag)/d;
    } else {
      out[i].real = in[i].real/num[i].real;
      out[i].imag = in[i].imag - num[i].imag;
    }
  }
}

int32_t complexa_div_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_diva(out,in1,in2,len);
  return OK;
}

int32_t complexa_divin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_diva(out,out,in1,len);
  return OK;
}

static inline void
cmplx_cmplx_adda(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT* num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real + num[i].real;
      out[i].imag = in[i].imag + num[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re += num[i].real;
      im += num[i].imag;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
}

int32_t complexa_plus_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_adda(out,in1,in2,len);
  return OK;
}

int32_t complexa_addin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_adda(out,out,in1,len);
  return OK;
}

static inline void
cmplx_cmplx_suba(COMPLEXDAT *out, COMPLEXDAT *in, COMPLEXDAT* num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real - num[i].real;
      out[i].imag = in[i].imag - num[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re -= num[i].real;
      im -= num[i].imag;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
}

int32_t complexa_sub_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_suba(out,in1,in2,len);
  return OK;
}

int32_t complexa_subin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;  
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_suba(out,out,in1,len);
  return OK;
}

static inline void
cmplx_real_proda(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT *num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real*num[i]; 
      out[i].imag = in[i].imag*num[i];
    } else {
      out[i].real = in[i].real*num[i];
      out[i].imag = in[i].imag;
    }
  }
}

int32_t complexa_x_reala(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  MYFLT *in2 = (MYFLT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_proda(out, in1, in2, len);
  return OK;
}

int32_t reala_x_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array2->data;
  MYFLT *in2 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_proda(out, in1, in2, len);
  return OK;
}


int32_t complexa_mulrealin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  MYFLT *in1 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_proda(out, out, in1, len);
  return OK;
}


static inline void
cmplx_real_diva(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT *num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real/num[i];
      out[i].imag = in[i].imag/num[i];
    } else {
      out[i].real = in[i].real/num[i];
      out[i].imag = in[i].imag;
    }
  }
}

int32_t complexa_div_reala(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  MYFLT *in2 = (MYFLT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_diva(out,in1,in2,len);
  return OK;
}

int32_t reala_div_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array2->data;
  MYFLT *in2 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_diva(out,in1,in2,len);
  return OK;
}


int32_t complexa_divrealin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  MYFLT *in1 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_diva(out,out,in1,len);
  return OK;
}


static inline void
cmplx_real_suma(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT *num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real+num[i];
      out[i].imag = in[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re = num[i] + re;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
}

int32_t complexa_plus_reala(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  MYFLT *in2 = (MYFLT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_suma(out,in1,in2,len);
  return OK;
}

int32_t reala_plus_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array2->data;
  MYFLT *in2 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_suma(out,in1,in2,len);
  return OK;
}


int32_t complexa_addrealin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len = p->out->sizes[0];
    smallest2(p->out->sizes[0], array1->sizes[0]);
  MYFLT *in1 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_suma(out,out,in1,len);
  return OK;
}


static inline void
cmplx_real_minusa(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT *num, int32_t n) {
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real-num[i];
      out[i].imag = in[i].imag;
    } else {
      MYFLT re, im;
      re = COS(in[i].imag)*in[i].real;
      im = SIN(in[i].imag)*in[i].real;
      re = num[i] - re;
      out[i].real = HYPOT(re,im);
      out[i].imag = ATAN2(im,re);
    }
  }
}


int32_t complexa_minus_reala(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  MYFLT *in2 = (MYFLT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_minusa(out,in1,in2,len);
  return OK;
}

int32_t reala_minus_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1, *array2;
  array2 = (ARRAYDAT *) p->b;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest(p->out->sizes[0], array1->sizes[0], array2->sizes[0]);
  COMPLEXDAT *in1 = (COMPLEXDAT *) array2->data;
  MYFLT *in2 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_minusa(out,in1,in2,len);
  return OK;
}


int32_t complexa_subrealin(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array1;
  array1 = (ARRAYDAT *) p->a;
  int32_t len =
    smallest2(p->out->sizes[0], array1->sizes[0]);
  MYFLT *in1 = (MYFLT *) array1->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_real_minusa(out,out,in1,len);
  return OK;
}


int32_t cops_init_r(CSOUND *csound, COPS1 *p) {
  if(((ARRAYDAT *)p->a)->dimensions) {
    if(IS_ARRAY_ARG(p->out)) {
      int32_t size = ((ARRAYDAT *)p->a)->sizes ? ((ARRAYDAT *)p->a)->sizes[0] : 0;
      if (size <= 0) size = CS_KSMPS; /* fallback to ksmps for audio-like arrays */
      tabinit(csound, p->out, size, p->h.insdshead);
    } else {
      int32_t inSize = ((ARRAYDAT *)p->a)->sizes ? ((ARRAYDAT *)p->a)->sizes[0] : 0;
      if(inSize < CS_KSMPS)
        return csound->InitError(csound, "array length < ksmps\n");
    }
    return OK;
  } else return csound->InitError(csound, "array not initialised\n");
}

int32_t complex_array_real(CSOUND *csound, COPS1 *p) {
  int32_t n = CS_KSMPS;
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  MYFLT *out = (MYFLT *) p->out;
  if(IS_ARRAY_ARG(p->out)) {
    out = (MYFLT *) p->out->data;
    n = p->out->sizes[0];
  }
  for(int i = 0; i < n; i++) {
    out[i] = in[i].isPolar == 0 ? in[i].real :
    in[i].real*COS(in[i].imag);
  }
  return OK;
}

int32_t complex_array_imag(CSOUND *csound, COPS1 *p) {
  int32_t n = CS_KSMPS;
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  MYFLT *out = (MYFLT *) p->out;
  if(IS_ARRAY_ARG(p->out)) {
    out = (MYFLT *) p->out->data;
    n = p->out->sizes[0];
  }
  for(int i = 0; i < n; i++) {
    out[i] = in[i].isPolar == 0 ? in[i].imag :
      in[i].real*SIN(in[i].imag);
  }
  return OK;
}

int32_t complex_array_abs(CSOUND *csound, COPS1 *p) {
  int32_t n = CS_KSMPS;
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  MYFLT *out = (MYFLT *) p->out;
  if(IS_ARRAY_ARG(p->out)) {
    out = (MYFLT *) p->out->data;
    n = p->out->sizes[0];
  }
  for(int i = 0; i < n; i++) {
    out[i] = in[i].isPolar == 0 ?
      HYPOT(in[i].real, in[i].imag) : in[i].real;
  }
  return OK;
}

int32_t complex_array_arg(CSOUND *csound, COPS1 *p) {
  int32_t n = CS_KSMPS;
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  MYFLT *out = (MYFLT *) p->out;
  if(IS_ARRAY_ARG(p->out)) {
    out = (MYFLT *) p->out->data;
    n = p->out->sizes[0];
  }
  for(int i = 0; i < n; i++) {
    out[i] = in[i].isPolar == 0 ?
      ATAN2(in[i].imag, in[i].real) : in[i].imag;
  }
  return OK;
}

int32_t complex_array_conj(CSOUND *csound, COPS1 *p) {
  int32_t n =
    smallest2(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    out[i].real = in[i].real;
    out[i].imag = -in[i].imag;
    out[i].isPolar = in[i].isPolar;
  }
  return OK;
}

int32_t complex_array_polar(CSOUND *csound, COPS1 *p) {
 int32_t n =
    smallest2(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(in[i].isPolar) out[i] = in[i];
    else {
      out[i].real = HYPOT(in[i].real, in[i].imag);
      out[i].imag = ATAN2(in[i].imag, in[i].real);
    }
  }
  return OK;
}

int32_t complex_array_complex(CSOUND *csound, COPS1 *p) {
  int32_t n =
    smallest2(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0]);
  COMPLEXDAT *in = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) out[i] = in[i];
    else {
      out[i].real = in[i].real*COS(in[i].imag);
      out[i].imag = in[i].real*SIN(in[i].imag);
    }
  }
  return OK;
}

int32_t cops_init_a(CSOUND *csound, COPS1 *p) {
  int32_t size = CS_KSMPS;
  if(IS_ARRAY_ARG(p->a)) {
    ARRAYDAT *aa = (ARRAYDAT *) p->a;
    ARRAYDAT *ab = (ARRAYDAT *) p->b;
    if(aa->dimensions == 0)
      return csound->InitError(csound, "array1 unitialised\n");
    if(ab->dimensions == 0)
      return csound->InitError(csound, "array2 unitialised\n");
    // Use the minimum non-zero array size, or CS_KSMPS if both are 0
    if (aa->sizes[0] == 0 && ab->sizes[0] == 0) {
      size = CS_KSMPS;
    } else if (aa->sizes[0] == 0) {
      size = ab->sizes[0];
    } else if (ab->sizes[0] == 0) {
      size = aa->sizes[0];
    } else {
      size = aa->sizes[0] < ab->sizes[0] ? aa->sizes[0] : ab->sizes[0];
    }
  }
  tabinit(csound, p->out, size, p->h.insdshead);
  return OK;
}


int32_t complex_array_assign(CSOUND *csound, COPS1 *p) {
  int32_t n = p->out->sizes[0];
  MYFLT *in1 = p->a;
  MYFLT *in2 = p->b;
  if(IS_ARRAY_ARG(p->a)) {
    in1 = ((ARRAYDAT *) p->a)->data;
    in2 = ((ARRAYDAT *) p->b)->data;
    n =
    smallest(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0],
             ((ARRAYDAT *)p->b)->sizes[0]);
  }
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    out[i].isPolar = 0;
    out[i].real = in1[i];
    out[i].imag = in2[i];
  }
  return OK;
}

int32_t complex_array_exp(CSOUND *csond, COPS1 *p) {
  int32_t n = smallest2(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0]);
  COMPLEXDAT *cmpx = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  COMPLEXDAT *ans = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    if(!cmpx[i].isPolar) {
      ans[i].real = EXP(cmpx[i].real)*COS(cmpx[i].imag);
      ans[i].imag = EXP(cmpx[i].real)*SIN(cmpx[i].imag);
      ans[i].isPolar = 0;
    } else {
      // exp(Rexp(jw)) = exp(Rcos(w) + Rjsin(w)) = exp(Rcos(w))exp(jRsin(w))
      ans[i].real = EXP(cmpx[i].real*COS(cmpx[i].imag));
      ans[i].imag = cmpx[i].real*SIN(cmpx[i].imag);
      ans[i].isPolar = 1;
    }
  }
  return OK;
}

int32_t complex_array_log(CSOUND *csond, COPS1 *p) {
  int32_t n = smallest2(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0]);
  COMPLEXDAT *cmpx = (COMPLEXDAT *)((ARRAYDAT *)p->a)->data;
  COMPLEXDAT *ans = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    if(!cmpx[i].isPolar) {
      ans[i].real = LOG(HYPOT(cmpx[i].real,cmpx[i].imag));
      ans[i].imag = ATAN2(cmpx[i].imag,cmpx[i].real);
      ans[i].isPolar = 0;
    } else {
      //log(Rexp(jw)) = log(R) + jw = HYPOT(log(R), w)*atan2(w, log(R))
      MYFLT logr;
      logr = LOG(cmpx[i].real);
      ans[i].real = HYPOT(logr, cmpx[i].imag);
      ans[i].imag = ATAN2(cmpx[i].imag, logr);
      ans[i].isPolar = 1;
    }
  }
  return OK;
}

int32_t complex_exp_array(CSOUND *csond, COPS1 *p) {
  int32_t n = smallest2(p->out->sizes[0],((ARRAYDAT *)p->a)->sizes[0]);
  MYFLT *angle = (MYFLT *)((ARRAYDAT *)p->a)->data;
  COMPLEXDAT *ans = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < n; i++) {
    if(!ans[i].isPolar) {
      ans[i].real = COS(angle[i]);
      ans[i].imag = SIN(angle[i]);
    } else {
      ans[i].real = 1.;
      ans[i].imag = angle[i];
    }
  }
  return OK;
}


#define WRAPPI(x) while(x >= PI) x -= PI; while(x < -PI) x += PI;

int32_t quadosc_init(CSOUND *csound, QUADOSC *p) {
  MYFLT ifn = -1;
  FUNC *ftp = csound->FTFind(csound, &ifn);
  p->offs = FMAXLEN/4;
  p->tab = ftp;
  p->freq = *p->cps;
  int32_t isPolar = (int32_t) *p->isPolar;
  MYFLT ang = CS_TPIDSR*p->freq;
  if(!isPolar) {
    p->rinc = COS(ang);
    p->iinc = SIN(ang);
  } else 
    p->iinc = ang; 
  if(*p->skip == 0) {
    p->rphs = 1.0;
    p->iphs = 0;
  }
  tabinit(csound, p->out, CS_KSMPS, p->h.insdshead);
  return OK;
}


static
inline MYFLT sintab(FUNC *ftp, int32_t phs) {
  MYFLT *tab = ftp->ftable;
  MYFLT *samp = tab + ((phs>>ftp->lobits) & PHMASK);
  MYFLT frac = PFRAC(phs);
  return *samp + frac*(*(samp+1) - *samp);
}


int32_t quadosc(CSOUND *csound, QUADOSC *p) {
  int32_t n = p->out->sizes[0];
  COMPLEXDAT *ans = (COMPLEXDAT *) p->out->data;
  MYFLT rphs = p->rphs, iphs = p->iphs;
  MYFLT rinc = p->rinc, iinc = p->iinc;
  int32_t isPolar = (int32_t) *p->isPolar;
  int32_t offs = p->offs;
  FUNC *tab = p->tab;

  if(p->freq != *p->cps) {
    MYFLT freq = *p->cps;
    if(!isPolar) {
      int32_t ang = CS_SICVT*freq;
      rinc = p->rinc = sintab(tab, ang + offs);
      iinc = p->iinc = sintab(tab, ang);
    } else {
      MYFLT ang = CS_TPIDSR*p->freq;
      iinc = p->iinc = ang;
    }
    p->freq = freq;
  }
  
  for(int i = 0; i < n; i++) {
    if(!isPolar) {
    ans[i].real = rphs*rinc - iphs*iinc;
    ans[i].imag = rphs*iinc + iphs*rinc;
    rphs = ans[i].real; iphs = ans[i].imag;
    } else {
      ans[i].real = 1.;
      ans[i].imag = iphs + iinc;
      WRAPPI(ans[i].imag);
      iphs = ans[i].imag;
    }
  }
  p->rphs = rphs;
  p->iphs = iphs;  
  return OK;
}


int32_t quadosc_audio(CSOUND *csound, QUADOSC *p) {
  int32_t n = p->out->sizes[0];
  COMPLEXDAT *ans = (COMPLEXDAT *) p->out->data;
  MYFLT rphs = p->rphs, iphs = p->iphs;
  MYFLT rinc, iinc;
  int32_t isPolar = (int32_t) *p->isPolar;
  int32_t offs = p->offs;
  FUNC *tab = p->tab;
  MYFLT *freq = p->cps;

  for(int i = 0; i < n; i++) {
    if(!isPolar) {
    int32_t ang = CS_SICVT*freq[i];
    rinc = sintab(tab, ang + offs);
    iinc = sintab(tab, ang);  
    ans[i].real = rphs*rinc - iphs*iinc;
    ans[i].imag = rphs*iinc + iphs*rinc;
    rphs = ans[i].real; iphs = ans[i].imag;
    } else {
      MYFLT ang = CS_TPIDSR*freq[i];
      iinc = ang;
      ans[i].real = 1.;
      ans[i].imag = iphs + iinc;
      WRAPPI(ans[i].imag);
      iphs = ans[i].imag;
    }
  }
  p->rphs = rphs;
  p->iphs = iphs;  
  return OK;
}
