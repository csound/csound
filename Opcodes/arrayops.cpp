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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/
#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <plugin.h>


inline MYFLT logb(MYFLT a, MYFLT b) {
  return log(a)/log(b);
}  

// extern
inline MYFLT frac(MYFLT f) { return std::modf(f, &f); }

// extern
inline MYFLT lim1(MYFLT f) {
  return f > FL(0.0) ? (f < FL(1.0) ? f : FL(1.0)) : FL(0.0);
}

inline MYFLT limx(MYFLT f, MYFLT v1, MYFLT v2) {
  return f > v1 ? (f < v2 ? f : v2) : v1;
}

/** k-rate operator
    kout[] op kin[]
 */
template <MYFLT (*op)(MYFLT)> struct ArrayOp : csnd::Plugin<1, 1> {
  int32_t process(csnd::myfltvec &out, csnd::myfltvec &in) {
    std::transform(in.begin(), in.end(), out.begin(),
                   [](MYFLT f) { return op(f); });
    return OK;
  }

  int32_t init() {
    csnd::myfltvec &out = outargs.myfltvec_data(0);
    csnd::myfltvec &in = inargs.myfltvec_data(0);
    out.init(csound, in.len(), this->insdshead);
    if (!is_perf()) process(out, in);
    return OK;
  }

  int32_t kperf() {
    return process(outargs.myfltvec_data(0), inargs.myfltvec_data(0));
  }
};

/** k-rate binary operator
    kout[] op kin1[], kin2[]
 */
template <MYFLT (*bop)(MYFLT, MYFLT)> struct ArrayOp2 : csnd::Plugin<1, 2> {

  int32_t process(csnd::myfltvec &out, csnd::myfltvec &in1, csnd::myfltvec &in2) {
    std::transform(in1.begin(), in1.end(), in2.begin(), out.begin(),
                   [](MYFLT f1, MYFLT f2) { return bop(f1, f2); });
    return OK;
  }

  int32_t init() {
    csnd::myfltvec &out = outargs.myfltvec_data(0);
    csnd::myfltvec &in1 = inargs.myfltvec_data(0);
    csnd::myfltvec &in2 = inargs.myfltvec_data(1);
    if (UNLIKELY(in2.len() < in1.len()))
      return csound->init_error(Str_noop("second input array is too short\n"));
    out.init(csound, in1.len(), this->insdshead);
    if (!is_perf()) process(out, in1, in2);
    return OK;
  }
  int32_t kperf() {
    return process(outargs.myfltvec_data(0), inargs.myfltvec_data(0),
                   inargs.myfltvec_data(1));
  }
};

/** k-rate binary operator with array and scalar
    kout[] op kin1[], kin2
 */
template <MYFLT (*bop)(MYFLT, MYFLT)> struct ArrayOp3 : csnd::Plugin<1, 2> {

  int32_t process(csnd::myfltvec &out, csnd::myfltvec &in, MYFLT v) {
    for (MYFLT *s = in.begin(), *o = out.begin(); s != in.end(); s++, o++)
      *o = bop(*s, v);
    return OK;
  }

  int32_t init() {
    csnd::myfltvec &out = outargs.myfltvec_data(0);
    csnd::myfltvec &in = inargs.myfltvec_data(0);
    out.init(csound, in.len(), this->insdshead);
    if (!is_perf()) process(out, in, inargs[1]);
    return OK;
  }

  int32_t kperf() {
    return process(outargs.myfltvec_data(0), inargs.myfltvec_data(0),
                   inargs[1]);
  }
};

/** k-rate binary operator with array and two scalar
    kout[] op kin1[], kin2, kin3
 */
template <MYFLT (*trop)(MYFLT, MYFLT, MYFLT)>
struct ArrayOp4 : csnd::Plugin<1, 3> {

  int32_t process(csnd::myfltvec &out, csnd::myfltvec &in, MYFLT v1, MYFLT v2) {
    for (MYFLT *s = in.begin(), *o = out.begin(); s != in.end(); s++, o++)
      *o = trop(*s, v1, v2);
    return OK;
  }

  int32_t init() {
    csnd::myfltvec &out = outargs.myfltvec_data(0);
    csnd::myfltvec &in = inargs.myfltvec_data(0);
    out.init(csound, in.len(), this->insdshead);
    if(!is_perf()) process(out, in, inargs[1], inargs[2]);
    return OK;
  }

  int32_t kperf() {
    return process(outargs.myfltvec_data(0), inargs.myfltvec_data(0), inargs[1],
                   inargs[2]);
  }
};

/** k-rate operator
    kout[] sort[a,d] kin[]
 */
template <typename T> struct ArraySort : csnd::Plugin<1, 1> {
  int32_t process(csnd::myfltvec &out, csnd::myfltvec &in) {
    std::copy(in.begin(), in.end(), out.begin());
    std::sort(out.begin(), out.end(), T());
    return OK;
  }

  int32_t init() {
    csnd::myfltvec &out = outargs.myfltvec_data(0);
    csnd::myfltvec &in = inargs.myfltvec_data(0);
    out.init(csound, in.len(), this->insdshead);
    if (!is_perf()) process(out, in);
    return OK;
  }

  int32_t kperf() {
    return process(outargs.myfltvec_data(0), inargs.myfltvec_data(0));
  }
};

/** dot product
 */
struct Dot : csnd::Plugin<1, 2> {

  MYFLT process(csnd::myfltvec &in1, csnd::myfltvec &in2) {
    return std::inner_product(in1.begin(), in1.end(), in2.begin(), 0.0);
  }

  int32_t init() {
    csnd::myfltvec &in1 = inargs.myfltvec_data(0);
    csnd::myfltvec &in2 = inargs.myfltvec_data(1);
    if (UNLIKELY(in2.len() < in1.len()))
      return csound->init_error(Str_noop("second input array is too short\n"));
    outargs[0] = process(in1, in2);
    return OK;
  }

  int32_t kperf() {
    outargs[0] = process(inargs.myfltvec_data(0), inargs.myfltvec_data(1));
    return OK;
  }
};

template <typename T, int32_t I> struct Accum : csnd::Plugin<1, 1> {

  MYFLT process(csnd::myfltvec &in1) {
    return std::accumulate(in1.begin(), in1.end(), FL(I), T());
  }

  int32_t init() {
    csnd::myfltvec &in1 = inargs.myfltvec_data(0);
    outargs[0] = process(in1);
    return OK;
  }

  int32_t kperf() {
    outargs[0] = process(inargs.myfltvec_data(0));
    return OK;
  }
};


static void onload(csnd::Csound *csound) {
  csnd::plugin<ArrayOp<lim1>>(csound, "limit1", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<lim1>>(csound, "limit1", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::ceil>>(csound, "ceil", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::ceil>>(csound, "ceil", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::floor>>(csound, "floor", "i[]", "i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp<std::floor>>(csound, "floor", "k[]", "k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp<std::round>>(csound, "round", "i[]", "i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp<std::round>>(csound, "round", "k[]", "k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp<std::trunc>>(csound, "int", "i[]", "i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp<std::trunc>>(csound, "int", "k[]", "k[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp<frac>>(csound, "frac", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<frac>>(csound, "frac", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<ArrayOp<std::exp2>>(csound, "powoftwo", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp2>>(csound, "powoftwo", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::fabs>>(csound, "abs", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::fabs>>(csound, "abs", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log2", "i[]", "i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log2", "k[]", "k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log10", "i[]", "i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp<std::log10>>(csound, "log10", "k[]", "k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp<std::log>>(csound, "log", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::log>>(csound, "log", "k[]", "k[]",
                                  csnd::thread::ik);
  csnd::plugin<ArrayOp3<logb>>(csound, "log", "i[]", "i[]i", csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp>>(csound, "exp", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::exp>>(csound, "exp", "k[]", "k[]",
                                  csnd::thread::ik);
  csnd::plugin<ArrayOp<std::sqrt>>(csound, "sqrt", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::sqrt>>(csound, "sqrt", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::cos>>(csound, "cos", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::cos>>(csound, "cos", "k[]", "k[]",
                                  csnd::thread::ik);
  csnd::plugin<ArrayOp<std::sin>>(csound, "sin", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::sin>>(csound, "sin", "k[]", "k[]",
                                  csnd::thread::ik);
  csnd::plugin<ArrayOp<std::tan>>(csound, "tan", "i[]", "i[]", csnd::thread::i);
  csnd::plugin<ArrayOp<std::tan>>(csound, "tan", "k[]", "k[]",
                                  csnd::thread::ik);
  csnd::plugin<ArrayOp<std::acos>>(csound, "cosinv", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::acos>>(csound, "cosinv", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::asin>>(csound, "sininv", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::asin>>(csound, "sininv", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::atan>>(csound, "taninv", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::atan>>(csound, "taninv", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::cosh>>(csound, "cosh", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::cosh>>(csound, "cosh", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::sinh>>(csound, "sinh", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::sinh>>(csound, "sinh", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::tanh>>(csound, "tanh", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::tanh>>(csound, "tanh", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp<std::cbrt>>(csound, "cbrt", "i[]", "i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp<std::cbrt>>(csound, "cbrt", "k[]", "k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::atan2>>(csound, "taninv", "i[]", "i[]i[]",
                                     csnd::thread::i);
  csnd::plugin<ArrayOp2<std::atan2>>(csound, "taninv", "k[]", "k[]k[]",
                                     csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::pow>>(csound, "pow", "i[]", "i[]i[]",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp2<std::pow>>(csound, "pow", "k[]", "k[]k[]",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::hypot>>(csound, "hypot", "i[]", "i[]i[]",
                                     csnd::thread::i);
  csnd::plugin<ArrayOp2<std::hypot>>(csound, "hypot", "k[]", "k[]k[]",
                                     csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::fmod>>(csound, "fmod", "i[]", "i[]i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp2<std::fmod>>(csound, "fmod", "k[]", "k[]k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::fmax>>(csound, "fmax", "i[]", "i[]i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp2<std::fmax>>(csound, "fmax", "k[]", "k[]k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp2<std::fmin>>(csound, "fmin", "i[]", "i[]i[]",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp2<std::fmin>>(csound, "fmin", "k[]", "k[]k[]",
                                    csnd::thread::ik);
  csnd::plugin<ArraySort<std::less<MYFLT>>>(csound, "sorta", "i[]", "i[]",
                                            csnd::thread::i);
  csnd::plugin<ArraySort<std::greater<MYFLT>>>(csound, "sortd", "i[]", "i[]",
                                               csnd::thread::i);
  csnd::plugin<ArraySort<std::less<MYFLT>>>(csound, "sorta", "k[]", "k[]",
                                            csnd::thread::ik);
  csnd::plugin<ArraySort<std::greater<MYFLT>>>(csound, "sortd", "k[]", "k[]",
                                               csnd::thread::ik);
  csnd::plugin<Dot>(csound, "dot", "i", "i[]i[]", csnd::thread::i);
  csnd::plugin<Dot>(csound, "dot", "k", "k[]k[]", csnd::thread::k);
  csnd::plugin<Accum<std::multiplies<MYFLT>, 1>>(csound, "product", "k", "k[]",
                                                 csnd::thread::k);
  csnd::plugin<Accum<std::plus<MYFLT>, 0>>(csound, "sum", "k", "k[]",
                                           csnd::thread::k);
  csnd::plugin<Accum<std::multiplies<MYFLT>, 1>>(csound, "product", "i", "i[]",
                                                 csnd::thread::i);
  csnd::plugin<Accum<std::plus<MYFLT>, 0>>(csound, "sum", "i", "i[]",
                                           csnd::thread::i);
  csnd::plugin<ArrayOp4<limx>>(csound, "limit", "i[]", "i[]ii",
                               csnd::thread::i);
  csnd::plugin<ArrayOp4<limx>>(csound, "limit", "k[]", "k[]kk",
                               csnd::thread::ik);
  csnd::plugin<ArrayOp3<std::pow>>(csound, "pow", "i[]", "i[]i",
                                   csnd::thread::i);
  csnd::plugin<ArrayOp3<std::pow>>(csound, "pow", "k[]", "k[]k",
                                   csnd::thread::ik);
  csnd::plugin<ArrayOp3<std::fmod>>(csound, "fmod", "i[]", "i[]i",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp3<std::fmod>>(csound, "fmod", "k[]", "k[]k",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp3<std::fmax>>(csound, "fmax", "i[]", "i[]i",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp3<std::fmax>>(csound, "fmax", "k[]", "k[]k",
                                    csnd::thread::ik);
  csnd::plugin<ArrayOp3<std::fmin>>(csound, "fmin", "i[]", "i[]i",
                                    csnd::thread::i);
  csnd::plugin<ArrayOp3<std::fmin>>(csound, "fmin", "k[]", "k[]k",
                                    csnd::thread::ik);
}



#ifdef BUILD_PLUGINS
#include <modload.h>
void csnd::on_load(csnd::Csound *csound) {
    onload(csound);
}
#else
extern "C" int32_t arrayops_init_modules(CSOUND *csound) {
    onload((csnd::Csound *)csound);
    return OK;
  }
#endif
