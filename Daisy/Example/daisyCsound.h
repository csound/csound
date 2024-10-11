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
      outs afilt*aenv*kamp, afilt*aenv*kamp
      endin


      </CsInstruments>
      <CsScore>
      </CsScore>
      </CsoundSynthesizer>
   )csd";
#endif