<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  asig vco2 .01, 110	; sawtooth waveform at low volume
  kcut line 60, p3, 300	; Vary cutoff frequency
  kresonance = 3
  asig lowres asig, kcut, kresonance
  outs1 asig		; output stereo channel 1 only
endin
</CsInstruments>
<CsScore>

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
