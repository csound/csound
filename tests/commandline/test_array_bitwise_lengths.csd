<CsTest>
description = "array bitwise operators follow changing input lengths"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr 1
  kLeft[] fillarray 3, 6
  kRight[] fillarray 5, 3
  kOr[] = kLeft | kRight
  kAnd[] = kLeft & kRight
  kLength lenarray kLeft
  kOrLength lenarray kOr
  kAndLength lenarray kAnd
  if kOrLength != kLength || kAndLength != kLength then
    printks "Lengths: input %g, or %g, and %g\n", 0, kLength, kOrLength, kAndLength
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    if kOr[kIndex] != 7 || kAnd[kIndex] != kIndex + 1 then
      printks "Index %g: or %g, and %g\n", 0, kIndex, kOr[kIndex], kAnd[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  ; Shrink, regrow within capacity, then empty both operands.
  kCycle timeinstk
  kNext = (kCycle == 1 ? 1 : (kCycle == 2 ? 2 : 0))
  trim kLeft, kNext
  trim kRight, kNext
endin
</CsInstruments>
<CsScore>
i 1 0 .0625
e
</CsScore>
</CsoundSynthesizer>
