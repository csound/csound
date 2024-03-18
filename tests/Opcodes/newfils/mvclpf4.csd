<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
 asig[] init 4
 kenv linen p4,0.1,p3,0.1
 ain rand kenv 
 kfr expon 220, p3, 1760
 asig[0],asig[1],asig[2],asig[3] mvclpf4 ain,kfr,0.9
  out asig[p5]
endin

</CsInstruments>
<CsScore>
i1 0 5 0.9 0
i1 + 5 0.9 1
i1 + 5 0.9 2
i1 + 5 0.9 3
</CsScore>
</CsoundSynthesizer>

