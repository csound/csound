<CsTest>
description = "expon preserves slow rising and falling curves at both rates"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckCurve
  kCurve expon p4, 32, p5
  aCurve expon p4, 32, p5
  kAudio downsamp aCurve
  kStep init 0
  ; Check the midpoint, endpoint, and continued motion past the duration.
  if kStep == 16*kr || kStep == 32*kr || kStep == 48*kr then
    kExpected = p4 * (p5/p4)^(kStep/(32*kr))
    if !(abs(kCurve-kExpected) < .00001 && abs(kAudio-kExpected) < .00001) then
      printks "expon %g to %g at %g seconds: expected %.9f, got control=%.9f audio=%.9f\n", 0, p4, p5, kStep/kr, kExpected, kCurve, kAudio
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kStep += 1
endin

instr CheckCompletion
  if i(gkChecks) != 12 then
    prints "All four curves must reach all three check points\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Small changes must work in both directions and with negative endpoints.
i "CheckCurve" 0 48.01 1 1.05
i "CheckCurve" 0 48.01 1.05 1
i "CheckCurve" 0 48.01 -1 -1.05
i "CheckCurve" 0 48.01 -1.05 -1
i "CheckCompletion" 49 .01
e
</CsScore>
</CsoundSynthesizer>
