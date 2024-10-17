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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA
*/

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
