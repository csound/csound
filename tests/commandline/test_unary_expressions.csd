<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs =	1

instr 1
  // bitwise-not legacy
  inum1 = 5
  iansw1 = ¬inum1
  if iansw1 == -6 then
    prints "pass ¬5 is -6\n"
  else
    prints "fail ¬5 isn't -6\n"
  endif

  // bitwise-not modern
  iansw2 = ~-3
  if iansw2 == 2 then
    prints "pass \\~-3 is 2\n"
  else
    prints "fail \\~-3 isn't 2\n"
  endif

endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
