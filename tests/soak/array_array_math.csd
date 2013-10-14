<CsoundSynthesizer>
<CsOptions>
-n -m128
</CsOptions>
<CsInstruments>

  instr 1

;create array and fill with numbers 1..10 resp .1..1
kArr1[] fillarray 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
kArr2[] fillarray 1, 2, 3, 5, 8, 13, 21, 34, 55, 89

;print contents
        printf  "%s", 1, "\nkArr1:\n"
kndx    =       0
  until kndx == lenarray(kArr1) do
        printf  "kArr1[%d] = %f\n", kndx+1, kndx, kArr1[kndx]
kndx    +=      1
  od
        printf  "%s", 1, "\nkArr2:\n"
kndx    =       0
  until kndx == lenarray(kArr2) do
        printf  "kArr2[%d] = %f\n", kndx+1, kndx, kArr2[kndx]
kndx    +=      1
  od

;add arrays
kArr3[] =       kArr1 + kArr2

;print content
        printf  "%s", 1, "\nkArr1 + kArr2:\n"
kndx    =       0
  until kndx == lenarray(kArr3) do
        printf  "kArr3[%d] = %f\n", kndx+1, kndx, kArr3[kndx]
kndx    +=      1
  od

;subtract arrays
kArr4[] =       kArr1 - kArr2

;print content
        printf  "%s", 1, "\nkArr1 - kArr2:\n"
kndx    =       0
  until kndx == lenarray(kArr4) do
        printf  "kArr4[%d] = %f\n", kndx+1, kndx, kArr4[kndx]
kndx    +=      1
  od

;multiply arrays
kArr5[] =       kArr1 * kArr2

;print content
        printf  "%s", 1, "\nkArr1 * kArr2:\n"
kndx    =       0
  until kndx == lenarray(kArr5) do
        printf  "kArr5[%d] = %f\n", kndx+1, kndx, kArr5[kndx]
kndx += 1
  od

;divide arrays
kArr6[] =       kArr1 / kArr2

;print content
        printf  "%s", 1, "\nkArr1 / kArr2:\n"
kndx    =       0
  until kndx == lenarray(kArr6) do
        printf  "kArr5[%d] = %f\n", kndx+1, kndx, kArr6[kndx]
kndx += 1
  od

;turnoff
        turnoff
        
  endin
  
</CsInstruments>
<CsScore>
i 1 0 .1
</CsScore>
</CsoundSynthesizer>
