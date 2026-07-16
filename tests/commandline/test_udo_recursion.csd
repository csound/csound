<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

opcode Crashy():i
  ikeepGoing = 1
  while (ikeepGoing == 1) do
    if (ikeepGoing == 1) then
      xout -1
    endif
    ivalue = Crashy()
  od
  xout 0
endop

opcode RecurseOK(cnt:i):i
  cnt += 1
  while (cnt < 10) do
    print cnt
    xout RecurseOK(cnt)
  od
    xout 0
endop

instr 1
  ivalue = Crashy()
endin

instr 2
  ret:i = RecurseOK(0)
  prints "carried on"
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
</CsScore>
</CsoundSynthesizer>