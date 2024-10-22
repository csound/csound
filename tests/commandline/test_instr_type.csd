<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr test
 test2:Instr = test
 i1 nstrnum test2
 S1 = test2
 prints "instr %d %s\n", i1, S1
 schedule this_instr, 1, 1
endin

event_i "i", test, 0, 1

</CsInstruments>
<CsScore>
f 0 2
</CsScore>
</CsoundSynthesizer>
