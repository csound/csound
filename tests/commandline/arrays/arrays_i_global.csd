<CsoundSynthesizer>
<CsOptions>
-dnm0
</CsOptions>
<CsInstruments>

;test global iArrays
;jh march 2013

giArr[]    init       4

instr 1
           prints     "Printing giArr[] in instr %d:\n [", p1
icounter   =          0
           until      (icounter == 3) do
           prints     "%f ", giArr[icounter]
icounter += 1
od
           prints     "%f]\n", giArr[3]
endin

instr 2
           prints     "Changing giArr[] in instr %d.\n", p1
icounter   =          0
           until      (icounter == 4) do
giArr[icounter] =     rnd(10)
           printf_i   " giArr[%d] = %f\n", icounter+1, icounter, giArr[icounter]
icounter   +=         1
od
endin

instr 3
           prints     "Printing giArr[] in instr %d:\n [", p1
icounter   =          0
           until      (icounter == 3) do
           prints     "%f ", giArr[icounter]
icounter   +=         1
od
           prints     "%f]\n", giArr[3]
endin
</CsInstruments>    
<CsScore>
i 1 0 0
i 2 0 0
i 3 0 0
</CsScore>
</CsoundSynthesizer>
Prints:
Printing giArr[] in instr 1:
 [0.000000 0.000000 0.000000 0.000000]
Changing giArr[] in instr 2.
  giArr[0] = 9.735000
  giArr[1] = 1.394045
  giArr[2] = 7.694886
  giArr[3] = 5.250046
Printing giArr[] in instr 3:
 [9.735000 1.394045 7.694886 5.250046]

