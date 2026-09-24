<CsTest>
description = "wrap gives the same result at whole-period offsets"

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

instr CheckBoundary
  iInput = p4
  iLower = p5
  iUpper = p6
  iExpected = p7
  iWrapped wrap iInput, iLower, iUpper
  kInput = iInput
  aInput = iInput
  kWrapped wrap kInput, iLower, iUpper
  aWrapped wrap aInput, iLower, iUpper
  kAudioWrapped downsamp aWrapped
  if iWrapped != iExpected || kWrapped != iExpected || kAudioWrapped != iExpected then
    printks "wrap input=%g bounds=[%g,%g) expected=%g init=%g control=%g audio=%g\n", \
      0, iInput, iLower, iUpper, iExpected, iWrapped, kWrapped, kAudioWrapped
    gkFailures += 1
  endif
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 12 || i(gkFailures) != 0 then
    prints "wrap checked %g cases, with %g failures\n", i(gkChecks), i(gkFailures)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; All values differ from the lower bound by a whole number of periods.
;                                     input lower upper expected
i "CheckBoundary" 0 [1/8192]          -2     0     1    0
i "CheckBoundary" 0 [1/8192]          -1     0     1    0
i "CheckBoundary" 0 [1/8192]           0     0     1    0
i "CheckBoundary" 0 [1/8192]           1     0     1    0
i "CheckBoundary" 0 [1/8192]           2     0     1    0
; The same periodicity must hold when the lower bound is nonzero.
i "CheckBoundary" 0 [1/8192]          -1     2     5    2
; Fractional values on either side must keep their position in the cycle.
i "CheckBoundary" 0 [1/8192]         -.25    0     1    .75
i "CheckBoundary" 0 [1/8192]          .25    0     1    .25
i "CheckBoundary" 0 [1/8192]         1.25    0     1    .25
; Rounding a tiny negative offset must not emit the excluded upper bound.
i "CheckBoundary" 0 [1/8192]       -1e-20    0     1    0
i "CheckBoundary" 0 [1/8192]        1e-20    0     1    1e-20
i "CheckBoundary" 0 [1/8192]           5     2     5    2
i "CheckResults" [2/8192] [1/8192]
e
</CsScore>
</CsoundSynthesizer>
