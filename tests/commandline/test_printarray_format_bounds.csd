<CsTest>
description = "printarray handles long strings, wide fields and long integer formats"
[expect]
exit = 0
output = ["2 %d", "END", "label-"]
output_regex = ['"0{2999}1"', '0{300}2', ' {2999}2']
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
  kValues[] fillarray 2
  SLong sprintf "%03000d", 1
  SStrings[] fillarray SLong
  SLabel init "short"
  SPrefix sprintf "%0300d", 0
  SFormat strcat SPrefix, "%d"
  printarray iValues, SFormat
  printarray kValues, 1, SFormat
  printarray iValues, "%d %%d"
  printarray SStrings, 1
  printarray kValues, -1, "%3000.0f", SLabel
  iMatrix[][][] init 1, 1, 1
  iMatrix[0][0][0] = 2
  printarray iMatrix, "%3000.0f"
  ; Growing the label must not leave printarray holding the old allocation.
  SLabel sprintfk "label-%s", SLong
  printks "END\n", 0
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
e
</CsScore>
</CsoundSynthesizer>
