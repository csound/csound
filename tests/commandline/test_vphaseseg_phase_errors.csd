<CsTest>
description = "Reject invalid vphaseseg phases"

[expect]
exit = "nonzero"
stderr = ["vphaseseg: invalid phase"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
giOut ftgen 0, 0, 4, -2, 0
giA ftgen 0, 0, 4, -2, 1, 2, 3, 4
instr 1
 iPhase = (p4 == 0 ? sqrt(-1) : exp(1000))
 vphaseseg iPhase, giOut, 4, giA, 1, giA
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 1
e
</CsScore>
</CsoundSynthesizer>
