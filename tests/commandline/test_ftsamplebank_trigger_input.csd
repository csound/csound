<CsTest>
description = "ftsamplebank preserves shared triggers at init and performance"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
instr 1
  ; This fixture directory contains no sound files; no tables need loading.
  kTrigger init 1
  kCycle timeinstk
  kExpected = (kCycle < 3 ? 1 : kCycle % 2)
  kTrigger = kExpected
  kFirst ftsamplebank "arrays", 100, kTrigger, 0, 0, 0
  iTrigger = i(kTrigger)
  if iTrigger != 1 then
    prints "FAIL: ftsamplebank changed its trigger during initialization\n"
    exitnow -1
  endif
  if kTrigger != kExpected then
    printks "FAIL: ftsamplebank consumed a shared trigger\n", 0
    exitnowk -1
  endif
  kSecond ftsamplebank "arrays", 200, kTrigger, 0, 0, 0
  if kTrigger != kExpected then
    printks "FAIL: second ftsamplebank changed its input\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001
e
</CsScore>
</CsoundSynthesizer>
