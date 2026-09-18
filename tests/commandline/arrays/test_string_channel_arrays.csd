<CsTest>
description = "string channel arrays read initial values and copy changing strings"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"
sr = 1024
ksmps = 32

chnset "initial left", "left"
chnset "initial right", "right"

instr 1
  SNames[] fillarray "left", "right"
  SInitial[] chngets SNames
  assert(strcmp(SInitial[0], "initial left") == 0, "chngets: wrong initial left value\n")
  assert(strcmp(SInitial[1], "initial right") == 0, "chngets: wrong initial right value\n")

  ; Both channel names and values must use the array's element spacing.
  kCycle init 0
  kCycle += 1
  SLeft sprintfk "left %d", kCycle
  SRight sprintfk "right %d", kCycle
  SValues[] init 2
  SValues[0] = SLeft
  SValues[1] = SRight
  chnsets SValues, SNames
  SRead[] chngets SNames
  if strcmpk(SRead[0], SLeft) != 0 || strcmpk(SRead[1], SRight) != 0 then
    printks "string channel array copy failed on cycle %d\n", 0, kCycle
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.125
</CsScore>
</CsoundSynthesizer>
