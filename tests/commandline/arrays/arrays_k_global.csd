<CsoundSynthesizer>
<CsOptions>
-dnm0
</CsOptions>
<CsInstruments>

;test global iArrays
;jh march 2013

giArrLen  =        5
gkArr[]   init     giArrLen

instr 1
          printks   "Printing gkArr[] in instr %d:\n [", 0, p1
kcounter  =        0
  until (kcounter == giArrLen-1) do
          printf   "%f ", kcounter+1, gkArr[kcounter]
kcounter  +=       1
  od
          printf    "%f]\n", kcounter+1, gkArr[kcounter]
          turnoff
endin

instr 2
          printks   "Changing gkArr[] in instr %d.\n", 0, p1
kcounter  =        0
kLim      init     10
  until kcounter == giArrLen do
gkArr[kcounter] =  rnd(kLim)
          printf " gkArr[%d] = %f\n", kcounter+1, kcounter, gkArr[kcounter]
kcounter  +=       1
  od
          turnoff
endin

instr 3
          printks   "Printing gkArr[] in instr %d:\n [", 0, p1
kcounter  =        0
  until (kcounter == giArrLen-1) do
          printf   "%f ", kcounter+1, gkArr[kcounter]
kcounter  +=       1
  od
          printf    "%f]\n", kcounter+1, gkArr[kcounter]
          turnoff
endin
</CsInstruments>    
<CsScore>
i 1 0 1
i 2 0 1
i 3 0 1
</CsScore>
</CsoundSynthesizer>
Prints:
Printing gkArr[] in instr 1:
 [0.000000 0.000000 0.000000 0.000000 0.000000]
Changing gkArr[] in instr 2.
 gkArr[0] = 9.735000
 gkArr[1] = 1.394045
 gkArr[2] = 7.694886
 gkArr[3] = 5.250046
 gkArr[4] = 6.226652
Printing gkArr[] in instr 3:
 [9.735000 1.394045 7.694886 5.250046 6.226652]

