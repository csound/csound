<CsTest>
description = "report the correct error line for an array annotation with one tab"

[expect]
exit = "nonzero"
stderr = ["syntax error", "line 16, columns 9,14"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  ; An input cannot declare a type.
  ; Keep one tab between i and [] to check the reported line.
  print x:i	[]
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
