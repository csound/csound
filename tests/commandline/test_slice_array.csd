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
</CsInstruments>
<CsScore>
i1 0 0.001
</CsScore>
</CsoundSynthesizer>


