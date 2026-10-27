<CsTest>
description = "control channel arrays follow each member's changed name"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1

instr 1
  SNames[] fillarray "first", "second"
  kValues[] fillarray 11, 22
  kCycle timeinstk

  ; Swap the destinations, including a move to the last member's old name.
  if kCycle == 2 then
    SNames[0] strcpyk "second"
    SNames[1] strcpyk "first"
  elseif kCycle == 3 then
    SNames[0] strcpyk "first"
    SNames[1] strcpyk "second"
  endif
  chnsetk kValues, SNames

  kFirst chnget "first"
  kSecond chnget "second"
  kExpectedFirst = (kCycle == 2 ? 22 : 11)
  kExpectedSecond = (kCycle == 2 ? 11 : 22)
  if kFirst != kExpectedFirst || kSecond != kExpectedSecond then
    printks "wrong array destination on cycle %d: first=%d, second=%d\n", 0, kCycle, kFirst, kSecond
    exitnowk(-1)
  endif
endin

instr 2
  ; Keep the source values fixed while the reader swaps its channel names.
  chnset 11, "read-first"
  chnset 22, "read-second"
  SNames[] fillarray "read-first", "read-second"
  kCycle timeinstk
  if kCycle == 2 then
    SNames[0] strcpyk "read-second"
    SNames[1] strcpyk "read-first"
  elseif kCycle == 3 then
    SNames[0] strcpyk "read-first"
    SNames[1] strcpyk "read-second"
  endif
  kValues[] chngetk SNames
  kExpectedFirst = (kCycle == 2 ? 22 : 11)
  kExpectedSecond = (kCycle == 2 ? 11 : 22)
  if kValues[0] != kExpectedFirst || kValues[1] != kExpectedSecond then
    printks "wrong array source on cycle %d: first=%d, second=%d\n", 0, kCycle, kValues[0], kValues[1]
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.004
; Reuse the instrument with fresh bindings.
i 1 0.01 0.004
i 2 0 0.004
i 2 0.01 0.004
</CsScore>
</CsoundSynthesizer>
