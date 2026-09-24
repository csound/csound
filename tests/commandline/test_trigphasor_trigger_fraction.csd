<CsTest>
description = "trigphasor audio reset uses the elapsed fraction since the zero crossing"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0
gkFailures init 0

instr 1
  kSample init 0
  aTrigger = kSample < p8 ? p4 : p5
  kRate = kSample < p8 ? p6 : p7
  aRate = kRate
  aAK trigphasor aTrigger, kRate, 0, 16, 4
  aAA trigphasor aTrigger, aRate, 0, 16, 4
  kAK downsamp aAK
  kAA downsamp aAA
  if kSample == p8 then
    ; Linear interpolation places the crossing within the previous interval.
    ; Only the part after that crossing contributes to the reset phase.
    kElapsed = p5/(p5-p4)
    kExpected = p8 == 0 ? 4 : 4 + p6*kElapsed
    printks "trigphasor trigger %g -> %g: expected=%.9f actual=%.9f/%.9f\n", 0, p4, p5, kExpected, kAK, kAA
    if abs(kAK-kExpected) + abs(kAA-kExpected) > .000001 then
      gkFailures += 1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin

instr 99
  if i(gkChecks) != 8 || i(gkFailures) != 0 then
    prints "trigphasor fractional reset failures=%g checks=%g\n", i(gkFailures), i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; previous trigger, current trigger, previous rate, current rate, trigger sample
i 1 0 .001 -1 1 .25 .25 2
i 1 .002 .001 -3 1 .25 .25 2
i 1 .004 .001 0 1 .25 .25 2
i 1 .006 .001 -1 1 .25 .75 2
i 1 .008 .001 -1 1 -.25 .75 2
i 1 .010 .001 -1 1 0 .75 2
i 1 .012 .001 -1 1 .25 .25 0
i 1 .014 .001 -1 1 -.25 -.25 0
i 99 .016 .001
e
</CsScore>
</CsoundSynthesizer>
