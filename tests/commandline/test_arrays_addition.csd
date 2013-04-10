<CsoundSynthesizer>
<CsInstruments>
instr 1
kS[] init 2
 kS[0] = 1
 kS[1] = 2
printk2 kS[0]
kT[] init 2
kT[0] = 3
kT[1] = 4
kans[] init 2
kans = kS + kT
printk2 kans[0]
printk2 kans[1]
endin
</CsInstruments>
<CsScore>
i1 0 .5
</CsScore>
</CsoundSynthesizer>
