<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
  klambda	gaussi	100, 1, 3
  aout	oscili	0.8, 440+klambda, 1	; & listen
  outs	aout, aout
endin

</CsInstruments>
<CsScore>
; sine wave
f 1 0 16384 10 1

i 1 0 4
e

</CsScore>
</CsoundSynthesizer>
