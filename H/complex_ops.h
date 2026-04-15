/* complex_ops.h: complex operators

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
#include "arrays.h"

typedef struct {
  OPDS    h;
  COMPLEXDAT *ans;
  COMPLEXDAT *a, *b;
} CXOP;

typedef struct {
  OPDS    h;
  MYFLT *ans;
  COMPLEXDAT *a;
} CXOP2R;


typedef struct {
  OPDS    h;
  COMPLEXDAT *ans;
  MYFLT *a, *b;
  MYFLT *isPolar;
} R2CXOP;

typedef struct COPS1 {
  OPDS h;
  ARRAYDAT *out;
  MYFLT *a, *b;
} COPS1;


int32_t complex_assign(CSOUND *csound, R2CXOP *p);
int32_t complex_add(CSOUND *csound, CXOP *p);
int32_t complex_sub(CSOUND *csound, CXOP *p);
int32_t complex_prod(CSOUND *csound, CXOP *p);
int32_t complex_div(CSOUND *csound, CXOP *p);
int32_t complex_conj(CSOUND *csound, CXOP *p);
int32_t complex_abs(CSOUND *csound, CXOP2R *p);
int32_t complex_arg(CSOUND *csound, CXOP2R *p);
int32_t complex_real(CSOUND *csound, CXOP2R *p);
int32_t complex_imag(CSOUND *csound, CXOP2R *p);
int32_t complex_init(CSOUND *csound, CXOP *p);
int32_t real_add_complex(CSOUND *csound, AOP *p);
int32_t real_sub_complex(CSOUND *csound, AOP *p);
int32_t real_mul_complex(CSOUND *csound, AOP *p);
int32_t real_div_complex(CSOUND *csound, AOP *p);
int32_t complex_add_real(CSOUND *csound, AOP *p);
int32_t complex_sub_real(CSOUND *csound, AOP *p);  
int32_t complex_mul_real(CSOUND *csound, AOP *p);
int32_t complex_div_real(CSOUND *csound, AOP *p);
int32_t complex_to_polar(CSOUND *csound, CXOP *p);
int32_t polar_to_complex(CSOUND *csound, CXOP *p);
int32_t complex_log(CSOUND *csound, CXOP *p);
int32_t complex_exp(CSOUND *csound, CXOP *p);
                         

int32_t cops_init(CSOUND *csound, COPS1 *p);
int32_t complex_x_scalar(CSOUND *csound, COPS1 *p);
int32_t complex_plus_scalar(CSOUND *csound, COPS1 *p);
int32_t complex_div_scalar(CSOUND *csound, COPS1 *p);
int32_t complex_minus_scalar(CSOUND *csound, COPS1 *p);
int32_t scalar_minus_complex(CSOUND *csound, COPS1 *p);
int32_t complex_x_complex(CSOUND *csound, COPS1 *p);
int32_t complex_plus_complex(CSOUND *csound, COPS1 *p);
int32_t complex_div_complex(CSOUND *csound, COPS1 *p);
int32_t complexa_minus_complex(CSOUND *csound, COPS1 *p);
int32_t complex_minus_complexa(CSOUND *csound, COPS1 *p);
int32_t complexa_x_complexa(CSOUND *csound, COPS1 *p); 
int32_t complexa_div_complexa(CSOUND *csound, COPS1 *p);
int32_t complexa_plus_complexa(CSOUND *csound, COPS1 *p);
int32_t complexa_sub_complexa(CSOUND *csound, COPS1 *p);
int32_t complex_array_real(CSOUND *csound, COPS1 *p);
int32_t complex_array_imag(CSOUND *csound, COPS1 *p);
int32_t complex_array_abs(CSOUND *csound, COPS1 *p);
int32_t complex_array_arg(CSOUND *csound, COPS1 *p);
int32_t complex_array_conj(CSOUND *csound, COPS1 *p);
int32_t cops_init_r(CSOUND *csound, COPS1 *p);
int32_t complex_array_polar(CSOUND *csound, COPS1 *p);
int32_t complex_array_complex(CSOUND *csound, COPS1 *p);
int32_t cops_init_a(CSOUND *csound, COPS1 *p);
int32_t complex_array_assign(CSOUND *csound, COPS1 *p);
int32_t complex_array_exp(CSOUND *csound, COPS1 *p);
int32_t complex_array_log(CSOUND *csound, COPS1 *p);
int32_t complexa_mulin(CSOUND *csound, COPS1 *p);
int32_t complexa_divin(CSOUND *csound, COPS1 *p);
int32_t complexa_subin(CSOUND *csound, COPS1 *p);
int32_t complexa_addin(CSOUND *csound, COPS1 *p);
int32_t complex_mulin(CSOUND *csound, CXOP *p);
int32_t complex_divin(CSOUND *csound, CXOP *p);
int32_t complex_subin(CSOUND *csound, CXOP *p);
int32_t complex_addin(CSOUND *csound, CXOP *p);
int32_t complexa_mulrealin(CSOUND *csound, COPS1 *p);
int32_t complexa_divrealin(CSOUND *csound, COPS1 *p);
int32_t complexa_subrealin(CSOUND *csound, COPS1 *p);
int32_t complexa_addrealin(CSOUND *csound, COPS1 *p);
int32_t complexa_x_reala(CSOUND *csound, COPS1 *p);
int32_t reala_x_complexa(CSOUND *csound, COPS1 *p);
int32_t complexa_div_reala(CSOUND *csound, COPS1 *p);
int32_t reala_div_complexa(CSOUND *csound, COPS1 *p);
int32_t complexa_plus_reala(CSOUND *csound, COPS1 *p);
int32_t reala_plus_complexa(CSOUND *csound, COPS1 *p);
int32_t complexa_minus_reala(CSOUND *csound, COPS1 *p);
int32_t reala_minus_complexa(CSOUND *csound, COPS1 *p); 
int32_t complex_exp_array(CSOUND *csond, COPS1 *p);
int32_t complex_exp_real(CSOUND *csond, CXOP *p);
