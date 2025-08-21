/*
  daisy_pod_midi.h

  Copyright (C) 2025 

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

std::string csd_text = R"csd(
      <CsoundSynthesizer>
      <CsOptions>
      </CsOptions>
      <CsInstruments>

      sr = 48000
      0dbfs = 1
      ksmps = 128
      nchnls = 2
 
      instr 1
      kcf chnget "pot1"
      kres chnget "pot2"
      iwave = chnget("toggle1")*10
      kdet = chnget("encoder")
      kcps cpsmidib 2
      iamp ampmidi 0dbfs*0.1
      aenv madsr .05, .1, .6, .2
      a1 vco2 iamp, kcps*(1-kdet), iwave
      a2 vco2 iamp, kcps*(1+kdet), iwave
      afilt vclpf (a1+a2)*aenv, port(kcps + exp(kcf*9.5), 0.01), kres
      outs afilt, afilt
      endin

      </CsInstruments>
      <CsScore>
      </CsScore>
      </CsoundSynthesizer>
   )csd";
#endif
