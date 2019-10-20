<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  asig vco2 .01, 110	; sawtooth waveform at low volume
  kcut line 300, p3, 60	; Vary cutoff frequency
  kresonance = 3
  asig lowres asig, kcut, kresonance
  outs2 asig ; output stereo channel 2 only
endin
</CsInstruments>
<CsScore>

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
