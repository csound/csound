<CsTest>
description = "audio channel arrays follow changed names when reading and writing"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 8
nchnls = 1

instr 1
  aFirst = 11
  aSecond = 22
  chnset aFirst, "first"
  chnset aSecond, "second"
  SNames[] fillarray "first", "second"
  kCycle timeinstk

  ; Swap only the reader's names, then restore them on the next block.
  if kCycle == 2 then
    SNames[0] strcpyk "second"
    SNames[1] strcpyk "first"
  elseif kCycle == 3 then
    SNames[0] strcpyk "first"
    SNames[1] strcpyk "second"
  endif
  aRead[] chngeta SNames
  kExpectedFirst = (kCycle == 2 ? 22 : 11)
  kExpectedSecond = (kCycle == 2 ? 11 : 22)
  if k(aRead[0]) != kExpectedFirst || k(aRead[1]) != kExpectedSecond then
    printks "audio array read ignored a changed name on block %d\n", 0, kCycle
    exitnowk(-1)
  endif
endin

instr 2
  aValues[] init 2
  aValues[0] = 11
  aValues[1] = 22
  SNames[] fillarray "write-first", "write-second"
  kCycle timeinstk
  if kCycle == 2 then
    SNames[0] strcpyk "write-second"
    SNames[1] strcpyk "write-first"
  elseif kCycle == 3 then
    SNames[0] strcpyk "write-first"
    SNames[1] strcpyk "write-second"
  endif
  chnseta aValues, SNames

  ; Read fixed names so matching mistakes in the two array opcodes cannot pass.
  aFirst chnget "write-first"
  aSecond chnget "write-second"
  kExpectedFirst = (kCycle == 2 ? 22 : 11)
  kExpectedSecond = (kCycle == 2 ? 11 : 22)
  if k(aFirst) != kExpectedFirst || k(aSecond) != kExpectedSecond then
    printks "audio array write ignored a changed name on block %d\n", 0, kCycle
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.032
i 2 0 0.032
; Reuse the instruments with fresh bindings.
i 1 0.04 0.032
i 2 0.04 0.032
</CsScore>
</CsoundSynthesizer>
