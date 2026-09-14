<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "test for UDO recursion depth exception",
  "expect": {
    "exit": "nonzero",
    "stderr_regex": ["(?i)recurs(?:ion|ive).*(?:depth|limit)"]
  },
  "skip": "Known defect: recursive UDO exits by signal instead of reporting a recursion limit; the old runner counted the crash as a pass."
}
*/
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