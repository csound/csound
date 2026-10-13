<CsTest>
description = "31-bit random generators use the same bipolar scale at startup and later knots"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckKnots
  ; Park-Miller reference: start at 2^30, apply (16807*state) mod (2^31-1),
  ; discard the first step, and scale each state as (2*state-(2^31-1))/2^31.
  iKnots[] fillarray .1315377880819142, .7556053218431771, -.541349867824465, \
    -.4672327623702586, -.7810408133082092, -.9529553833417594, -.3211352829821408
  ; Seed 0 and the upper endpoint use the same default as seed .5.
  kWhite rand 1, p4, 1
  aWhite rand 1, p4, 1
  kAudioWhite downsamp aWhite
  kHold randh 1, 8, p4, 1
  kLinear randi 1, 8, p4, 1
  kCubic randc 1, 8, p4, 1
  aHold randh 1, 8, p4, 1
  aLinear randi 1, 8, p4, 1
  aCubic randc 1, 8, p4, 1
  kAudioHold downsamp aHold
  kAudioLinear downsamp aLinear
  kAudioCubic downsamp aCubic
  kSample init 0
  ; White noise advances on every sample and starts at the third PRNG step.
  if kSample < 6 then
    kExpectedWhite = iKnots[kSample+1]
    if !(abs(kWhite-kExpectedWhite) < .00001 && abs(kAudioWhite-kExpectedWhite) < .00001) then
      printks "seed=%g white sample=%g: expected %g, got %g/%g\n", \
        0, p4, kSample, kExpectedWhite, kWhite, kAudioWhite
      exitnowk -1
    endif
  endif
  ; This midpoint also checks the cubic generator's first history value.
  if kSample == 2 then
    kMidpoint = (5*iKnots[1]+5*iKnots[2]-iKnots[0]-iKnots[3])/8
    if !(abs(kCubic-kMidpoint) < .00001 && abs(kAudioCubic-kMidpoint) < .00001) then
      printks "Cubic startup midpoint differs from its four reference knots\n", 0
      exitnowk -1
    endif
  endif
  if kSample % 4 == 0 then
    kIndex = int(kSample/4)
    kExpected = iKnots[kIndex]
    ; Cubic interpolation starts at the second knot and uses the first as history.
    kExpectedCubic = iKnots[kIndex+1]
    if !(abs(kHold-kExpected) < .00001 && abs(kLinear-kExpected) < .00001 && \
         abs(kAudioHold-kExpected) < .00001 && abs(kAudioLinear-kExpected) < .00001 && \
         abs(kCubic-kExpectedCubic) < .00001 && abs(kAudioCubic-kExpectedCubic) < .00001) then
      printks "seed=%g knot=%g: expected hold/linear=%g cubic=%g, got %g/%g/%g and %g/%g/%g\n", \
        0, p4, kIndex, kExpected, kExpectedCubic, kHold, kLinear, kCubic, kAudioHold, kAudioLinear, kAudioCubic
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin

instr CheckResults
  if i(gkChecks) != 18 then
    prints "31-bit knot checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckKnots" 0 .75 .5
i "CheckKnots" 0 .75 0
i "CheckKnots" 0 .75 1
i "CheckResults" .75 .03125
e
</CsScore>
</CsoundSynthesizer>
