<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode cycle, i, ik[]
  indx, kvals[] xin
  ival = i(kvals, indx % lenarray(kvals))
  xout ival
endop

instr 1
ival = cycle(p4, [0,4,2,3,1])
endin

</CsInstruments>
<CsScore>
i1 0 0.001
</CsScore>
</CsoundSynthesizer>
