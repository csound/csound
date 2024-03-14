<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
 kenv linen p4,0.1,p3,0.1
 ain rand kenv 
 kfr expon 220, p3, 1760
 asig mvclpf3 ain,kfr,0.9
   out asig
endin

</CsInstruments>
<CsScore>
i1 0 5 0.9
</CsScore>
</CsoundSynthesizer>
