<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

;create and fill an array
kArr[]  genarray_i 1, 9

;print the content
        printf  "%s", 1, "kArr = whole array\n"
kndx    =       0
  until kndx == lenarray(kArr) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr[kndx]
kndx    +=      1
  od

;build new arrays for the slices
kArr1[] init    5
kArr2[] init    4
kArr3[] init    3

;put in first five and last four elements
kArr1   slicearray kArr, 0, 4
kArr2   slicearray kArr, 5, 8
; and three values from 1, 1+2 and 1+2+3
kArr3   slicearray kArr, 1, 5, 2

;print the content
        printf  "%s", 1, "\nkArr1 = slice from index 0 to index 4\n"
kndx    =       0
  until kndx == lenarray(kArr1) do
        printf  "kArr1[%d] = %f\n", kndx+1, kndx, kArr1[kndx]
kndx    +=      1
  od
        printf  "%s", 1, "\nkArr2 = slice from index 5 to index 8\n"
kndx    =       0
  until kndx == lenarray(kArr2) do
        printf  "kArr2[%d] = %f\n", kndx+1, kndx, kArr2[kndx]
kndx    +=      1
  od
        printf  "%s", 1, "\nkArr3 = slice from index 1 to index 5 inc2\n"
kndx    =       0
  until kndx == lenarray(kArr3) do
        printf  "kArr3[%d] = %f\n", kndx+1, kndx, kArr3[kndx]
kndx    +=      1
  od

        turnoff
endin


</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
