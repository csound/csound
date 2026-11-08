<CsTest>
description = "Reject invalid vphaseseg tables, vector sizes, and segment distances"

[expect]
exit = "nonzero"
stderr = ["source table too short", "invalid number of elements", "distances must be positive", "total distance must be finite", "expected table/distance pairs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
giOut ftgen 0, 0, 4, -2, 0
giA ftgen 0, 0, 4, -2, 0, 1, 2, 3
giShort ftgen 0, 0, 1, -2, 0
instr 1
 vphaseseg .5, giOut, 4, giShort, 1, giA
endin
instr 2
 vphaseseg .5, giOut, 4, giA, 1, giShort
endin
instr 3
 vphaseseg .5, giOut, p4, giA, 1, giA
endin
instr 4
 vphaseseg .5, giOut, 4, giA, p4, giA, p5, giA
endin
instr 5
 vphaseseg .5, giOut, 4, giA, 1
endin
instr 6
 vphaseseg .5, giOut, 4, giA
endin
instr 7
 iNan = sqrt(-1)
 vphaseseg .5, giOut, iNan, giA, 1, giA
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01 -1
i 3 0 .01 5
i 3 0 .01 1e20
i 4 0 .01 0 1
i 4 0 .01 1 0
i 4 0 .01 1 -1
i 4 0 .01 1e308 1e308
i 5 0 .01
i 6 0 .01
i 7 0 .01
e
</CsScore>
</CsoundSynthesizer>
