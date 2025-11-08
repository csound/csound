<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
; keep original opcall using an array expression
sig:k sumarray fillarray(1,2,3)*2

; assert the array expression equals [2,4,6]
iarr[] = fillarray(1,2,3)*2
if (lenarray(iarr) != 3) then
  prints "error\n"
  exitnow(-1)
endif
if (iarr[0] != 2 || iarr[1] != 4 || iarr[2] != 6) then
  prints "error\n"
  exitnow(-1)
else
  prints "pass\n"
endif
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
