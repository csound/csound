<CsTest>
description = "wrap preserves offsets and finite results for large values"

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

instr CheckValue
  iScale = 2^p8
  ; The 2^1022 cases exercise double overflow; float builds skip them.
  if iScale == 2*iScale then
    turnoff
  endif
  iInput = p4*iScale
  iLower = p5*iScale
  iUpper = p6*iScale
  iExpected = p7
  iWrapped wrap iInput, iLower, iUpper
  kInput = iInput
  aInput = iInput
  kWrapped wrap kInput, iLower, iUpper
  aWrapped wrap aInput, iLower, iUpper
  kAudioWrapped downsamp aWrapped
  if !(iWrapped/iScale == iExpected && kWrapped/iScale == iExpected && kAudioWrapped/iScale == iExpected) then
    printks "wrap input=%g bounds=%g..%g scale exponent=%g expected=%g init=%g control=%g audio=%g\n", \
      0, p4, p5, p6, p8, iExpected, iWrapped/iScale, kWrapped/iScale, kAudioWrapped/iScale
    gkFailures += 1
  endif
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) < 7 || i(gkFailures) != 0 then
    prints "wrap checked %g numeric cases, with %g failures\n", i(gkChecks), i(gkFailures)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
;                                 input lower upper expected scale exponent
; 2^70 has remainder 1 modulo 3. Subtracting 1 first loses that offset.
i "CheckValue" 0 [1/8192] [2^70]   1     4     1       0
i "CheckValue" 0 [1/8192] [0 - 2^70]  1     4     2       0
; Finite inputs and bounds must not overflow while computing the wrap.
i "CheckValue" 0 [1/8192]  3.5    -3     3    -2.5   126
i "CheckValue" 0 [1/8192] -3.5    -3     3     2.5   126
i "CheckValue" 0 [1/8192]  3      -3    -2    -3     126
; Equal or reversed bounds retain the existing midpoint result.
i "CheckValue" 0 [1/8192]  0       3     3     3     126
i "CheckValue" 0 [1/8192]  0       3     2     2.5   126
; Repeat the overflow cases near the double-precision limit.
i "CheckValue" 0 [1/8192]  3.5    -3     3    -2.5  1022
i "CheckValue" 0 [1/8192] -3.5    -3     3     2.5  1022
i "CheckValue" 0 [1/8192]  3      -3    -2    -3    1022
i "CheckValue" 0 [1/8192]  0       3     3     3    1022
i "CheckValue" 0 [1/8192]  0       3     2     2.5  1022
i "CheckResults" [2/8192] [1/8192]
e
</CsScore>
</CsoundSynthesizer>
