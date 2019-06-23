<CsoundSynthesizer>

<CsInstruments>
instr 1
   ivalues[][] init 6, 3
   ivalues fillarray 0,   1,  2, \
                     10, 11, 12, \
                     20, 21, 22, \
                     30, 31, 32, \
                     40, 41, 42, \
                     50, 51, 52
   ivalues *= 100
   printk 1, ivalues[4][2]
endin

</CsInstruments>

<CsScore>
i1 0 0.1
e
</CsScore>

</CsoundSynthesizer>
