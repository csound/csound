<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

opcode ManagedTable(number:i):i
  result:i ftgen number, 0, 8, -2, number
  ftfree result, 1
  xout result
endop

opcode InitOnlyWrapper(number:i):i
  result:i = ManagedTable(number)
  xout result
endop

instr TableOwner
  tableNumber:i = InitOnlyWrapper(p4)
  prints "table %d initialized\n", tableNumber
endin

instr VerifySecondTable
  length:i = ftlen(102)
  if (length != 8) then
    prints "nested UDO cleanup failed: table 102 length=%d\n", length
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i "TableOwner" 0    0.05 101
i "TableOwner" 0.01 0.10 102
i "VerifySecondTable" 0.06 0
</CsScore>
</CsoundSynthesizer>
