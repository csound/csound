<CsoundSynthesizer>
<CsInstruments>
instr 1 ;; Addition
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kS + kT
  printk2 kans[0]
  printk2 kans[1]
endin
instr 2 ;; Subtraction
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kT - kS
  printk2 kans[0]
  printk2 kans[1]
endin
instr 3 ;; Multiplication
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kT * kS
  printk2 kans[0]
  printk2 kans[1]
endin
instr 4 ;; Division
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kT / kS
  printk2 kans[0]
  printk2 kans[1]
endin
instr 5 ;; Addition
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kans[] init 2
  kans = kS + 2.5
  printk2 kans[0]
  printk2 kans[1]
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
