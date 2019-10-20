<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

  iroot = 440		; root note is A above middle-C (440 Hz)
  koct  lfo 5, 1, 5	; generate sawtooth, go from 5 octaves higher to root
  koc = int(koct)		; produce only whole numbers
  kfactor = octave(koc)	; for octave
  knew = iroot * kfactor
  asig pluck 1, knew, 1000, 0, 1
  asig dcblock asig	;remove DC
  outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 5
e

</CsScore>
</CsoundSynthesizer>
