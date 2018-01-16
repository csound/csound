<CsoundSynthesizer>

<CsInstruments>
instr 1
 kArr[] fillarray 1, 2, 3
 printks "   kArr[0] = %d\n", 0, kArr[0]
 kArr[0] = kArr[0] + 10
 printks "   kArr[0] = %d\n", 0, kArr[0]
 turnoff
endin
instr 2
 kArr[] fillarray 1, 2, 3
 printks "   kArr[0] = %d\n", 0, kArr[0]
 turnoff
 endin




</CsInstruments>

<CsScore>
i1 0 1
i2 1 1
e

</CsScore>

</CsoundSynthesizer>
