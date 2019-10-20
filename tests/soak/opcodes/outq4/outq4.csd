<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

instr 1

asig vco2 .05, 30	; sawtooth waveform at low volume

  kcut line 100, p3, 30	; Vary cutoff frequency
  kresonance = 7
  asig lowres asig, kcut, kresonance
  outq4 asig	; output channel 4

endin
</CsInstruments>
<CsScore>

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
