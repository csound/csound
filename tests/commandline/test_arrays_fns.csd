<CsoundSynthesizer>
<CsInstruments>
instr 1 ;; lentab
  kS[] init 2
  printk2 lentab(kS)
endin
instr 2 ;; tabgen
  kS[] init 10
  kS   tabgen 0,10,2
  printk2 kS[0]
  printk2 kS[4]
  printk2 kS[9]
endin
</CsInstruments>
<CsScore>
i1 0 0.1
i2 0 0.2
</CsScore>
</CsoundSynthesizer>
