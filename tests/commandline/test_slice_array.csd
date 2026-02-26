<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
arr:i[] = [1,2,3,4,5,6,7]
sl:i[] = arr[0 : 4, 2]
printarray(sl)
endin

instr 2
i1[] = [1 ... 4]
i2[] = i1[2:]
i3[] = i1[:2]
print(i2)
print(i3)
endin

</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
</CsScore>
</CsoundSynthesizer>


