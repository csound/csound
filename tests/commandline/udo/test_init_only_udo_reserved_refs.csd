<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

opcode VerifyReservedReferences(value:i):i
  active:b = isactive(this)
  if !active then
    prints "reused UDO lost this\n"
    exitnow(1)
  endif

  instrumentNumber:i = nstrnum(this_instr)
  if instrumentNumber <= 0 then
    prints "reused UDO lost this_instr\n"
    exitnow(1)
  endif

  xout value
endop

instr CallVerifier
  result:i = VerifyReservedReferences(p4)
  if result != p4 then
    prints "reserved reference check returned %d, expected %d\n", result, p4
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i "CallVerifier" 0    0 1
i "CallVerifier" 0.01 0 2
</CsScore>
</CsoundSynthesizer>
