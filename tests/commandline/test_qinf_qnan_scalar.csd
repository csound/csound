<CsTest>
description = "qinf and qnan distinguish finite values, signed infinity, and NaN at init and control rates"
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
0dbfs = 1
gkChecks init 0

instr CheckScalars
  iInfinity = exp(1000)
  iNaN = sqrt(-1)
  ; Select a large finite value that fits the build's sample precision.
  iOne init 1
  iLarge = (iOne + 1e-10 == iOne ? 2e38 : 1.5e308)
  iValues[] fillarray 0, 0.25, -0.25, iInfinity, -iInfinity, iNaN, -iNaN, iLarge, -iLarge
  iInfExpected[] fillarray 0, 0, 0, 1, -1, 0, 0, 0, 0
  iNaNExpected[] fillarray 0, 0, 0, 0, 0, 1, 1, 0, 0

  iIndex = 0
  while iIndex < lenarray(iValues) do
    iInfCount = qinf(iValues[iIndex])
    iNaNCount = qnan(iValues[iIndex])
    if iInfCount != iInfExpected[iIndex] || iNaNCount != iNaNExpected[iIndex] then
      prints "init case %d: expected qinf=%g qnan=%g, got %g %g\n", \
        iIndex, iInfExpected[iIndex], iNaNExpected[iIndex], iInfCount, iNaNCount
      exitnow -1
    endif
    iIndex += 1
  od

  ; Reuse each control-rate opcode with a different value on every block.
  kIndex init 0
  kValue = iValues[kIndex]
  kInfCount = qinf(kValue)
  kNaNCount = qnan(kValue)
  if kInfCount != iInfExpected[kIndex] || kNaNCount != iNaNExpected[kIndex] then
    printks "control case %d: expected qinf=%g qnan=%g, got %g %g\n", \
      0, kIndex, iInfExpected[kIndex], iNaNExpected[kIndex], kInfCount, kNaNCount
    exitnowk -1
  endif
  gkChecks += 1
  kIndex += 1
  if kIndex == lenarray(iValues) then
    turnoff
  endif
endin

instr CheckResults
  if i(gkChecks) != 9 then
    prints "Scalar classification checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckScalars" 0 2.25
i "CheckResults" 2.25 .25
e
</CsScore>
</CsoundSynthesizer>
