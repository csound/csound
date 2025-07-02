<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode assert(var1:InstrDef,var2:InstrDef):void
if nstrnum(var1) != nstrnum(var2) then
  prints "assert error for instrument number\n"
  exitnow(-1)
endif
S1 = str(var1)
S2 = str(var2)
if strcmp(S1,S2) != 0 then
  prints "assert error for instrument name\n"
  exitnow(-1)
endif
endop


instr Test
 test2:InstrDef = Test
 assert(test2, this_instr)
 schedule this_instr, 0.5, 0.5
endin

event_i "i", Test, 0, 0.5

</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>
