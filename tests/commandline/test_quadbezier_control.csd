<CsTest>
description = "quadbezier rejects an invalid control segment"

[expect]
exit = "nonzero"
stderr = ["quadbezier control point must lie within its segment"]
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

giTable ftgen 1, 0, -16, "quadbezier", 0, 17, .5, 16, 1

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
