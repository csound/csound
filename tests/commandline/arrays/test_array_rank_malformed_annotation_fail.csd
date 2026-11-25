<CsTest>
description = "a mixed fixed and generic array annotation fails without a crash"

[expect]
exit = "nonzero"
stderr = ["Invalid array type annotation: i[][*]", "Parsing failed"]
output_excludes = ["Segmentation fault", "Bus error"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode Invalid(values:i[][*]):i
  xout 0
endop

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
