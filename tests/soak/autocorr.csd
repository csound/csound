<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

0dbfs = 1

instr 1
kin[] fillarray 1,0,1,0,1,0,1
kout[] autocorr kin
kcnt init 0
while kcnt < lenarray(kout) do
 printk2 kout[kcnt]
 kcnt += 1
od
turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>