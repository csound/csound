<CsoundSynthesizer>
<CsOptions>
-dnm0
</CsOptions>
<CsInstruments>

;test local kArrays
;jh march 2013

instr 1
          printks   "kArr in instr %d:\n", 0, p1
kArr[]    init     4
kcounter  =        0
  until (kcounter == 4) do
kArr[kcounter] =   kcounter ^ 2
          printf   " kArr[%d] = %f\n", kcounter+1, kcounter, kArr[kcounter]
kcounter  +=       1
  od
          turnoff
endin

instr 2
          printks   "kArr in instr %d:\n", 0, p1
kArr[]    init     4
kcounter  =        0
  until (kcounter == 4) do
kArr[kcounter] =   kcounter ^ 2 + 1
          printf   " kArr[%d] = %f\n", kcounter+1, kcounter, kArr[kcounter]
kcounter  +=       1
  od
          turnoff
endin

</CsInstruments>    
<CsScore>
i 1 0 1
i 2 0 1
</CsScore>
</CsoundSynthesizer>
Prints:
kArr in instr 1:
 kArr[0] = 0.000000
 kArr[1] = 1.000000
 kArr[2] = 4.000000
 kArr[3] = 9.000000
kArr in instr 2:
 kArr[0] = 1.000000
 kArr[1] = 2.000000
 kArr[2] = 5.000000
 kArr[3] = 10.000000

