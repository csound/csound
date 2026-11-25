<CsTest>
description = "a mixed fixed and generic legacy UDO signature fails without a crash"

[expect]
exit = "nonzero"
stderr = ["invalid output argument types", "i[*][]", "Parsing failed"]
output_excludes = ["Segmentation fault", "Bus error"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode Invalid, i[*][], i[*]
  iValues[*] xin
  xout iValues
endop

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
