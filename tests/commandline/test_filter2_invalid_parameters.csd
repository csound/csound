<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  aInput = .5
  aOutput filter2 aInput, p4, p5, 1
endin
instr 2
  aInput = .5
  aOutput zfilter2 aInput, 0, 0, p4, p5, 1
endin
</CsInstruments>
<CsScore>
; Each opcode rejects two bad orders and two missing-coefficient cases.
i 1 0 .01 1e30 0
i 1 0 .01 1 -1
i 1 0 .01 2 0
i 1 0 .01 1 1
i 2 0 .01 1e30 0
i 2 0 .01 1 -1
i 2 0 .01 2 0
i 2 0 .01 1 1
e
</CsScore>
</CsoundSynthesizer>
