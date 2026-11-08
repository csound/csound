<CsTest>
description = "reject truncated GEN28 trajectory"

[expect]
exit = "nonzero"
stderr = ["GEN28: malformed trajectory point 2"]
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

giTrajectory ftgen 1, 0, 0, -28, "gen28_truncated.txt"

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
