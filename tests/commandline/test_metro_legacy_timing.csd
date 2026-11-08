<CsTest>
description = "metro keeps historical timing while metro2 discards completed cycles"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --error-deprecated
</CsOptions>
<CsInstruments>
sr = 6400
ksmps = 64
nchnls = 1
gkChecks init 0

; This difference is intentional. Do not change metro to match metro2.
; At kr=100, three cycles at 250 Hz leave three extra metro triggers.
instr 1
  kCycle timeinstk
  kFrequency = (kCycle <= 3 ? 250 : 0)
  kLegacy metro kFrequency
  kCorrect metro2 kFrequency, .5
  kExpectedLegacy = (kCycle <= 6 ? 1 : 0)
  kExpectedCorrect = (kCycle <= 3 ? 1 : 0)
  if kLegacy != kExpectedLegacy || kCorrect != kExpectedCorrect then
    printks "cycle %g: metro=%g expected=%g, metro2=%g expected=%g\n", 0, kCycle, kLegacy, kExpectedLegacy, kCorrect, kExpectedCorrect
    exitnowk 1
  endif
  if kCycle == 10 then
    gkChecks += 1
    turnoff
  endif
endin

; Preserve metro's initial trigger and nonzero-phase startup as well.
instr 2
  kCycle timeinstk
  kTick metro 25, p4
  iPhase = frac(p4)
  iHeld = (iPhase == 0 ? 1 : 0)
  kExpected = ((iPhase*4+kCycle-iHeld) % 4 == 0 ? 1 : 0)
  if kTick != kExpected then
    printks "metro phase %g cycle %g: %g expected %g\n", 0, p4, kCycle, kTick, kExpected
    exitnowk 1
  endif
  if kCycle == 10 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 4 then
    prints "metro timing checks did not complete\n"
    exitnow 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .2
i 2 0 .2 0
i 2 0 .2 .25
i 2 0 .2 1
i 99 .3 .01
e
</CsScore>
</CsoundSynthesizer>
