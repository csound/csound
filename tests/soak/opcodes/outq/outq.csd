<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

instr 1

asig vco2 .01, 110	; sawtooth waveform at low volume

  ;filter the first channel
  kcut1 line 60, p3, 300	; Vary cutoff frequency
  kresonance1 = 3
  asig1 lowres asig, kcut1, kresonance1

  ;filter the second channel
  kcut2 line 300, p3, 60	; Vary cutoff frequency
  kresonance2 = 3
  asig2 lowres asig, kcut2, kresonance2

  ;filter the third channel
  kcut3 line 30, p3, 100; Vary cutoff frequency
  kresonance3 = 6
  asig3 lowres asig, kcut3, kresonance3
  asig3 = asig3*.1	; lower volume
  ;filter the fourth channel
  kcut4 line 100, p3, 30; Vary cutoff frequency
  kresonance4 = 6
  asig4 lowres asig, kcut4, kresonance4
  asig4 = asig4*.1	; lower volume
  outq asig1, asig2, asig3, asig4; output channels 1, 2, 3 & 4

endin
</CsInstruments>
<CsScore>

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>
