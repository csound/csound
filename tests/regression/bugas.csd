<CsoundSynthesizer>
<CsOptions>
-n -m128
</CsOptions>
<CsInstruments>

   instr 1 ;works

;create array and fill with numbers 1..10
   kArr[] fillarray 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
          printf  "%s", 1, "\ninstr 1:\n"

;print content
   kndx    =       0
   while kndx != lenarray(kArr) do
         printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr[kndx]
   kndx += 1
od

;turnoff
         turnoff
   endin

   instr 2 ;expected to add 10 to each element -- got destruction

;create array and fill with numbers 1..10
kArr[] fillarray 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
         printf  "%s", 1, "\ninstr 2:\n"

;print content
kndx    =       0
   until kndx == lenarray(kArr) do
         printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr[kndx]
kndx += 1
od

;add 10
kArr += 10

;print content
kndx    =       0
   until kndx == lenarray(kArr) do
         printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr[kndx]
kndx += 1
od

;turnoff
         turnoff
   endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 0 .1
</CsScore>
</CsoundSynthesizer>

