<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "scalearray ranges, constant input and current array length",
  "expect": {
    "exit": 0
  }
}
*/
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iValues[] fillarray 10,20,30,40
  kValues[] fillarray 10,20,30,40
  kLegacy[] fillarray 10,20,30,40
  iExpected[] fillarray p9,p10,p11,p12
  if p8 == 1 then
    iValues fillarray 5,5,5,5
    kValues fillarray 5,5,5,5
    kLegacy fillarray 5,5,5,5
  endif
  scalearray iValues, p6,p7,p4,p5
  iIndex = 0
  while iIndex < 4 do
    if !(abs(iValues[iIndex]-iExpected[iIndex]) < .00001) then
      prints "scalearray init mismatch at index %g: %g expected %g\n", iIndex,iValues[iIndex],iExpected[iIndex]
      exitnow(-1)
    endif
    iIndex += 1
  od
  scalearray kValues, p6,p7,p4,p5
  scalet kLegacy, p6,p7,p4,p5
  kIndex = 0
  while kIndex < 4 do
    kExpected = iExpected[kIndex]
    if !(abs(kValues[kIndex]-kExpected) < .00001) || !(abs(kLegacy[kIndex]-kExpected) < .00001) then
      printks "scalearray control mismatch at index %g: %g legacy %g expected %g\n", 0,kIndex,kValues[kIndex],kLegacy[kIndex],kExpected
      exitnowk(-1)
    endif
    kIndex += 1
  od
  gkChecks += 1
endin

instr 2
  ; Follow the current array length, including an empty array after trim.
  kValues[] fillarray 10,20,30,40
  kCycle init 0
  if kCycle == 1 then
    trim kValues, 2
    kValues[0] = 20
    kValues[1] = 40
  elseif kCycle == 2 then
    trim kValues, 0
  elseif kCycle == 3 then
    trim kValues, 3
    kValues[0] = 5
    kValues[1] = 5
    kValues[2] = 5
  endif
  scalearray kValues, 2,4
  kIndex = 0
  kSize lenarray kValues
  while kIndex < kSize do
    kExpected = (kCycle == 3 ? 2 : 2+2*kIndex/(kSize-1))
    if !(abs(kValues[kIndex]-kExpected) < .00001) then
      printks "scalearray resize mismatch on cycle %g\n", 0,kCycle
      exitnowk(-1)
    endif
    kIndex += 1
  od
  gkChecks += 1
  kCycle += 1
endin

instr 99
  if i(gkChecks) != 16 then
    prints "scalearray checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0.0 .001953125 1 3 0 1 0 10 0 1 40
i 1 0.03125 .001953125 3 1 0 1 0 10 0 1 40
i 1 0.0625 .001953125 -10 99 0 1 0 0 0.3333333333333333 0.6666666666666666 1
i 1 0.09375 .001953125 4 4 0 1 0 10 20 30 40
i 1 0.125 .001953125 99 1 0 1 0 10 0 0.5 1
i 1 0.15625 .001953125 1 1 0 1 0 10 20 30 40
i 1 0.1875 .001953125 0 -1 0 1 0 0 0.3333333333333333 0.6666666666666666 1
i 1 0.21875 .001953125 1 2 -2 2 0 10 -2 30 40
i 1 0.25 .001953125 0 -1 0 0 1 0 0 0 0
i 1 0.28125 .001953125 0 -1 -2 2 1 -2 -2 -2 -2
i 1 0.3125 .001953125 0 -1 1 -1 0 1 0.3333333333333333 -0.3333333333333333 -1
i 1 0.34375 .001953125 1 3 7 9 1 5 7 7 5
i 2 .5 .0078125
i 99 .6 .01
e
</CsScore>
</CsoundSynthesizer>
