<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 5
0dbfs  = 1

instr 1

  asig vco2 .05, 100	; sawtooth waveform at low volume
  kcut line 100, p3, 30	; Vary cutoff frequency
  kresonance = .7
  asig lowres asig, kcut, kresonance
  klfo lfo 4, .5, 4
  klfo = klfo+1		; offset of 1
  outch klfo,asig
endin
</CsInstruments>
<CsScore>

i 1 0 30
e
</CsScore>
</CsoundSynthesizer>
