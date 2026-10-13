<CsTest>
description = "ntrpol preserves interpolation, reversed bounds, and extrapolation at every rate"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

instr Check
  ; p4 = point, p5 = first bound, p6 = second bound, p7 = expected value.
  iValue ntrpol 2, 6, p4, p5, p6
  if abs(iValue - p7) > 0.000001 then
    prints "i-rate: point=%g, bounds=[%g, %g], expected=%g, got=%g\n", \
           p4, p5, p6, p7, iValue
    exitnow -1
  endif

  ; Change the inputs on the second cycle to check performance-time updates.
  kShift = timeinstk() - 1
  kExpected = p7 + kShift
  kValue ntrpol 2 + kShift, 6 + kShift, p4, p5, p6
  aFirst = 2 + kShift
  aSecond = 6 + kShift
  aOriginalFirst = aFirst
  aValue ntrpol aFirst, aSecond, p4, p5, p6
  ; Reusing either input as output must give the same answer.
  aFirst ntrpol aFirst, aSecond, p4, p5, p6
  aSecond ntrpol aOriginalFirst, aSecond, p4, p5, p6
  kSample = 0
  while kSample < ksmps do
    kAudio vaget kSample, aValue
    kAliasFirst vaget kSample, aFirst
    kAliasSecond vaget kSample, aSecond
    if abs(kValue - kExpected) > 0.000001 || \
       abs(kAudio - kExpected) > 0.000001 || \
       abs(kAliasFirst - kExpected) > 0.000001 || \
       abs(kAliasSecond - kExpected) > 0.000001 then
      printks "expected=%g, k-rate=%g, audio=%g, first alias=%g, second alias=%g\n", \
              0, kExpected, kValue, kAudio, kAliasFirst, kAliasSecond
      exitnowk -1
    endif
    kSample += 1
  od
endin
</CsInstruments>
<CsScore>
; Endpoints and midpoint of the usual interval.
i "Check" 0 0.5 0   0 1 2
i "Check" 0 0.5 0.5 0 1 4
i "Check" 0 0.5 1   0 1 6
; A custom interval and the same interval in reverse.
i "Check" 0 0.5 15 10 20 4
i "Check" 0 0.5 20 20 10 2
i "Check" 0 0.5 15 20 10 4
i "Check" 0 0.5 10 20 10 6
; Points outside the interval still extrapolate.
i "Check" 0 0.5 -0.5 0 1 0
i "Check" 0 0.5  1.5 0 1 8
e
</CsScore>
</CsoundSynthesizer>
