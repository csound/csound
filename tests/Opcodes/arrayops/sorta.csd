<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
 kArr[] fillarray 1,3,2,7,4
 kSorted[] sorta kArr
 kn = 0
 while kn < lenarray(kSorted) do
  printk2 kSorted[kn]
  kn += 1
 od
 turnoff
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>