<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
instr 1
k1[] fillarray 1,2,3,4
k3[] init 4

k2 = 2
k3 = k2 ^ k1
printk2 k3[0]
printk2 k3[1]
printk2 k3[2]
printk2 k3[3]
endin

instr 2
ii[][] init 3,4
print lenarray(ii,0)
print lenarray(ii,1)
print lenarray(ii,2)
endin

</CsInstruments>

<CsScore>
i2 0 0.2
</CsScore>

</CsoundSynthesizer>
