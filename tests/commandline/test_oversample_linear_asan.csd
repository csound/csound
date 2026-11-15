<CsTest>
description = "regression test for csound/csound#3617: linear UDO converter buffer underallocation with oversample 3"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1

opcode Counter, k, 0
  oversample 3, 4
  kCount timeinstk
  xout kCount
endop

instr 1
  kCount Counter
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
t 0.1
</CsScore>
</CsoundSynthesizer>
