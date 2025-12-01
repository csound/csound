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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
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
      ksmps = 160
      nchnls = 2
      garev init 0
      prealloc 1,8
      initc7 1,73,0.01 
 
      instr 1
       iatt midic7 73,0.01,1
       kcf chnget "pot1"
       kres chnget "pot2"
       iwave = chnget("toggle1")*10
       kdet = chnget("encoder")
       kcps cpsmidib 2
       kvib midic7 1,0,kcps
       if kvib > 0 then
        kcps += oscil:k(kvib*0.05,7)
       endif
       iamp ampmidi 0.05
       kenv madsr iatt, .1, .6, 0.1
       a1 vco2 iamp, kcps*(1-kdet), iwave, 0.5, rnd(1)
       a2 vco2 iamp, kcps*(1+kdet), iwave, 0.5, rnd(1)
       asig vclpf (a1+a2), kcps + kenv*port(kcf*13000, 0.01), kres
       asig linenr asig,iatt,0.1,0.01
         garev += asig
      endin

      
      instr 100
       al, ar init 0
       ktrig,ktrig1,ktrig2 init 1,1,1
       krev = chnget:k("etoggle")
       kchr = chnget:k("toggle2")
       
      if kchr > 0 then 
        ad delayr 0.05
        as1 oscili 0.0011, 1.03
	as2 oscili 0.0017, 0.97
        al deltapi as1+0.011
	ar deltapi as2+0.017
	   delayw garev
	al += garev
        ar += garev
       else 
        al = garev
        ar = garev
       endif

       if krev > 0 then
        arl, arr reverbsc2 al, ar, 0.7, 5000, sr, 0
	arl *= 0.5
	arr *= 0.5
	al += arl
	ar += arr
       endif

       if kchr > 0 && kchr > 0 then
          scoreline "i101 0 0 7", ktrig
          ktrig = 0;
          ktrig1 = 1;
          ktrig2  = 1
       elseif krev > 0 then
          scoreline "i101 0 0 7", ktrig1
          ktrig = 1;
          ktrig1 = 0;
          ktrig2 = 1;
       elseif kchr > 0 then
          scoreline "i101 0 0 6", ktrig1
          ktrig = 1;
          ktrig1 = 0;
          ktrig2 = 1;
       else
         scoreline "i101 0 0 8", ktrig2
         ktrig = 1
         ktrig1 = 1
         ktrig2 = 0
       endif

       out al, ar
       garev = 0
      endin
      schedule(100,0,-1)

      instr 101
        maxalloc 1,p4,2
      endin
      </CsInstruments>
      <CsScore>
      </CsScore>
      </CsoundSynthesizer>
   )csd";
#endif
