<CsTest>
description = "report the correct error line for an array annotation with one space"

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
  ; The error must point to print, not to this preceding line.
  print x:i []
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
