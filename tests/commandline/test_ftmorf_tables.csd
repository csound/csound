<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
; Explicit guard points differ from the first sample.
giA ftgen 1, 0, 9, -2, 1, 1, 1, 1, 1, 1, 1, 1, 5
giB ftgen 2, 0, 9, -2, 3, 3, 3, 3, 3, 3, 3, 3, 7
giC ftgen 3, 0, 9, -2, 5, 5, 5, 5, 5, 5, 5, 5, 9
giList ftgen 4, 0, 2, -2, 1, 2
giSingle ftgen 5, 0, -1, -2, 2
gkChecks init 0

instr 1
  iResult ftgen 0, 0, 8, -2, 0
  kIndex init p4
  ftmorf kIndex, giList, iResult
  kMiddle tablei 3.5, iResult
  kEnd tablei 7.5, iResult
  kExpected = 1 + 2 * limit(p4, 0, 1)
  if kIndex != p4 || abs(kMiddle - kExpected) > .00001 || abs(kEnd - kExpected - 2) > .00001 then
    printks "ftmorf index %g became %g, middle %g end %g expected %g/%g\n", 0, p4, kIndex, kMiddle, kEnd, kExpected, kExpected + 2
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 2
  iResult ftgen 0, 0, 8, -2, 0
  kIndex init .5
  ftmorf kIndex, giSingle, iResult
  kMiddle tablei 3.5, iResult
  kEnd tablei 7.5, iResult
  if kIndex != .5 || kMiddle != 3 || kEnd != 5 then
    printks "ftmorf failed for a one-entry list\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 3
  iList ftgen 0, 0, 2, -2, 1, 2
  iResult ftgen 0, 0, 8, -2, 0
  kCycle init 0
  kCycle += 1
  kIndex = (kCycle == 1 ? 0 : .5)
  kTable = 3
  if kCycle == 2 then
    tablew kTable, 0, iList
  endif
  ftmorf kIndex, iList, iResult
  kMiddle tablei 3.5, iResult
  kEnd tablei 7.5, iResult
  kExpected = (kCycle == 1 ? 1 : 4)
  if kMiddle != kExpected || kEnd != kExpected + 2 then
    printks "ftmorf changed list cycle %g: middle %g end %g expected %g/%g\n", 0, kCycle, kMiddle, kEnd, kExpected, kExpected + 2
    exitnowk -1
  endif
  if kCycle == 2 then
    gkChecks += 1
    turnoff
  endif
endin

instr 4
  iList ftgen 0, 0, -3, -2, 1, 2, 3
  iResult ftgen 0, 0, 8, -2, 0
  kCycle init 0
  kCycle += 1
  kIndex = (kCycle <= 5 ? (kCycle - 1) * .5 : (9 - kCycle) * .5)
  ftmorf kIndex, iList, iResult
  kMiddle tablei 3.5, iResult
  kEnd tablei 7.5, iResult
  kExpected = 1 + 2 * kIndex
  if kMiddle != kExpected || kEnd != kExpected + 2 then
    printks "ftmorf moving index %g: middle %g end %g expected %g/%g\n", 0, kIndex, kMiddle, kEnd, kExpected, kExpected + 2
    exitnowk -1
  endif
  if kCycle == 9 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 10 then
    prints "not all ftmorf checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 0 .01 0
i 1 0 .01 .5
i 1 0 .01 1
i 1 0 .01 1.5
i 1 0 .01 2
i 1 0 .01 1e30
i 2 0 .01
i 3 0 .02
i 4 0 .05
i 99 .06 .01
e
</CsScore>
</CsoundSynthesizer>
