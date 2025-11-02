<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

gauss[] init 1 //
pitchamdf@global:i[] = [1,2,3]

instr 1
pitch:i[] = [1,2,3,4]
printarray(pitchamdf)
gauss[0] = 1
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>