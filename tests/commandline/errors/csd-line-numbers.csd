<CsTest>
description = "report orchestra line numbers from the start of the CSD after an embedded file"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR in instr 1 (opcode table3.a) line 22:"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsFileB filename="csd-line-number-test.txt">
aGVsbG8=
</CsFileB>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
a1 table3 0, 999, 1
out a1
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
