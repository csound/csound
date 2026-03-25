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
  exitnowk(-1)
endif
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

