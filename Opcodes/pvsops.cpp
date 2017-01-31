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
      return init_error("sliding not supported");
    
    if (inargs.fsig_data(0).fsig_format() != csnd::fsig_format::pvs &&
        inargs.fsig_data(0).fsig_format() != csnd::fsig_format::polar)
      return init_error("fsig format not supported");

    amps.allocate(csound, inargs.fsig_data(0).len());
    csnd::Fsig &fout = outargs.fsig_data(0);
    fout.init(csound, inargs.fsig_data(0));
    framecount = 0;
    return OK;
  }

  int kperf() {
    csnd::Fsig &fin = inargs.fsig_data(0);
    csnd::Fsig &fout = outargs.fsig_data(0);

    if (framecount < fin.count()) {
      int n =  fin.len() - (int) inargs[1];
      float thrsh;
      std::transform(fin.begin(), fin.end(), amps.begin(),
		     [](csnd::pvsbin f){ return f.amp(); });
      std::nth_element(amps.begin(), amps.begin()+n, amps.end());
      thrsh = amps[n];
      std::transform(fin.begin(), fin.end(), fout.begin(),
		     [thrsh](csnd::pvsbin f){ 
		       return f.amp() >= thrsh ? f : csnd::pvsbin(); });
      framecount = fout.count(fin.count());
    }
    return OK;
  }
};

extern "C" {
PUBLIC int csoundModuleInit(CSOUND *csound) {
  csnd::plugin<PVTrace>(csound, "pvstrace", csnd::thread::ik);
  return 0;
}
PUBLIC int csoundModuleCreate(CSOUND *csound) { return 0; }
PUBLIC int csoundModuleDestroy(CSOUND *csound) { return 0; }
}
