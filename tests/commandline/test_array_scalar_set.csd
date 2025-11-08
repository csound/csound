<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1


instr 1
arr:k[][] init 2, 10
arr = 1
if arr[1][9] != 1 then
  event "i", 2, 0, 0
endif
endin

instr 2
exitnow(-1)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

