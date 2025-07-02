<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode assert(inst1:i, inst2:i):void
if inst1 != inst2 then
 exitnow(-1)
endif
endop

instr Test
assert(p1, 1.01)
endin
schedule("Test.01", 0, 0)

</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>
