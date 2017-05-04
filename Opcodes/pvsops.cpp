/*
  pvsops.c: pvs and other spectral-based opcodes

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
#include <algorithm>
#include <plugin.h>

struct PVTrace : csnd::FPlugin<1, 2> {
  csnd::AuxMem<float> amps;
  static constexpr char const *otypes = "f";
  static constexpr char const *itypes = "fk";

  int init() {
    if (inargs.fsig_data(0).isSliding())
      return csound->init_error(Str("sliding not supported"));

    if (inargs.fsig_data(0).fsig_format() != csnd::fsig_format::pvs &&
        inargs.fsig_data(0).fsig_format() != csnd::fsig_format::polar)
      return csound->init_error(Str("fsig format not supported"));

    amps.allocate(csound, inargs.fsig_data(0).nbins());
    csnd::Fsig &fout = outargs.fsig_data(0);
    fout.init(csound, inargs.fsig_data(0));
    framecount = 0;
    return OK;
  }

  int kperf() {
    csnd::pv_frame &fin = inargs.fsig_data(0);
    csnd::pv_frame &fout = outargs.fsig_data(0);

    if (framecount < fin.count()) {
      int n = fin.len() - (int)inargs[1];
      float thrsh;
      std::transform(fin.begin(), fin.end(), amps.begin(),
                     [](csnd::pv_bin f) { return f.amp(); });
      std::nth_element(amps.begin(), amps.begin() + n, amps.end());
      thrsh = amps[n];
      std::transform(fin.begin(), fin.end(), fout.begin(),
                     [thrsh](csnd::pv_bin f) {
                       return f.amp() >= thrsh ? f : csnd::pv_bin();
                     });
      framecount = fout.count(fin.count());
    }
    return OK;
  }
};

struct TVConv : csnd::Plugin<1, 6> {
  csnd::AuxMem<MYFLT> ir;
  csnd::AuxMem<MYFLT> in;
  csnd::AuxMem<MYFLT> insp;
  csnd::AuxMem<MYFLT> irsp;
  csnd::AuxMem<MYFLT> out;
  csnd::AuxMem<MYFLT> saved;
  uint32_t n;
  uint32_t fils;
  uint32_t pars;
  uint32_t ffts;
  uint32_t nparts;
  uint32_t pp;
  csnd::fftp fwd, inv;
  typedef std::complex<MYFLT> cmplx;

  uint32_t rpow2(uint32_t n) {
    uint32_t v = 2;
    while (v <= n)
      v <<= 1;
    if ((n - (v >> 1)) < (v - n))
      return v >> 1;
    else
      return v;
  }

  cmplx *to_cmplx(MYFLT *f) { return reinterpret_cast<cmplx *>(f); }

  cmplx real_prod(cmplx &a, cmplx &b) {
    return cmplx(a.real() * b.real(), a.imag() * b.imag());
  }

  int init() {
    pars = inargs[4];
    fils = inargs[5];
    if (pars > fils)
      std::swap(pars, fils);
    if (pars > 1) {
      pars = rpow2(pars);
      fils = rpow2(fils);
      ffts = pars * 2;
      fwd = csound->fft_setup(ffts, FFT_FWD);
      inv = csound->fft_setup(ffts, FFT_INV);
      out.allocate(csound, 2 * pars);
      insp.allocate(csound, 2 * fils);
      irsp.allocate(csound, 2 * fils);
      saved.allocate(csound, pars);
      ir.allocate(csound, 2 * fils);
      in.allocate(csound, 2 * fils);
      nparts = fils / pars;
      fils *= 2;
    } else {
      ir.allocate(csound, fils);
      in.allocate(csound, fils);
    }
    n = 0;
    pp = 0;
    return OK;
  }

  int pconv() {
    csnd::AudioSig insig(this, inargs(0));
    csnd::AudioSig irsig(this, inargs(1));
    csnd::AudioSig outsig(this, outargs(0));
    auto irp = irsig.begin();
    auto inp = insig.begin();
    MYFLT *frz1 = inargs(2);
    MYFLT *frz2 = inargs(3);
    bool inc1 = csound->is_asig(frz1);
    bool inc2 = csound->is_asig(frz2);

    for (auto &s : outsig) {
      if(*frz1 > 0) in[pp] = *inp;
      if(*frz2 > 0) ir[pp] = *irp;

      s = out[n] + saved[n];
      saved[n] = out[n + pars];
      if (++n == pars) {
        cmplx *ins, *irs, *ous = to_cmplx(out.data());
        std::copy(in.begin() + pp, in.begin() + pp + ffts, insp.begin() + pp);
        std::copy(ir.begin() + pp, ir.begin() + pp + ffts, irsp.begin() + pp);
        std::fill(out.begin(), out.end(), 0.);
        // FFT
        csound->rfft(fwd, insp.data() + pp);
        csound->rfft(fwd, irsp.data() + pp);
        pp += ffts;
        if (pp == fils)
          pp = 0;
        // spectral delay line
        for (uint32_t k = 0, kp = pp; k < nparts; k++, kp += ffts) {
          if (kp == fils)
            kp = 0;
          ins = to_cmplx(insp.data() + kp);
          irs = to_cmplx(irsp.data() + (nparts - k - 1) * ffts);
          // spectral product
          for (uint32_t i = 1; i < pars; i++)
            ous[i] += ins[i] * irs[i];
          ous[0] += real_prod(ins[0], irs[0]);
        }
        // IFFT
        csound->rfft(inv, out.data());
        n = 0;
      }
      frz1 += inc1;
      frz2 += inc2;
      irp++;
      inp++;
    }
    return OK;
  }

  int dconv() {
    csnd::AudioSig insig(this, inargs(0));
    csnd::AudioSig irsig(this, inargs(1));
    csnd::AudioSig outsig(this, outargs(0));
    auto irp = irsig.begin();
    auto inp = insig.begin();
    MYFLT *frz1 = inargs(2);
    MYFLT *frz2 = inargs(3);
    bool inc1 = csound->is_asig(frz1);
    bool inc2 = csound->is_asig(frz2);
    for (auto &s : outsig) {
      if(*frz1 > 0) in[pp] = *inp;
      if(*frz2 > 0) ir[pp] = *irp;
      pp = pp != fils - 1 ? pp + 1 : 0;
      s = 0.;
      for (uint32_t k = 0, kp = pp; k < fils; k++, kp++) {
        if (kp == fils)
          kp = 0;
        s += in[kp] * ir[fils - k - 1];
      }
      frz1 += inc1;
      frz2 += inc2;
      inp++;
      irp++;
    }
    return OK;
  }

  int aperf() {
    if (pars > 1)
      return pconv();
    else
      return dconv();
  }
};

#include <modload.h>
void csnd::on_load(Csound *csound) {
  csnd::plugin<PVTrace>(csound, "pvstrace", csnd::thread::ik);
  csnd::plugin<TVConv>(csound, "tvconv", "a", "aakkii", csnd::thread::ia);
  csnd::plugin<TVConv>(csound, "tvconv", "a", "aaakii", csnd::thread::ia);
  csnd::plugin<TVConv>(csound, "tvconv", "a", "aakaii", csnd::thread::ia);
  csnd::plugin<TVConv>(csound, "tvconv", "a", "aaaaii", csnd::thread::ia);
}
