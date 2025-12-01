/*
  daisyCsoundProcess.h:

  Copyright (C) 2025 Aman Jagawni

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1335, USA
*/


#ifndef DAISY_CSOUND_H
#define DAISY_CSOUND_H

#include <string>

using namespace std;

string csdText = R"csd(
      <CsoundSynthesizer>
      <CsOptions>
      </CsOptions>
      <CsInstruments>

      sr = 48000
      0dbfs = 1
      nchnls = 2



      instr 4 // echo effect
      aL, aR ins
      klpf = 6000
      kfb chnget "AnalogIn0"
      kdel chnget "AnalogIn6"
      kdel = kdel * 1000
      adel interp kdel
      kfb limit kfb, 0, 0.9
      afbL init 0
      afbR init 0
      adelayL vdelay (aL+afbL)*0.5, adel, 1000
      adelayR vdelay (aR+afbR)*0.5, adel * 1.1, 1000
      afbL = adelayL * kfb
      afbR = adelayR * kfb
      adelayL butterlp adelayL, klpf
      adelayR butterlp adelayR, klpf
      outs adelayL*0.7 + aL*0.3, adelayR*0.7 + aR*0.3
      endin
      schedule(4, 0, 60*60*24*7)



      </CsInstruments>
      <CsScore>
      </CsScore>
      </CsoundSynthesizer>
  )csd";


#endif