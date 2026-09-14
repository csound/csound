<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
giInitial ftgen 0, 0, 8, -2, 0
giRules ftgen 0, 0, 8, -2, 0
instr 1
 iOutput ftgen 0, 0, 8, -2, 0
 cell 1, 0, iOutput, giInitial, giRules, p4
endin
instr 2
 iInitial ftgen 0, 0, 8, -2, p4
 iRules ftgen 0, 0, -p5, -2, 0
 iOutput ftgen 0, 0, 8, -2, 0
 cell 1, 0, iOutput, iInitial, iRules, 8
endin
instr 3
 iRules ftgen 0, 0, 8, -2, 2
 iOutput ftgen 0, 0, 8, -2, 0
 ; The first lookup is valid, but its result cannot index the next generation.
 cell 1, 0, iOutput, giInitial, iRules, 8
endin
</CsInstruments>
<CsScore>
i 1 0 .02 0
i 1 0 .02 -1
i 1 0 .02 .5
i 1 0 .02 1e30
i 1 0 .02 9
i 2 0 .02 -1 8
i 2 0 .02 1e30 8
i 2 0 .02 1 1
i 3 0 .02
e
</CsScore>
</CsoundSynthesizer>
