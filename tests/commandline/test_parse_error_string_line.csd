<CsTest>
description = "a syntax error at a string retains its reported line"

[expect]
exit = "nonzero"
stderr = ["syntax error, unexpected STRING_TOKEN", "line 13, columns 1,1"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
instr 1
"test\n" x
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
