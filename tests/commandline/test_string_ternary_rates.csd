<CsTest>
description = "String ternaries keep init selection separate from control selection"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

instr InitSelection
  Sleft init "left"
  Sright init "right"
  ; p4 makes this an init-time condition (b). Its result must stay fixed
  ; even when both inputs change during performance.
  Sselected = (p4 > 0 ? Sleft : Sright)
  Sexpected strcpy "right"
  if p4 > 0 then
    Sexpected strcpy "left"
  endif
  if strcmp(Sselected, Sexpected) != 0 then
    prints "Wrong init-time selection\n"
    exitnow -1
  endif
  Sleft strcpyk "changed left"
  Sright strcpyk "changed right"
  if strcmpk(Sselected, Sexpected) != 0 then
    printks "Init-time selection ran during performance\n", 0
    exitnowk -1
  endif
endin

instr ControlSelection
  kCycle eventcycles
  ; Neither literal changes; only the control-time condition (B) changes.
  Sselected = (kCycle % 2 == 0 ? "even" : "odd")
  Scopy strcpyk Sselected
  if kCycle % 2 == 0 then
    Sexpected strcpyk "even"
  else
    Sexpected strcpyk "odd"
  endif
  if strcmpk(Sselected, Sexpected) != 0 || strcmpk(Scopy, Sexpected) != 0 then
    printks "Control-time selection is stale on cycle %d\n", 0, kCycle
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i "InitSelection" 0 .125 1
i "InitSelection" .25 .125 0
i "ControlSelection" .5 .125
e
</CsScore>
</CsoundSynthesizer>
