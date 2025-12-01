// Engine.cpp: Csound engine implementation
// Copyright (C) 2025 Victor Lazzarini.
//
// This file is part of Csound.
//
// The Csound Library is free software; you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// Csound is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Csound; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA

#include <string>
#include "iOSCsound.hpp"
#include "Engine.h"

static iOSCsound *csound = nullptr;
static float vol = 0.5f;
static float oct = 4.f;
static const char *code = R"(
0dbfs = 1
nchnls = 2
seed 0
ga1 init 0
ga2 init 0
instr 1
 vol:k = chnget:k("volume")
 fr:i = chnget:i("oct")*45;
 mod:a = oscili(linrand(5)*p5, p5*(1 + gauss(0.5)))
 sig:a = oscili(vol*expon(p4,p3,0.001),p5+mod*expon(1,p3,0.001))
 left:a, right:a pan2 sig, linrand:i(1)
 out(left, right)
 ga1 = left*0.25;
 ga2 = right*0.25;
 schedule(1,0.1 + linrand(0.1), 0.1+linrand(2), linrand(1), fr + gauss(fr))
endin
schedule(1,0,0.1,0.5,500)
instr 2
 left:a, right:a reverbsc2 ga1,ga2, 0.7, 5000
  out(left, right)
  ga1 = 0
  ga2 = 0
endin
schedule(2,0,-1)
        )";

void  set_volume(float val){
  vol = val;
  if(csound != nullptr)
    csound->SetControlChannel("volume", val);
}

void  set_octave(float val) {
  float octave = powf(2.f, val * 8.f);
  oct = octave;
  if(csound != nullptr)
    csound->SetControlChannel("oct", octave);

}

void start_csound() {
  if (csound == nullptr) {
    csound = new iOSCsound;
    csound->SetOption("-o dac -b 128");
    csound->CompileOrc(code);
    csound->SetControlChannel("volume", vol);
    csound->SetControlChannel("oct", oct);
    csound->Start();
  }
}

void stop_csound(){
  if(csound != nullptr) {
    csound->EventString("e 0 0", 1);
    delete csound;
    csound = nullptr;
  }
}
