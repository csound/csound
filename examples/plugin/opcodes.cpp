/**
  optest.cpp
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
#include <plugin.h>

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

/** i-time string plugin opcode example
    with only 1 input \n
    tprint Sin
 */
struct Tprint : csnd::Plugin<0, 1> {
  int init() {
    csound->Message(csound, "%s", inargs.str_data(0).data);
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
    delay.allocate(csound, csound->GetSr(csound) * inargs[1]);
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
  MYFLT ndx;

  int init() {
    table.init(csound, inargs.data(2));
    ndx = 0;
    return OK;
  }

  int aperf() {
    MYFLT *out = outargs.data(0);
    MYFLT amp = inargs[0];
    MYFLT freq = inargs[1];
    MYFLT incr = freq * table.len() / csound->GetSr(csound);

    sa_offset(out);
    for (uint32_t i = offset; i < nsmps; i++) {
      out[i] = amp * table[(uint32_t)ndx];
      ndx += incr;
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
    fsig simple fsin, kgain
 */
struct PVGain : csnd::Plugin<1, 2> {
  uint32_t framecount;

  int init() {
    if (inargs.fsig_data(0).isSliding())
      return csound->InitError(csound, "sliding mode not supported\n");
    else if (inargs.fsig_data(0).fsig_format() != csnd::fsig_format::pvs ||
             inargs.fsig_data(0).fsig_format() != csnd::fsig_format::polar) {
      csnd::Fsig &fout = outargs.fsig_data(0);
      fout.init(csound, inargs.fsig_data(0));
      framecount = 0;
      return OK;
    } else
      return csound->InitError(csound, "wrong fsig format\n");
  }

  int kperf() {
    csnd::Fsig &fin = inargs.fsig_data(0);
    csnd::Fsig &fout = outargs.fsig_data(0);
    csnd::Fsig::iterator iteri, itero;
    if (framecount < fin.count()) {
      for (iteri = fin.begin(), itero = fout.begin(); iteri < fin.end();
           iteri += 2, itero += 2) {
        *itero = *iteri * inargs[1];
        *(itero + 1) = *(itero + 1);
      }
      framecount = fout.count(fin.count());
    }
    return OK;
  }
};

/** Module creation, initalisation and destruction
 */
extern "C" {
PUBLIC int csoundModuleInit(CSOUND *csound) {
  csnd::plugin<Simplei>(csound, "simple", "i", "i", csnd::thread::i);
  csnd::plugin<Simplek>(csound, "simple", "k", "k", csnd::thread::k);
  csnd::plugin<Tprint>(csound, "tprint", "", "S", csnd::thread::i);
  csnd::plugin<DelayLine>(csound, "delayline", "a", "ai", csnd::thread::ia);
  csnd::plugin<Oscillator>(csound, "oscillator", "a", "kki", csnd::thread::ia);
  csnd::plugin<PVGain>(csound, "pvg", "f", "fk", csnd::thread::ik);
  return 0;
}
PUBLIC int csoundModuleCreate(CSOUND *csound) { return 0; }
PUBLIC int csoundModuleDestroy(CSOUND *csound) { return 0; }
}
