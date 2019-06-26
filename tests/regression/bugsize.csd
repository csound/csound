<CsoundSynthesizer>

<CsInstruments>
instr 1
        iStart      =           1
        iEnd        =           9

        iCnt        =           0
        while (iCnt < 3) do
        iArray[]    genarray    iStart + iCnt, iEnd - iCnt, 2
                    print       lenarray(iArray)
        iCnt        +=          1
        od
        iArray[]    fillarray   1,2,3,4,5,6,7,8,9
                    print       lenarray(iArray)
endin

instr 2
     iArr1[] fillarray 1,2
     iArr2[] fillarray 1,2,3

     iArr2 = sin(iArr1)
     print iArr2[0]
     print iArr2[1]
;;     print iArr2[2]
     print lenarray(iArr2)
endin
</CsInstruments>

<CsScore>
i1 0 0.1
i2 1 1.1
e
</CsScore>

</CsoundSynthesizer>
