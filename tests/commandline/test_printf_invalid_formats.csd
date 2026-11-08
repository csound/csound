<CsTest>
description = "Formatting rejects unsupported formats, wrong types, and reused inputs"

[expect]
exit = "nonzero"
stderr = ["invalid format string", "argument type inconsistent with format", "output argument may not be the same as any of the input args"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  SResult sprintf "%ld", 1
endin
instr 2
  SResult sprintf "%d", "word"
endin
instr 3
  SResult = "word"
  SResult sprintf "%d%s", 1, SResult
endin
instr 4
  SFormat = "%d"
  SFormat sprintf SFormat, 1
endin
instr 5
  SResult sprintf "%*d", 8, 1
endin
instr 6
  SResult sprintf "%s", 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
i 4 0 .01
i 5 0 .01
i 6 0 .01
e
</CsScore>
</CsoundSynthesizer>
