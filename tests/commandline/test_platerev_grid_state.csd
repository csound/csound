<CsTest>
description = "platerev keeps independent input, output, and boundary state"

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
0dbfs = 1

instr 1
  aImpulse = mpulse(1, 0)
  aZero = 0

  ; Input and output counts are independent.
  aOne platerev 1, 2, 1, 1, 1, 1, 0.001, aImpulse, aZero
  aLeft, aRight platerev 3, 4, 1, 1, 1, 1, 0.001, aImpulse

  ; The variadic input list can exceed the output limit of 40.
  aMany platerev 5, 2, 1, 1, 1, 1, 0.001, \
    aZero, aZero, aZero, aZero, aZero, aZero, aZero, aZero, \
    aZero, aZero, aZero, aZero, aZero, aZero, aZero, aZero, \
    aZero, aZero, aZero, aZero, aZero, aZero, aZero, aZero, \
    aZero, aZero, aZero, aZero, aZero, aZero, aZero, aZero, \
    aZero, aZero, aZero, aZero, aZero, aZero, aZero, aZero, aZero

  ; A stationary source at the edge exposes the horizontal boundary state.
  aFree platerev 6, 7, 0, 1, 1, 1, 0.001, aImpulse
  aClamp platerev 6, 7, 1, 1, 1, 1, 0.001, aImpulse

  kSymmetry = abs(downsamp(aLeft - aRight))
  kBoundary = abs(downsamp(aFree - aClamp))
  kMaxSymmetry init 0
  kMaxBoundary init 0
  kChecked init 0
  kMaxSymmetry = max(kMaxSymmetry, kSymmetry)
  kMaxBoundary = max(kMaxBoundary, kBoundary)

  if kChecked == 0 && timeinsts() >= 0.019 then
    if !(kMaxSymmetry < 0.000000001) then
      printks "platerev output paths lost symmetry: %.12f\n", 0, kMaxSymmetry
      exitnowk(-1)
    endif
    if !(kMaxBoundary > 0.01) then
      printks "platerev clamped boundary was not applied across rows: %.12f\n", 0, kMaxBoundary
      exitnowk(-1)
    endif
    kChecked = 1
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 -6 -2 0 0 0 0 0 0
f 2 0 -3 -2 0 0.2 0
f 3 0 -3 -2 0 0 0
; Opposite signed radii at phase zero avoid rounding pi in float builds.
f 4 0 -6 -2 0 0.2 0 0 -0.2 0
f 5 0 128 -2 0
f 6 0 -3 -2 0 0.99 0
f 7 0 -3 -2 0 0.99 0
i 1 0 0.02
</CsScore>
</CsoundSynthesizer>
