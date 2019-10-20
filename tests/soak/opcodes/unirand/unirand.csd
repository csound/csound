<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1  	; every run time same values
  ktri	unirand 100
  aout	oscili 0.8, 440+ktri, 1		; & listen
  outs	aout, aout
endin

instr 2
  seed 593
  ktri	unirand 100
  aout	oscili 0.8, 440+ktri, 1		; & listen
  outs	aout, aout
endin

</CsInstruments>
<CsScore>
; sine wave
f 1 0 16384 10 1

i 1 0 2
i 2 3 2
e
</CsScore>
</CsoundSynthesizer>
