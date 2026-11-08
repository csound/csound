<CsTest>
description = "array bitwise operators reject unequal lengths and insufficient output capacity"
[expect]
exit = "nonzero"
stderr = ["array bitwise: operand lengths do not match", "Array too small"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr UnequalLengths
  kLeft[] fillarray 3, 6
  kRight[] fillarray 5
  kOut[] = kLeft & kRight
endin
instr OutputTooSmall
  kLeft[] fillarray 3, 6, 1
  kRight[] fillarray 5, 3, 1
  trim_i kLeft, 2
  trim_i kRight, 2
  kOut[] = kLeft | kRight
  ; The next cycle needs three entries; the output only has two.
  trim kLeft, 3
  trim kRight, 3
endin
</CsInstruments>
<CsScore>
i "UnequalLengths" 0 .015625
i "OutputTooSmall" .03125 .03125
e
</CsScore>
</CsoundSynthesizer>
