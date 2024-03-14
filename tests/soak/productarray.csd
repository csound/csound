<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
 kArr1[] fillarray 1,3,2,7,4
 k1 product kArr1
 printk2 k1
 turnoff
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>