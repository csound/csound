<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
 kArr1[] fillarray 1,3,2,7,4
 kArr2[] fillarray 2,3,1,2,5
 kd dot kArr1,kArr2
 printk2 kd
 turnoff
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
