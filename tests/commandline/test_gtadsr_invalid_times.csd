<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
 kEnv gtadsr 1, p4, p5, .5, .1, 1
endin
instr 2
 aEnv gtadsr 1, p4, p5, .5, .1, 1
endin
instr 3
 aInput = 1
 aEnv gtadsr aInput, p4, p5, .5, .1, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1 .1
i 1 0 .01 .1 -1
i 1 0 .01 1e20 .1
i 1 0 .01 .1 1e20
i 2 0 .01 -1 .1
i 2 0 .01 .1 -1
i 2 0 .01 1e20 .1
i 2 0 .01 .1 1e20
i 3 0 .01 -1 .1
i 3 0 .01 .1 -1
i 3 0 .01 1e20 .1
i 3 0 .01 .1 1e20
e
</CsScore>
</CsoundSynthesizer>
