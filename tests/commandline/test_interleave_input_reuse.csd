<CsTest>
description = "interleave preserves reused inputs and follows current lengths"

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
gkChecks init 0

opcode CheckArray, 0, i[]i[]
  iActual[], iExpected[] xin
  if lenarray(iActual) != lenarray(iExpected) then
    prints "interleave: wrong output length\n"
    exitnow -1
  endif
  iIndex = 0
  while iIndex < lenarray(iExpected) do
    if iActual[iIndex] != iExpected[iIndex] then
      prints "interleave: index %d expected %g, got %g\n", iIndex, iExpected[iIndex], iActual[iIndex]
      exitnow -1
    endif
    iIndex += 1
  od
endop

instr 1
  iExpected[] fillarray 1, 5, 2, 6, 3, 7, 4, 8
  iA[] fillarray 1, 2, 3, 4
  iB[] fillarray 5, 6, 7, 8
  iSeparate[] interleave iA, iB
  CheckArray iSeparate, iExpected
  iA interleave iA, iB
  CheckArray iA, iExpected
  iC[] fillarray 1, 2, 3, 4
  iB interleave iC, iB
  CheckArray iB, iExpected
  iBoth[] fillarray 1, 2, 3, 4
  iDoubled[] fillarray 1, 1, 2, 2, 3, 3, 4, 4
  iBoth interleave iBoth, iBoth
  CheckArray iBoth, iDoubled
  iEmpty[] init 0
  iEmpty interleave iEmpty, iEmpty
  CheckArray iEmpty, iEmpty

  ; The inverse operation must also preserve an input reused as an output.
  iA, iRight[] deinterleave iA
  CheckArray iA, iC
  iRightExpected[] fillarray 5, 6, 7, 8
  CheckArray iRight, iRightExpected

  kA[] init 4
  kB[] init 4
  kC[] init 4
  kD[] init 4
  kSame[] init 4
  kCycle timeinstk
  kLength = (kCycle == 2 ? 2 : (kCycle == 3 ? 0 : 4))
  ; Refill reused inputs before each control-cycle transform.
  trim kA, kLength
  trim kB, kLength
  trim kC, kLength
  trim kD, kLength
  trim kSame, kLength
  kIndex = 0
  while kIndex < kLength do
    kA[kIndex] = kIndex + 1
    kB[kIndex] = kIndex + 5
    kC[kIndex] = kIndex + 1
    kD[kIndex] = kIndex + 5
    kSame[kIndex] = kIndex + 1
    kIndex += 1
  od
  kA interleave kA, kB
  kD interleave kC, kD
  kSame interleave kSame, kSame
  kLeft[], kRight[] deinterleave kA
  kASize lenarray kA
  kDSize lenarray kD
  kSameSize lenarray kSame
  kLeftSize lenarray kLeft
  kRightSize lenarray kRight
  if kASize != 2*kLength || kDSize != 2*kLength || kSameSize != 2*kLength || kLeftSize != kLength || kRightSize != kLength then
    printks "interleave: cycle %d length %d outputs %d %d %d %d %d\n", 0, kCycle, kLength, kASize, kDSize, kSameSize, kLeftSize, kRightSize
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    if kA[2*kIndex] != kIndex+1 || kA[2*kIndex+1] != kIndex+5 || kD[2*kIndex] != kIndex+1 || kD[2*kIndex+1] != kIndex+5 || kSame[2*kIndex] != kIndex+1 || kSame[2*kIndex+1] != kIndex+1 || kLeft[kIndex] != kIndex+1 || kRight[kIndex] != kIndex+5 then
      printks "interleave: incorrect value in control cycle %d\n", 0, kCycle
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 1 then
    prints "interleave checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
