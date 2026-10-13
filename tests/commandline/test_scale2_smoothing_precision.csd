<CsTest>
description = "scale2 preserves slow smoothing and decay in float and double builds"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckSmoothing
  iHalfTime = p4
  iTarget = p5
  kSamples init 0
  ; Move toward the target for two seconds, then decay toward zero.
  kInput = (kSamples < 2*sr ? 1 : 0)
  kSmoothed scale2 kInput, 0, iTarget, 0, 1, iHalfTime
  kSamples += 1
  if kSamples == sr || kSamples == 2*sr || kSamples == 3*sr || kSamples == 4*sr then
    kSeconds = kSamples/sr
    if kSeconds <= 2 then
      kExpected = iTarget*(1-pow(.5, kSeconds/iHalfTime))
    else
      kAtTwoSeconds = iTarget*(1-pow(.5, 2/iHalfTime))
      kExpected = kAtTwoSeconds*pow(.5, (kSeconds-2)/iHalfTime)
    endif
    if !(abs(kSmoothed-kExpected) < .000003) then
      printks "scale2 half-time %g, target %g, time %g: expected %.9f, got %.9f\n", 0, iHalfTime, iTarget, kSeconds, kExpected, kSmoothed
      exitnowk -1
    endif
    gkChecks += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) != 16 then
    prints "All four smoothed signals must reach all four check times\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; A two-second half-time reaches half the target, then half that value.
i "CheckSmoothing" 0 4.01 2 1
i "CheckSmoothing" 0 4.01 2 -1
; At this half-time, a float coefficient rounded to 1 would prevent all motion.
i "CheckSmoothing" 0 4.01 1024 1
i "CheckSmoothing" 0 4.01 1024 -1
i "CheckCompletion" 4.02 .01
e
</CsScore>
</CsoundSynthesizer>
