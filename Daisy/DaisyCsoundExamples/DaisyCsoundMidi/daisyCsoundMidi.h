/*
  daisyCsoundMidi.h

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
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

      instr 1
      kdigi digiInDaisy 0, 2
      kcf chnget "AnalogIn0"
      kamp chnget "AnalogIn6"
      icps cpsmidi
      aenv madsr .05, .1, .6, .01
      a1 vco2 0.2, icps*0.995, i(kdigi)*10
      a2 vco2 0.2, icps*1.005, i(kdigi)*10
      afilt butterlp (a1+a2), 70 + (12000*kcf)
      outs afilt*aenv, afilt*aenv
      endin


      </CsInstruments>
      <CsScore>
      </CsScore>
      </CsoundSynthesizer>
   )csd";
#endif
