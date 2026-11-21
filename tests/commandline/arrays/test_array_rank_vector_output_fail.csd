<CsTest>
description = "a vector-producing opcode cannot replace a declared matrix rank"

[expect]
exit = "nonzero"
stderr = ["rank mismatch", "expected 2, got 1"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iInput[] fillarray 1, 2, 3, 4
  iMatrix[][] rfft iInput
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
