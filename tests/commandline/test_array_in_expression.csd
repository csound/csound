<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1
instr 1
kA[] fillarray 1, 2, 3
kB[] fillarray 2, 4, 6
kC[] = (kA * kB) * 0.5
endin
</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


