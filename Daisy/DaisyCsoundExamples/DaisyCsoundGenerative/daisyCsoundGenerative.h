/*
  daisyCsoundGenerative.h

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

      
      gkseq[] fillarray 71, 67, 79, 74, 71, 74, 79, 76, 83, 76

      seed 0
      instr 1 // metronome
      gktempo = 600
      gkclick metro gktempo/60
      endin



      instr 2 // counter
      isubdiv = 4
      // counter going up till isubdiv 
      RESET:
      gkcount init 0
      if (gkclick == 1) then
      gkcount += 1
      endif
      if (gkcount == isubdiv*50) then
      reinit RESET
      endif
      rireturn 
      endin




      instr 10
      krandcps rspline 0.1, 4, 0.1, 0.5
      gkrandtrig metro krandcps 
      kbubdiv trandom gkrandtrig, 1, 6
      if (gkcount % int(kbubdiv) == 0) then
      kbubtrig = 1
      else kbubtrig = 0
      endif
      schedkwhen kbubtrig, 0, 1, 3, 0, (60/gktempo)*kbubdiv*1
      endin 



      instr 3 ; marimba
      kindex chnget "AnalogIn0"
      kdelay chnget "AnalogIn6"
      index random 0, 10
      index = int(index)
      knote = gkseq[index]
      iamp = (0.9 + rnd(0.1))
      idur = p3
      knote = cpsmidinn(knote-24)
      aindx expseg 30, idur/2, 0.00001
      aenv expsegr 0.001, 0.01, 0.1, idur, 0.01, idur/4, 0.001
      amod1 oscili (aindx*kindex)*(knote*4), knote*3
      acar1 oscili 1, knote*2+amod1
      al = (acar1)*aenv*iamp
      ar = (acar1)*aenv*iamp
      outs al, ar    
      chnmix (al+ar)*(0.9+rnd(0.1))*kdelay, "delayL"
      chnmix (al+ar)*(0.9+rnd(0.1))*kdelay, "delayR"
      endin


      instr 4 // delay
      adelL chnget "delayL"
      adelR chnget "delayR"
      khpf init 200
      klpf init 7500
      kfb init 0.9
      afb init 0
      adelL butterhp adelL, khpf
      adelR butterhp adelR, khpf
      adelayModL rspline 0.99, 1.01, 0.1, 9
      adelayModR rspline 0.99, 1.01, 0.1, 9
      adelayl vdelay adelL+afb, 280*adelayModL, 1000
      adelayr vdelay adelR+afb, 310*adelayModR, 1000
      afb = ((adelayl + adelayr)*0.5) * kfb
      adelayl butterlp adelayl, klpf
      adelayr butterlp adelayr, klpf
      outs adelayl*0.5, adelayr*0.5
      chnclear "delayL"
      chnclear "delayR"
      endin


      schedule(1, 0, 60*60*24*7)
      schedule(2, 0, 60*60*24*7)
      schedule(10, 0, 60*60*24*7)
      schedule(4, 0, 60*60*24*7)



      </CsInstruments>
      <CsScore>
      </CsScore>
            </CsoundSynthesizer>
      )csd";


#endif
