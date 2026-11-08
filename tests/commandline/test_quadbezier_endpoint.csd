<CsTest>
description = "quadbezier rejects an invalid endpoint segment"

[expect]
exit = "nonzero"
stderr = ["quadbezier endpoints must be finite and increasing"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giTable ftgen 1, 0, -16, "quadbezier", 0, 0, .5, 0, 1

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
