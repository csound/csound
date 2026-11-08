<CsTest>
description = "pindex replaces string results; pindex and passign use the queried event's strings"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
0dbfs = 1
giChecks init 0

opcode ReadField, S, i
  iIndex xin
  SResult pindex iIndex
  xout SResult
endop

instr 1
  SExpected[] fillarray "alpha", "", "a longer replacement string"
  SValue = "previous result"
  iIndex = 4
again:
  SValue pindex iIndex + 0.75
  if strcmp(SValue, SExpected[iIndex - 4]) != 0 then
    prints "pindex returned the wrong string for p%d\n", iIndex
    exitnow -1
  endif
  iIndex += 1
  if iIndex < 7 igoto again
  iValue pindex 7.75
  if iValue != 42 then
    prints "Numeric pindex changed fractional-index behavior\n"
    exitnow -1
  endif
  SUdo ReadField 4
  if strcmp(SUdo, "alpha") != 0 then
    prints "pindex did not read the enclosing event in a UDO\n"
    exitnow -1
  endif
  giChecks += 1
endin

instr 2
  aResult subinstr 3, "child"
  out aResult
endin

instr 3
  SDirect strget p4
  SIndexed pindex 4
  SAssigned passign 4, 4
  if strcmp(SDirect, "child") != 0 || strcmp(SIndexed, "host") != 0 || strcmp(SAssigned, "host") != 0 then
    prints "Subinstrument: direct='%s', pindex='%s', passign='%s'\n", SDirect, SIndexed, SAssigned
    exitnow -1
  endif
  giChecks += 1
  aZero = 0
  out aZero
endin

instr 99
  if giChecks != 3 then
    prints "Expected three pindex checks, got %g\n", giChecks
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.125 "alpha" "" "a longer replacement string" 42
i 1 0.25 0.125 "alpha" "" "a longer replacement string" 42
i 2 0.5 0.125 "host"
i 99 0.75 0.0625
e
</CsScore>
</CsoundSynthesizer>
