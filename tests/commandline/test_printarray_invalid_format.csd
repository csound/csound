<CsTest>
description = "printarray rejects formats that cannot consume one numeric element"
[expect]
exit = "nonzero"
stderr = ["printarray: format does not match the array type", "printarray: format must use at most one conversion"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr 1
  iValues[] fillarray 2
  SFormat strget p4
  printarray iValues, SFormat
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 "%s"
i 1 .03125 .015625 "%n"
i 1 .0625 .015625 "%*f"
i 1 .09375 .015625 "%f %f"
e
</CsScore>
</CsoundSynthesizer>
