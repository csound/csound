<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
0dbfs = 1

opcode phs(icount, iperiod):i
  xout (icount % iperiod) / iperiod
endop

opcode phs(iticks):i
  xout iticks / 100
endop

instr 1
  ires = phs(p4 / 2, 16)
  print ires
  ires2 = phs(16)
  print ires2
endin

</CsInstruments>
<CsScore>
i 1 0 0.1 8
</CsScore>
</CsoundSynthesizer>
