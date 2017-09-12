<CsoundSynthesizer>
<CsOptions>
-dnm0
</CsOptions>
<CsInstruments>

;test local iArrays
;jh march 2013

instr 1
          prints    "iArr in instr %d:\n", p1
iArr[]    init     4
icounter  =        0
  until (icounter >= 4) do
iArr[icounter] =   icounter ^ 2
          prints   " iArr[%d] = %f\n", icounter, iArr[icounter]
icounter  +=       1
  od
endin

instr 2
          prints    "iArr in instr %d:\n", p1
iArr[]    init     4
icounter  =        0
  until (icounter >= 4) do
iArr[icounter] =   icounter ^ 2 + 1
          prints   " iArr[%d] = %f\n", icounter, iArr[icounter]
icounter  +=       1
  od
endin

</CsInstruments>    
<CsScore>
i 1 0 0
i 2 0 0
</CsScore>
</CsoundSynthesizer>
Prints:
iArr in instr 1:
 iArr[0] = 0.000000
 iArr[1] = 1.000000
 iArr[2] = 4.000000
 iArr[3] = 9.000000
iArr in instr 2:
 iArr[0] = 1.000000
 iArr[1] = 2.000000
 iArr[2] = 5.000000
 iArr[3] = 10.000000

