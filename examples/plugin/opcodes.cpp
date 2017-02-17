/**
  opcodes.cpp
  C++ plugin opcode interface examples

  (c) Victor Lazzarini, 2017

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
#include <atomic>
#include <iostream>
#include <plugin.h>
#include <random>

/** i-time plugin opcode example
    with 1 output and 1 input \n
    iout simple iin
 */
struct Simplei : csnd::Plugin<1, 1> {
  int init() {
    outargs[0] = inargs[0];
    return OK;
  }
};

/** k-rate plugin opcode example
    with 1 output and 1 input \n
    kout simple kin
 */
struct Simplek : csnd::Plugin<1, 1> {
  int kperf() {
    outargs[0] = inargs[0];
    return OK;
  }
};

/** a-rate plugin opcode example
    with 1 output and 1 input \n
    aout simple ain
 */
struct Simplea : csnd::Plugin<1, 1> {
  int aperf() {
    nsmps = insdshead->ksmps;
    std::copy(inargs.data(0), inargs.data(0) + nsmps, outargs.data(0));
    return OK;
  }
};

/** k-rate numeric array example
    with 1 output and 1 input \n
    kout[] simple kin[] \n\n
    with a minimal deinit example
 */
struct SimpleArray : csnd::Plugin<1, 1> {
  int init() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    out.init(csound, in.len());
    csound->plugin_deinit(this);
    return OK;
  }

  int deinit() {
    /** nothing to do, just announce itself */
    csound->message("deinit called");
    return OK;
  }

  int kperf() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    std::copy(in.begin(), in.end(), out.begin());
    return OK;
  }
};

/** a-rate numeric array example
    with 1 output and 1 input \n
    aout[] simple ain[] \n\n
    NB: in this case, each
 */
struct SimpleArrayA : csnd::Plugin<1, 1> {
  int init() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    out.init(csound, in.len());
    return OK;
  }

  int aperf() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    // copy each a-var ksmps vector in turn
    // NB: copying the whole memory block
    // from in.begin() to in.end() also works
    for (int i = 0; i < in.len(); i++)
      std::copy(in.begin() + i * in.elem_offset(),
                in.begin() + (i + 1) * in.elem_offset(),
                out.begin() + i * in.elem_offset());
    return OK;
  }
};

/** i-time string plugin opcode example
    with only 1 input \n
    tprint Sin
 */
struct Tprint : csnd::Plugin<0, 1> {
  int init() {
    csound->message(inargs.str_data(0).data);
    return OK;
  }
};

/** a-rate plugin opcode example: delay line
    with 1 output and 2 inputs (a,i) \n
    asig delayline ain,idel
 */
struct DelayLine : csnd::Plugin<1, 2> {
  csnd::AuxMem<MYFLT> delay;
  csnd::AuxMem<MYFLT>::iterator iter;

  int init() {
    delay.allocate(csound, csound->sr() * inargs[1]);
    iter = delay.begin();
    return OK;
  }

  int aperf() {
    MYFLT *out = outargs.data(0);
    MYFLT *in = inargs.data(0);

    sa_offset(out);
    for (uint32_t i = offset; i < nsmps; i++, iter++) {
      if (iter == delay.end())
        iter = delay.begin();
      out[i] = *iter;
      *iter = in[i];
    }
    return OK;
  }
};

/** a-rate plugin opcode example: oscillator
    with 1 output and 3 inputs (k,k,i) \n
    aout oscillator kamp,kcps,ifn
 */
struct Oscillator : csnd::Plugin<1, 3> {
  csnd::Table table;
  double scl;
  double ndx;

  int init() {
    table.init(csound, inargs.data(2));
    scl = table.len() / csound->sr();
    ndx = 0;
    return OK;
  }

  int aperf() {
    MYFLT *out = outargs.data(0);
    MYFLT amp = inargs[0];
    MYFLT si = inargs[1] * scl;

    sa_offset(out);
    for (uint32_t i = offset; i < nsmps; i++) {
      out[i] = amp * table[(uint32_t)ndx];
      ndx += si;
      while (ndx < 0)
        ndx += table.len();
      while (ndx >= table.len())
        ndx -= table.len();
    }
    return OK;
  }
};

/** f-sig plugin opcode example: pv gain change
    with 1 output and 2 inputs (f,k) \n
    fsig pvg fsin, kgain
 */
struct PVGain : csnd::FPlugin<1, 2> {
  static constexpr char const *otypes = "f";
  static constexpr char const *itypes = "fk";

  int init() {
    if (inargs.fsig_data(0).isSliding())
      return csound->init_error("sliding not supported");

    if (inargs.fsig_data(0).fsig_format() != csnd::fsig_format::pvs &&
        inargs.fsig_data(0).fsig_format() != csnd::fsig_format::polar)
      return csound->init_error("fsig format not supported");

    csnd::Fsig &fout = outargs.fsig_data(0);
    fout.init(csound, inargs.fsig_data(0));
    framecount = 0;
    return OK;
  }

  int kperf() {
    csnd::pv_frame &fin = inargs.fsig_data(0);
    csnd::pv_frame &fout = outargs.fsig_data(0);

    if (framecount < fin.count()) {
      MYFLT g = inargs[1];
      std::transform(fin.begin(), fin.end(), fout.begin(),
                     [g](csnd::pv_bin f) { return f *= g; });
      framecount = fout.count(fin.count());
    }
    return OK;
  }
};

/** Thread to compute Gaussian distr.
 */
class MyThread : public csnd::Thread {
  MYFLT *res;
  std::atomic_bool on;
  std::normal_distribution<MYFLT> norm;
  std::mt19937 gen;

public:
  MyThread(csnd::Csound *csound, MYFLT mean, MYFLT std, MYFLT *r)
      : Thread(csound), res(r), on(true), norm(mean, std), gen(){};
  uintptr_t run() {
    while (on)
      *res = norm(gen);
    return 0;
  }
  void stop() { on = false; }
};

/**
   Asynchronous normal distribution generation
 */
struct AsyncGauss : csnd::Plugin<1, 2> {
  MyThread t;
  MYFLT res;

  int init() {
    csound->plugin_deinit(this);
    csnd::constr<MyThread>(&t, csound, inargs[0], inargs[1], &res);
    return OK;
  }

  int deinit() {
    t.stop();
    t.join();
    return OK;
  }

  int kperf() {
    outargs[0] = res;
    return OK;
  }
};

/** Library loading
 */
void csnd::on_load(Csound *csound) {
  csnd::plugin<Simplei>(csound, "simple", "i", "i", csnd::thread::i);
  csnd::plugin<Simplek>(csound, "simple", "k", "k", csnd::thread::k);
  csnd::plugin<Simplea>(csound, "simple", "a", "a", csnd::thread::a);
  csnd::plugin<SimpleArray>(csound, "simple", "k[]", "k[]", csnd::thread::ik);
  csnd::plugin<SimpleArrayA>(csound, "simple", "a[]", "a[]", csnd::thread::ia);
  csnd::plugin<Tprint>(csound, "tprint", "", "S", csnd::thread::i);
  csnd::plugin<DelayLine>(csound, "delayline", "a", "ai", csnd::thread::ia);
  csnd::plugin<Oscillator>(csound, "oscillator", "a", "kki", csnd::thread::ia);
  csnd::plugin<PVGain>(csound, "pvg", csnd::thread::ik);
  csnd::plugin<AsyncGauss>(csound, "gaussa", "k", "ii", csnd::thread::ik);
}
