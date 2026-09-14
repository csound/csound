<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
giIR ftgen 1, 0, 8, -2, 1
instr 1
  aInput = .5
  aOutput dconv aInput, p4, giIR
endin
</CsInstruments>
<CsScore>
; All three sizes must fail at init.
i 1 0 .01 0
i 1 0 .01 .5
i 1 0 .01 -1
e
</CsScore>
</CsoundSynthesizer>
