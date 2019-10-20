<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  kenv    expon 0.01, p3, 1
  aout    poscil 0.8*kenv, 440, 1 ;multiply amplitude from 0 to 1 * 0.8
  outs 	aout, aout
endin

</CsInstruments>
<CsScore>

f 1 0 16384 10 1	; sine wave

i 1 0 2

e

</CsScore>
</CsoundSynthesizer>
