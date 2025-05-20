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


static inline int32_t complex_div_rr(CSOUND *csound, CXOP *p) {
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
    p->ans->imag = (ra*ib - ia*rb)/den;
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
    p->ans->imag = (ra*ib - ia*rb)/den;
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


/** Complex array operators
 */

/** complex array op real scalar 
 */

int32_t cops_init(CSOUND *csound, COPS1 *p) {
  int32_t size;
  if(IS_ARRAY_ARG(p->a) &&
     IS_ARRAY_ARG(p->b)){
    ARRAYDAT *aa = (ARRAYDAT *) p->a; 
    ARRAYDAT *ab = (ARRAYDAT *) p->b;
    if(aa->dimensions == 0) return csound->InitError(csound, "array1 unitialised\n");
    if(ab->dimensions == 0) return csound->InitError(csound, "array2 unitialised\n");
    size = aa->sizes[0] < ab->sizes[0] ? aa->sizes[0] : ab->sizes[0]; 
  } else if (IS_ARRAY_ARG(p->a)) {
    if(((ARRAYDAT *)p->a)->sizes == NULL) return csound->InitError(csound, "array unitialised\n");
    size = ((ARRAYDAT *)p->a)->sizes[0];
  }
  else if (IS_ARRAY_ARG(p->b)) {
    if(((ARRAYDAT *)p->b)->sizes == NULL) return csound->InitError(csound, "array unitialised\n");
    size = ((ARRAYDAT *)p->b)->sizes[0];
  }
  else size = 1;
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
  int32_t len = p->out->sizes[0];
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_sc_div(out,in,num,len);
  return OK;
}

static inline void
cmplx_sc_add(COMPLEXDAT *out, COMPLEXDAT *in, MYFLT num, int32_t n) {
  for(int i = 0; i < n; i++) {
    if(!in[i].isPolar) {
      out[i].isPolar = in[i].isPolar;
      out[i].real = in[i].real + num;
      out[i].imag = in[i].imag;
    } else {
      MYFLT re, im;
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_sc_add(out,in,num,len);
  return OK;
}

int32_t scalar_minus_complex(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->b;
  MYFLT num = *p->a;
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = num - in[i].real;
      out[i].imag = num;
    } else {
      MYFLT re, im;
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real - num;
      out[i].imag = in[i].imag;
    } else {
      MYFLT re, im;
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
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
  int32_t len = p->out->sizes[0];
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
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_add(out,in,num,len);
  return OK;
}

int32_t complex_minus_complexa(CSOUND *csound, COPS1 *p) {
  ARRAYDAT *array = (ARRAYDAT *) p->b;
  COMPLEXDAT num = *(COMPLEXDAT *)p->a;
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = num.real - in[i].real;
      out[i].imag = num.imag - in[i].imag;
    } else {
      MYFLT re, im;
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in = (COMPLEXDAT *) array->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  for(int i = 0; i < len; i++) {
    out[i].isPolar = in[i].isPolar;
    if(!in[i].isPolar) {
      out[i].real = in[i].real - num.real;
      out[i].imag = in[i].imag - num.imag;
    } else {
      MYFLT re, im;
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_proda(out,in1,in2,len);
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_diva(out,in1,in2,len);
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
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_adda(out,in1,in2,len);
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
      re = cos(in[i].imag)*in[i].real;  
      im = sin(in[i].imag)*in[i].real;
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
  int32_t len = p->out->sizes[0];
  COMPLEXDAT *in1 = (COMPLEXDAT *) array1->data;
  COMPLEXDAT *in2 = (COMPLEXDAT *) array2->data;
  COMPLEXDAT *out = (COMPLEXDAT *) p->out->data;
  cmplx_cmplx_suba(out,in1,in2,len);
  return OK;
}
