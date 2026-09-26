<CsTest>
description = "Audio-array arithmetic respects caller, local and output element strides"

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
0dbfs = 32
gkChecks init 0

; Check both elements and every sample, including later local blocks.
opcode CheckPair, 0, a[]kkS
  aValues[], kFirst, kSecond, SOperation xin
  kIndex = 0
  while kIndex < ksmps do
    kActualFirst vaget kIndex, aValues[0]
    kActualSecond vaget kIndex, aValues[1]
    if !(abs(kActualFirst-kFirst) < .00001 && \
         abs(kActualSecond-kSecond) < .00001) then
      printks "%s, sample %g: expected [%g, %g], got [%g, %g]\n", \
        0, SOperation, kIndex, kFirst, kSecond, kActualFirst, kActualSecond
      exitnowk -1
    endif
    kIndex += 1
  od
endop

opcode CheckArithmetic, 0, a[]
  setksmps 2
  aCaller[] xin
  ; Caller elements store eight samples; local elements store two.
  aLocal[] init 2
  aLocal[0] = 1
  aLocal[1] = 2

  aSum[] = aLocal + aCaller
  CheckPair aSum, 11, 22, "aLocal + aCaller"

  aReverseSum[] = aCaller + aLocal
  CheckPair aReverseSum, 11, 22, "aCaller + aLocal"

  aDifference[] = aLocal - aCaller
  CheckPair aDifference, -9, -18, "aLocal - aCaller"

  aReverseDifference[] = aCaller - aLocal
  CheckPair aReverseDifference, 9, 18, "aCaller - aLocal"

  aProduct[] = aLocal * aCaller
  CheckPair aProduct, 10, 40, "aLocal * aCaller"

  aReverseProduct[] = aCaller * aLocal
  CheckPair aReverseProduct, 10, 40, "aCaller * aLocal"

  aQuotient[] = aLocal / aCaller
  CheckPair aQuotient, .1, .1, "aLocal / aCaller"

  aReverseQuotient[] = aCaller / aLocal
  CheckPair aReverseQuotient, 10, 10, "aCaller / aLocal"

  ; Compound assignments write into the shorter local storage.
  aLocal += aCaller
  CheckPair aLocal, 11, 22, "local += caller"
  aLocal -= aCaller
  CheckPair aLocal, 1, 2, "local -= caller"
  aLocal *= aCaller
  CheckPair aLocal, 10, 40, "local *= caller"
  aLocal /= aCaller
  CheckPair aLocal, 1, 2, "local /= caller"

  ; Also exercise a result with larger storage and a local right operand.
  aWide[] = aCaller + aCaller
  aWide += aLocal
  CheckPair aWide, 21, 42, "wide += local"
  aWide -= aLocal
  CheckPair aWide, 20, 40, "wide -= local"
  aWide *= aLocal
  CheckPair aWide, 20, 80, "wide *= local"
  aWide /= aLocal
  CheckPair aWide, 20, 40, "wide /= local"

  ; Reusing the left operand as the output must keep its local stride.
  aLocal = aLocal + aCaller
  CheckPair aLocal, 11, 22, "local = local + caller"
endop

instr RunChecks
  aCaller[] init 2
  aCaller[0] = 10
  aCaller[1] = 20
  CheckArithmetic aCaller
  gkChecks += 1
  turnoff
endin

instr CheckResults
  if i(gkChecks) != 1 then
    prints "Audio-array arithmetic checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "RunChecks" 0 .25
i "CheckResults" .5 .25
e
</CsScore>
</CsoundSynthesizer>
