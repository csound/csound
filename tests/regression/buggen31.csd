<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>
0dbfs = 1
gisine  ftgen   0,0,4096,10,1
gi31    ftgen   0,0,4096,31,gisine, 1,1,0, 2,1,0, 3,1,0
instr 1
 a1 poscil  0.3,300,gi31
    out a1
endin
</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
