
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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/
#include <algorithm>
#include <plugin.h>

struct PVTrace : csnd::FPlugin<1, 2> {
  csnd::AuxMem<float> amps;
  static constexpr char const *otypes = "f";
  static constexpr char const *itypes = "fk";

  int32_t init() {
    if (inargs.fsig_data(0).isSliding())
      return csound->init_error("sliding not supported");
    if (inargs.fsig_data(0).fsig_format() != csnd::fsig_format::pvs &&
        inargs.fsig_data(0).fsig_format() != csnd::fsig_format::polar)
      return csound->init_error("fsig format not supported");

    amps.allocate(csound, inargs.fsig_data(0).nbins());
    csnd::Fsig &fout = outargs.fsig_data(0);
    fout.init(csound, inargs.fsig_data(0));
    framecount = 0;
    return OK;
  }

  int32_t kperf() {
    csnd::pv_frame &fin = inargs.fsig_data(0);
    csnd::pv_frame &fout = outargs.fsig_data(0);
    if (framecount < fin.count()) {
      int32_t n = fin.len() - (int) (inargs[1] >= 1 ? inargs[1] : 1.);
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

struct binamp {
  int32_t bin;
  float amp;
};

struct PVTrace2 : csnd::FPlugin<2, 5> {
  csnd::AuxMem<float> amps;
  csnd::AuxMem<binamp> binlist;
  static constexpr char const *otypes = "fk[]";
  static constexpr char const *itypes = "fkooo";

  int32_t init() {
    csnd::Vector<MYFLT> &bins = outargs.vector_data<MYFLT>(1);
    if (inargs.fsig_data(0).isSliding())
      return csound->init_error("sliding not supported");

    if (inargs.fsig_data(0).fsig_format() != csnd::fsig_format::pvs &&
        inargs.fsig_data(0).fsig_format() != csnd::fsig_format::polar)
      return csound->init_error("fsig format not supported");

    amps.allocate(csound, inargs.fsig_data(0).nbins());
    binlist.allocate(csound, inargs.fsig_data(0).nbins());
    csnd::Fsig &fout = outargs.fsig_data(0);
    fout.init(csound, inargs.fsig_data(0));

    bins.init(csound, inargs.fsig_data(0).nbins(), this->insdshead);

    framecount = 0;
    return OK;
  }

  int32_t kperf() {
    csnd::pv_frame &fin = inargs.fsig_data(0);
    csnd::pv_frame &fout = outargs.fsig_data(0);
    csnd::Vector<MYFLT> &bins = outargs.vector_data<MYFLT>(1);
    csnd::AuxMem<binamp> &mbins = binlist;

    if (framecount < fin.count()) {
      int32_t n = fin.len() - (int) (inargs[1] >= 1 ? inargs[1] : 1.);
      float thrsh;
      int32_t cnt = 0;
      int32_t bin = 0;
      int32_t start = (int) inargs[3];
      int32_t end = (int) inargs[4];
      std::transform(fin.begin() + start,
                     end ? fin.begin() +
                     ((unsigned int)end <= fin.len() ? end : fin.len()) :
                     fin.end(), amps.begin(),
                     [](csnd::pv_bin f) { return f.amp(); });
      std::nth_element(amps.begin(), amps.begin() + n, amps.end());
      thrsh = amps[n];
      std::transform(fin.begin(), fin.end(), fout.begin(),
                     [thrsh, &mbins, &cnt, &bin](csnd::pv_bin f) {
                       if(f.amp() >= thrsh) {
                       mbins[cnt].bin = bin++;
                       mbins[cnt++].amp = f.amp();
                       return f;
                       }
                       else {
                        bin++;
                        return csnd::pv_bin();
                       }
                     });

      if(inargs[2] > 0)
      std::sort(binlist.begin(), binlist.begin()+cnt, [](binamp a, binamp b){
          return (a.amp > b.amp);});

      std::transform(binlist.begin(), binlist.begin()+cnt, bins.begin(),
                     [](binamp a) { return (MYFLT) a.bin;});
      std::fill(bins.begin()+cnt, bins.end(), FL(0.0));

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
  csnd::AuxMem<MYFLT>::iterator itn;
  csnd::AuxMem<MYFLT>::iterator itr;
  csnd::AuxMem<MYFLT>::iterator itnsp;
  csnd::AuxMem<MYFLT>::iterator itrsp;
  uint32_t n;
  uint32_t fils;
  uint32_t pars;
  uint32_t ffts;
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

  int32_t init() {
    pars = inargs[4];
    fils = inargs[5];
    if (pars > fils)
      std::swap(pars, fils);
    if (pars > 1) {
      pars = rpow2(pars);
      fils = rpow2(fils) * 2;
      ffts = pars * 2;
      fwd = csound->fft_setup(ffts, FFT_FWD);
      inv = csound->fft_setup(ffts, FFT_INV);
      out.allocate(csound, ffts);
      insp.allocate(csound, fils);
      irsp.allocate(csound, fils);
      saved.allocate(csound, pars);
      ir.allocate(csound, fils);
      in.allocate(csound, fils);
      itnsp = insp.begin();
      itrsp = irsp.begin();
      n = 0;
    } else {
      ir.allocate(csound, fils);
      in.allocate(csound, fils);
    }
    itn = in.begin();
    itr = ir.begin();
    return OK;
  }

  int32_t pconv() {
    csnd::AudioSig insig(this, inargs(0));
    csnd::AudioSig irsig(this, inargs(1));
    csnd::AudioSig outsig(this, outargs(0));
    auto irp = irsig.begin();
    auto inp = insig.begin();
    auto *frz1 = inargs(2);
    auto *frz2 = inargs(3);
    auto inc1 = csound->is_asig(frz1);
    auto inc2 = csound->is_asig(frz2);
    MYFLT _0dbfs = csound->_0dbfs();

    for (auto &s : outsig) {
      if (*frz1 > 0)
        itn[n] = *inp/_0dbfs;
      if (*frz2 > 0)
        itr[n] = *irp/_0dbfs;

      s = (out[n] + saved[n])*_0dbfs;
      saved[n] = out[n + pars];
      if (++n == pars) {
        cmplx *ins, *irs, *ous = to_cmplx(out.data());
        std::copy(itn, itn + ffts, itnsp);
        std::copy(itr, itr + ffts, itrsp);
        std::fill(out.begin(), out.end(), 0.);
        // FFT
        csound->rfft(fwd, itnsp);
        csound->rfft(fwd, itrsp);
        // increment iterators
        itnsp += ffts, itrsp += ffts;
        itn += ffts, itr += ffts;
        if (itnsp == insp.end()) {
          itnsp = insp.begin();
          itrsp = irsp.begin();
          itn = in.begin();
          itr = ir.begin();
        }
        // spectral delay line
        for (csnd::AuxMem<MYFLT>::iterator it1 = itnsp, it2 = irsp.end() - ffts;
             it2 >= irsp.begin(); it1 += ffts, it2 -= ffts) {
          if (it1 == insp.end())
            it1 = insp.begin();
          ins = to_cmplx(it1);
          irs = to_cmplx(it2);
          // spectral product
          for (uint32_t i = 1; i < pars; i++)
            ous[i] += ins[i] * irs[i];
          ous[0] += real_prod(ins[0], irs[0]);
        }
        // IFFT
        csound->rfft(inv, out.data());
        n = 0;
      }
      frz1 += inc1, frz2 += inc2;
      irp++, inp++;
    }
    return OK;
  }

  int32_t dconv() {
    csnd::AudioSig insig(this, inargs(0));
    csnd::AudioSig irsig(this, inargs(1));
    csnd::AudioSig outsig(this, outargs(0));
    auto irp = irsig.begin();
    auto inp = insig.begin();
    auto frz1 = inargs(2);
    auto frz2 = inargs(3);
    auto inc1 = csound->is_asig(frz1);
    auto inc2 = csound->is_asig(frz2);

    for (auto &s : outsig) {
      if (*frz1 > 0)
        *itn = *inp;
      if (*frz2 > 0)
        *itr = *irp;
      itn++, itr++;
      if (itn == in.end()) {
        itn = in.begin();
        itr = ir.begin();
      }
      s = 0.;
      for (csnd::AuxMem<MYFLT>::iterator it1 = itn, it2 = ir.end() - 1;
           it2 >= ir.begin(); it1++, it2--) {
        if (it1 == in.end())
          it1 = in.begin();
        s += *it1 * *it2;
      }
      frz1 += inc1, frz2 += inc2;
      inp++, irp++;
    }
    return OK;
  }

  int32_t aperf() {
    if (pars > 1)
      return pconv();
    else
      return dconv();
  }
};


struct Gtadsr : public csnd::Plugin<1,6> {
  uint64_t a,d;
  MYFLT e,ainc,dfac;
  uint64_t t;

  int32_t init() {
    t = 0;
    e = MYFLT(0);
    return OK;
  }

  int32_t kperf() {
    MYFLT gate = inargs[5];
    MYFLT s = inargs[3];
    s = s  > 0  ? (s < 1 ? s : 1.) : 0.;
    if(gate > 0) {
      if(t == 0) {
        a = inargs[1]*this->kr();
	d = inargs[2]*this->kr();
	if(a < 1) a = 1;
	if(d < 1) d = 1;
	ainc = 1./a;
	dfac = 1./d;
      }
      if (t < a && e < (1 - ainc))
       e +=  ainc;
     else if (t < a + d && e > s)
       e += (s - 1) * dfac;
     else
       e = s;
      t += 1;
    } else {
      if (e < 0.00001)
        e = 0;
      else
        e *= pow(0.001, 1. / (inargs[4]*this->kr()));
      t = 0;   
    }
    outargs[0] = e*inargs[0];
    return OK;
  }

  int32_t aperf() {
    MYFLT gate = inargs[5];
    MYFLT s = inargs[3];
    s = s > 0 ? (s < 1 ? s : 1.) : 0.;
    MYFLT *sig  = NULL, amp = MYFLT(0);
    if(csound->is_asig(inargs(0)))
       sig = inargs(0);
    else
      amp = inargs[0];
    MYFLT *out = outargs(0);

    for(auto n = offset; n < nsmps; n++) {
       if(gate > 0) {
      if(t == 0) {
        a = inargs[1]*this->sr();
	d = inargs[2]*this->sr();
	if(a < 1) a = 1;
	if(d < 1) d = 1;
	ainc = 1./a;
	dfac = 1./d;
      }
      if (t < a && e < (1 - ainc))
       e +=  ainc;
     else if (t < a + d && e > s)
       e += (s - 1) * dfac;
     else
       e = s;
      t += 1;
    } else {
      if (e < 0.00001)
        e = 0;
      else
        e *= pow(0.001, 1. / (inargs[4]*this->sr()));
      t = 0;   
    }
       out[n] = sig ? sig[n]*e : amp*e;
 
    }
    return OK;
  }
  
};




void onload(csnd::Csound *csound) {
  csnd::plugin<PVTrace>(csound, "pvstrace",  csnd::thread::ik);
  csnd::plugin<PVTrace2>(csound, "pvstrace", csnd::thread::ik);
  csnd::plugin<TVConv>(csound, "tvconv", "a", "aaxxii", csnd::thread::ia);
  csnd::plugin<Gtadsr>(csound, "gtadsr", "k", "kkkkkk", csnd::thread::ik);
  csnd::plugin<Gtadsr>(csound, "gtadsr", "a", "akkkkk", csnd::thread::ia);
  csnd::plugin<Gtadsr>(csound, "gtadsr", "a", "kkkkkk", csnd::thread::ia);
}

#ifdef BUILD_PLUGINS
#include <modload.h>
void csnd::on_load(csnd::Csound *csound) {
    onload(csound);
}
#else
extern "C" int32_t pvsops_init_modules(CSOUND *csound) {
    onload((csnd::Csound *)csound);
    return OK;
  }
#endif
