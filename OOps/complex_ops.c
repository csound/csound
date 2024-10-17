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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include "csoundCore.h"
#include "aops.h"
#include "complex_ops.h"
#include <math.h>


int32_t complex_assign(CSOUND *csound, R2CXOP *p){
  p->ans->real =  *p->a;
  p->ans->imag =  *p->b;
  return OK;
}

int32_t complex_init(CSOUND *csound, CXOP *p){
  p->ans->real =  p->a->real; 
  p->ans->imag =  p->a->imag;
  return OK;
}

// complex arithmetic
int32_t complex_add(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real + p->b->real;
  p->ans->imag =  p->a->imag + p->b->imag;
  return OK;
}

int32_t complex_sub(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real - p->b->real;
  p->ans->imag =  p->a->imag - p->b->imag;
  return OK;
}


int32_t complex_prod(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->a->real;
  MYFLT ia = p->a->imag;
  MYFLT rb = p->b->real;
  MYFLT ib = p->b->imag;
  p->ans->real = ra*rb - ia*ib;
  p->ans->imag = ra*ib + ia*rb;
  return OK;
}

int32_t complex_div(CSOUND *csound, CXOP *p) {
  MYFLT ra = p->a->real;
  MYFLT ia = p->a->imag;
  MYFLT rb = p->b->real;
  MYFLT ib = p->b->imag;
  MYFLT den = rb*rb + ib*ib;
  if(den != FL(0.0)) {
    p->ans->real = (ra*rb + ia*ib)/den;
    p->ans->imag = (ra*ib - ia*rb)/den;
  } else {
    csound->Message(csound, "complex div by zero\n");
    return NOTOK;
  }
  return OK;
}

int32_t complex_conj(CSOUND *csound, CXOP *p) {
  p->ans->real =  p->a->real;
  p->ans->imag = - p->a->imag;
  return OK;
}

int32_t complex_abs(CSOUND *csound, CXOP2R *p) {
  *p->ans =  SQRT(p->a->real * p->a->real +
                  p->a->imag * p->a->imag);
  return OK;
}

int32_t complex_arg(CSOUND *csound, CXOP2R *p) {
  *p->ans = (MYFLT) ATAN2(p->a->imag, p->a->real);
  return OK;
}

int32_t complex_real(CSOUND *csound, CXOP2R *p) {
  *p->ans =  p->a->real;
  return OK;
}

int32_t complex_imag(CSOUND *csound, CXOP2R *p) {
  *p->ans =  p->a->imag;
  return OK;
}

int32_t complex_add_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->a;
  ans->real =  cmpx->real + *p->b;
  ans->imag = cmpx->imag;
  return OK;
}

int32_t real_add_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->b;
  ans->real =  cmpx->real + *p->a;
  ans->imag = cmpx->imag;
  return OK;
}

int32_t complex_sub_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->a;
  ans->real =  cmpx->real - *p->b;
  ans->imag = cmpx->imag;
  return OK;
}

int32_t real_sub_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->b;
  ans->real =  cmpx->real - *p->a;
  ans->imag = cmpx->imag;
  return OK;
}


int32_t complex_mul_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->a;
  ans->real =  cmpx->real * *p->b;
  ans->imag = cmpx->imag * *p->b;
  return OK;
}

int32_t real_mul_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->b;
  ans->real =  cmpx->real * *p->a;
  ans->imag = cmpx->imag  * *p->a;
  return OK;
}

int32_t complex_div_real(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->a;
  ans->real =  cmpx->real / *p->b;
  ans->imag = cmpx->imag / *p->b;
  return OK;
}

int32_t real_div_complex(CSOUND *csound, AOP *p) {
  COMPLEXDAT *ans = (COMPLEXDAT *) p->r;
  COMPLEXDAT *cmpx = (COMPLEXDAT *) p->b;
  MYFLT den = cmpx->real*cmpx->real + cmpx->imag*cmpx->imag;
  ans->real =  (*p->a * cmpx->real)/den; 
  ans->imag = - (*p->a  * cmpx->imag)/den;
  return OK;
}




