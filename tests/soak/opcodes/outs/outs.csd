<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

  asig vco2 .01, 110	; sawtooth waveform at low volume
  ;filter a channel
  kcut1 line 60, p3, 300	; Vary cutoff frequency
  kresonance1 = 3
  asig1 lowres asig, kcut1, kresonance1
  ;filter the other channel
  kcut2 line 300, p3, 60	; Vary cutoff frequency
  kresonance2 = 3
  asig2 lowres asig, kcut2, kresonance2
  outs asig1, asig2	; output both channels 1 & 2
endin
</CsInstruments>
<CsScore>

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
