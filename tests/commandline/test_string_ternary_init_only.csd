<CsTest>
description = "Init-time string ternaries do not copy changing inputs during performance"
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
  kCycle eventcycles
  Sleft sprintfk "left %d", kCycle
  Sright sprintfk "right %d", kCycle
  ; p4 makes this an init-time condition. Capture the chosen text at init,
  ; then change both inputs before the expression on each control cycle.
  Sselected = (p4 > 0 ? Sleft : Sright)
  Sexpected strcpy Sselected
  if strcmpk(Sselected, Sexpected) != 0 then
    printks "Init-time selection changed on cycle %d: got '%s', expected '%s'\n", 0, kCycle, Sselected, Sexpected
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
; Check both branches over four control cycles each.
i "InitSelection" 0 .125 1
i "InitSelection" .25 .125 0
e
</CsScore>
</CsoundSynthesizer>
