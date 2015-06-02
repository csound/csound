<CsoundSynthesizer>
<CsOptions>
-n -m128
</CsOptions>
<CsInstruments>


  instr 1

;create array and fill with numbers 1..10
kArr1[] fillarray 1, 2, 3, 4, 5, 6, 7, 8, 9, 10

;print content
        printf  "%s", 1, "\nInitial content:\n"
kndx    =       0
  until kndx == lenarray(kArr1) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr1[kndx]
kndx    +=      1
  od

;add 10
kArr2[] =       kArr1 + 10

;print content
        printf  "%s", 1, "\nAfter adding 10:\n"
kndx    =       0
  until kndx == lenarray(kArr2) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr2[kndx]
kndx    +=      1
  od

;subtract 5
kArr3[] =       kArr2 - 5

;print content
        printf  "%s", 1, "\nAfter subtracting 5:\n"
kndx    =       0
  until kndx == lenarray(kArr3) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr3[kndx]
kndx    +=      1
  od

;multiply by -1.5
kArr4[] =       kArr3 * -1.5

;print content
        printf  "%s", 1, "\nAfter multiplying by -1.5:\n"
kndx    =       0
  until kndx == lenarray(kArr4) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr4[kndx]
kndx    +=      1
  od

;divide by -3/2
kArr5[] =       kArr4 / -(3/2)

;print content
        printf  "%s", 1, "\nAfter dividing by -3/2:\n"
kndx    =       0
  until kndx == lenarray(kArr5) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr5[kndx]
kndx    +=      1
  od

;turnoff
        turnoff
  endin
  

</CsInstruments>
<CsScore>
i 1 0 .1
</CsScore>
</CsoundSynthesizer>
