/*
  pvsops.c: pvs opcodes

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
      int n =  fin.len() - (int) inargs[1];
      float thrsh;
      std::transform(fin.begin(), fin.end(), amps.begin(),
                     [](csnd::pv_bin f){ return f.amp(); });
      std::nth_element(amps.begin(), amps.begin()+n, amps.end());
      thrsh = amps[n];
      std::transform(fin.begin(), fin.end(), fout.begin(),
                     [thrsh](csnd::pv_bin f){
                       return f.amp() >= thrsh ? f : csnd::pv_bin(); });
      framecount = fout.count(fin.count());
    }
    return OK;
  }
};

struct TVConv : csnd::Plugin<1,3> {
  static constexpr char const *otypes = "a";
  static constexpr char const *itypes = "aai";
  csnd::AuxMem<MYFLT> ir;
  csnd::AuxMem<MYFLT> in;
  csnd::AuxMem<MYFLT> out;
  csnd::AuxMem<MYFLT> saved;
  uint32_t n; 
  uint32_t ns;
  csnd::fftp fwd,inv;

  int init() {
    ns = inargs[2];
    fwd = csound->fft_setup(2*ns,FFT_FWD);
    inv = csound->fft_setup(2*ns,FFT_INV);
    ir.allocate(csound, 2*ns);
    in.allocate(csound, 2*ns);
    out.allocate(csound, 2*ns);
    saved.allocate(csound, ns);
    n = 0;
    return OK;
  }

  int aperf() {
    csnd::AudioSig insig(this, inargs(0));
    csnd::AudioSig irsig(this, inargs(1));
    csnd::AudioSig outsig(this, outargs(0));

    auto irp = irsig.begin();
    auto inp = insig.begin();

    for(auto &s: outsig) {
      ir[n] = *irp++;
      in[n] = *inp++;
      s  = out[n] + saved[n];
      saved[n] = out[n + ns];
       if(++n == ns) {
	 std::complex<MYFLT> *ins,*irs;
	 std::complex<MYFLT> *ous =
	  reinterpret_cast<std::complex<MYFLT>*>(out.data());
	 std::fill(in.begin()+ns,in.end(),0.); 
	 std::fill(ir.begin()+ns,ir.end(),0.);
	 // FFT
         irs = csound->rfft(fwd, ir.data());
	 ins = csound->rfft(fwd, in.data());
         // Mult
	 for(uint32_t i=1; i < ns; i++)
	   ous[i] =  ins[i] * irs[i];    
	 ous[0].real(irs[0].real()*ins[0].real());	 
	 ous[0].imag(irs[0].imag()*ins[0].imag());
	 // IFFT
	 csound->rfft(inv, out.data());
         n = 0;
       }	 
    }
    return OK; 
  }
};



#include <modload.h>
void csnd::on_load(Csound *csound) {
  csnd::plugin<PVTrace>(csound, "pvstrace", csnd::thread::ik);
  csnd::plugin<TVConv>(csound, "tvconv", csnd::thread::ia);
}
