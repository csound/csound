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
      garev init 0
 
      instr 1
      kcf chnget "pot1"
      kres chnget "pot2"
      iwave = chnget("toggle1")*10
      kdet = chnget("encoder")
      kcps cpsmidib 2
      iamp ampmidi 0.05
      aenv mxadsr .02, .1, .8, .2
      kenv madsr .05, .1, .6, .2
      a1 vco2 iamp, kcps*(1-kdet), iwave
      a2 vco2 iamp, kcps*(1+kdet), iwave
      afilt vclpf (a1+a2)*aenv, kcps + port(kenv*kcf*13000, 0.01), kres
      outs afilt, afilt
         garev += afilt
      endin

      instr 100
       krev = chnget:k("etoggle")*0.5
       kchr = chnget:k("toggle2")*0.5
       
      if kchr > 0 then 
        ad delayr 0.05
        as oscili 1, 0.93
        a1 deltapi 0.011+as*0.0011
	a2 deltapi 0.017-as*0.0017
	   delayw garev*port(kchr,0.1)
	 out a1*0.2+a2*0.8, a1*0.8+a2*0.2
       endif

       if krev > 0 then
        garev *= port(krev,0.1)
        asigl, asigr reverbsc2 garev, garev, 0.7, 5000
          out asigl, asigr
       endif

       garev = 0
      endin
      schedule(100,0,-1)

      </CsInstruments>
      <CsScore>
      </CsScore>
      </CsoundSynthesizer>
   )csd";
#endif
