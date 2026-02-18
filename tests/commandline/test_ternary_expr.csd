<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

instr 1
i1 = 0
i1 = i1 < 0 ? 0: 1
print i1
test:b = i1 > 0 ? true : false
print test
k1 = 1
k2 = 0
k1 = k1 > 0 ? k1: k2
printk2 k1
asig1 = 0
asig2 = rand(0.1)
out (k1 < 0 ? asig1 : asig2)
endin

</CsInstruments>
<CsScore>
i1 1 1
</CsScore>
</CsoundSynthesizer>


