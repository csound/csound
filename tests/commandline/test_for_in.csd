<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
j:i init 0
arr:i[] fillarray 1,2,3
for iin in arr do
 print iin
od
endin

instr 2
j:k init 0
arr:k[] fillarray 1,2,3
for j in arr do
 printk2 j
od
endin


</CsInstruments>
<CsScore>
i1 0 0
i2 0 0.001
</CsScore>
</CsoundSynthesizer>


