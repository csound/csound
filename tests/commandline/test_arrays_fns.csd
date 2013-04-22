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
instr 3 ;; tabmap
  kS[] init 10
  kT[] init 10
  kS   tabgen 0,10,0.01
  kT   tabmap kS,"sin"
  printk2 kS[0]
  printk2 kS[4]
  printk2 kS[9]
  printk2 kT[0]
  printk2 kT[4]
  printk2 kT[9]
endin
instr 4 ;; tabslice
  kS[] init 10
  kT[] init 5
  kS   tabgen 0,10,0.01
  kT[]   tabslice kS,3,7
  printk2 kT[0]
  printk2 kT[1]
  printk2 kT[2]
  printk2 kT[3]
  printk2 kT[4]
endin
instr 5 ;; scalet
  kT[]   tabgen 0,10
  printk2 kT[0]
  printk2 kT[1]
  printk2 kT[2]
  printk2 kT[3]
  printk2 kT[4]
  printk2 kT[5]
  printk2 kT[6]
  printk2 kT[7]
  printk2 kT[8]
  printk2 kT[9]
        scalet kT,0,1
  printk2 kT[0]
  printk2 kT[1]
  printk2 kT[2]
  printk2 kT[3]
  printk2 kT[4]
  printk2 kT[5]
  printk2 kT[6]
  printk2 kT[7]
  printk2 kT[8]
  printk2 kT[9]
endin
</CsInstruments>
<CsScore>
i1 0 0.1
i2 0.1 0.1
i3 0.2 0.1
i4 0.3 0.1
i5 0.4 0.1
</CsScore>
</CsoundSynthesizer>
