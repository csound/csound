<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iValues[] fillarray 0, 1, 2
  iScalar[] cmp iValues, "<>", 1
endin
instr 2
  iValues[] fillarray 0, 1, 2
  iArray[] cmp iValues, "===", iValues
endin
instr 3
  iValues[] fillarray 0, 1, 2
  iRange[] cmp 0, "<", iValues, "<bad", 2
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
e
</CsScore>
</CsoundSynthesizer>
