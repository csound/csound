<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

instr 1

iArr[] fillarray 1,2,3,4
iDCT[] dct iArr
iiDCT[] dctinv iDCT

prints "%.1f %.1f %.1f %.1f => %.1f %.1f %.1f %.1f\n",
        iArr[0], iArr[1], iArr[2], iArr[3], 
        iiDCT[0], iiDCT[1], iiDCT[2], iiDCT[3]
                
endin
</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
