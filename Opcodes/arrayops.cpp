/*
  arrayops.c: array operators

  Copyright (C) 2017 Victor Lazzarini
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
#include <plugin.h>
#include <algorithm>
#include <cmath>

static inline MYFLT frac(MYFLT f) { return std::modf(f,&f); }

/** i-time, k-rate operator
    kout[] op kin[]
 */
template<MYFLT (*op)(MYFLT)>
struct ArrayOp : csnd::Plugin<1, 1> {
  int init() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    out.init(csound,in.len());
    std::transform(in.begin(), in.end(), out.begin(),
                   [](MYFLT f) { return op(f); });
    return OK;
  }

  int kperf() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    std::transform(in.begin(), in.end(), out.begin(),
                   [](MYFLT f) { return op(f); });

    return OK;
  }
};

/** i-time, k-rate binary operator
    kout[] op kin1[], kin2[]
 */
template<MYFLT (*bop)(MYFLT, MYFLT)>
struct ArrayOp2 : csnd::Plugin<1, 2> {
  int init() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in1 = inargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in2 = inargs.vector_data<MYFLT>(0);
    if(in2.len() < in1.len())
      return csound->init_error(Str("second input array is too short\n"));
    out.init(csound,in1.len());
    std::transform(in1.begin(), in1.end(), in2.begin(), out.begin(),
                   [](MYFLT f1, MYFLT f2) { return bop(f1,f2); });
    return OK;
  }

  int kperf() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in1 = inargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in2 = inargs.vector_data<MYFLT>(0);
    std::transform(in1.begin(), in1.end(), in2.begin(), out.begin(),
                   [](MYFLT f1, MYFLT f2) { return bop(f1,f2); });

    return OK;
  }
};



/** Module creation, initalisation and destruction
 */
extern "C" {
PUBLIC int csoundModuleInit(CSOUND *csound) {
  csnd::plugin<ArrayOp<std::ceil>>(csound, "ceil", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::ceil>>(csound, "ceil", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::floor>>(csound, "floor", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::floor>>(csound, "floor", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::round>>(csound, "round", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::round>>(csound, "round", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::trunc>>(csound, "int", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::trunc>>(csound, "int", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<frac>>(csound, "frac", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<frac>>(csound, "frac", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp2>>(csound, "powoftwo", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp2>>(csound, "powoftwo", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::fabs>>(csound, "abs", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::fabs>>(csound, "abs", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::fabs>>(csound, "abs", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::fabs>>(csound, "abs", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log2", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log2", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log10", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log10", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log>>(csound, "log", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log>>(csound, "log", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp>>(csound, "exp", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp>>(csound, "exp", "k[]", "k[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::sqrt>>(csound, "sqrt", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::sqrt>>(csound, "sqrt", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::cos>>(csound, "cos", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::cos>>(csound, "cos", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::sin>>(csound, "sin", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::sin>>(csound, "sin", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::tan>>(csound, "tan", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::tan>>(csound, "tan", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::acos>>(csound, "cosinv", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::acos>>(csound, "cosinv", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::asin>>(csound, "sininv", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::asin>>(csound, "sininv", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::atan>>(csound, "taninv", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::atan>>(csound, "taninv", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::cosh>>(csound, "cosh", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::cosh>>(csound, "cosh", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::sinh>>(csound, "sinh", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::sinh>>(csound, "sinh", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::tanh>>(csound, "tanh", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::tanh>>(csound, "tanh", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::cbrt>>(csound, "cbrt", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::cbrt>>(csound, "cbrt", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::atan2>>(csound, "taninv", "i[]", "i[]i[]", csnd::thread::i);
  csnd::plugin<ArrayOp2<std::atan2>>(csound, "taninv", "k[]", "k[]k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::pow>>(csound, "pow", "i[]", "i[]i[]", csnd::thread::i);
  csnd::plugin<ArrayOp2<std::pow>>(csound, "pow", "k[]", "k[]k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::hypot>>(csound, "hypot", "i[]", "i[]i[]", csnd::thread::i);
  csnd::plugin<ArrayOp2<std::hypot>>(csound, "hypot", "k[]", "k[]k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::fmod>>(csound, "fmod", "i[]", "i[]i[]", csnd::thread::i);
  csnd::plugin<ArrayOp2<std::fmod>>(csound, "fmod", "k[]", "k[]k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::fmax>>(csound, "fmax", "i[]", "i[]i[]", csnd::thread::i);
  csnd::plugin<ArrayOp2<std::fmax>>(csound, "fmax", "k[]", "k[]k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::fmin>>(csound, "fmin", "i[]", "i[]i[]", csnd::thread::i);
  csnd::plugin<ArrayOp2<std::fmin>>(csound, "fmin", "k[]", "k[]k[]", csnd::thread::ik);
  return 0;
}
PUBLIC int csoundModuleCreate(CSOUND *csound) { return 0; }
PUBLIC int csoundModuleDestroy(CSOUND *csound) { return 0; }
}
