<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
instr 1
    kA1[]   fillarray       0, 1, 2, 3, 4, 5, 6, 7 ; <-- 8 elements
            printf          "lenarray(kA1) before slicearray: %d\n", 1, lenarray:k(kA1)
            kA1     slicearray      kA1, 1, 4 ; <-- 4 elements
            printf          "lenarray(kA1) AFTER  slicearray: %d\n", 1, lenarray:k(kA1)
            trim            kA1, 4
            printks         "kA1 after trim: { ", 0
      kCnt    =               0
 
    while (kCnt < lenarray:k(kA1)) do
                    printf  "%d ", kCnt + 1, kA1[kCnt]
            kCnt    +=      1
     od
        
            printks         "}\n", 0
            turnoff
endin


</CsInstruments>

<CsScore>
i1  0 0.1
e
</CsScore>

</CsoundSynthesizer>
