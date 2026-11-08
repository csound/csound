<CsTest>
description = "maparray rejects unsupported call records and insufficient output capacity"
[expect]
exit = "nonzero"
stderr = ["maparray: unsupported init-time function: invalue", "Array too small"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr UnsupportedFunction
  iInput[] fillarray 1
  iOutput[] maparray iInput, "invalue"
endin
instr GrowingInput
  kInput[] fillarray 4, 9, 16
  trim_i kInput, 2
  kOutput[] maparray kInput, "sqrt"
  trim kInput, 3
endin
</CsInstruments>
<CsScore>
i "UnsupportedFunction" 0 .015625
i "GrowingInput" .03125 .03125
e
</CsScore>
</CsoundSynthesizer>
