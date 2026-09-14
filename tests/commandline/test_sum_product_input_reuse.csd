<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
gaInput init 0
gaSum init 0
gaProduct init 0
gaExpected init 0

instr 1
  aWave oscili .1, 321
  aFirst = .2 + aWave
  aSecond = .3 - aWave
  aThird = .4 + aWave
  aExpected product aFirst, aSecond, aThird
  aReuseFirst = aFirst
  aReuseSecond = aSecond
  aReuseThird = aThird
  aReuseFirst product aReuseFirst, aSecond, aThird
  aReuseSecond product aFirst, aReuseSecond, aThird
  aReuseThird product aFirst, aSecond, aReuseThird
  aRepeated = aSecond
  aRepeated product aFirst, aRepeated, aRepeated
  aOne = aFirst
  aOne product aOne
  aSum = aSecond
  aSum sum aFirst, aSum, aThird, aFirst, aSecond
  aError = abs(aExpected-aFirst*aSecond*aThird)
  aError += abs(aReuseFirst-aExpected) + abs(aReuseSecond-aExpected)
  aError += abs(aReuseThird-aExpected) + abs(aRepeated-aFirst*aSecond*aSecond)
  aError += abs(aOne-aFirst) + abs(aSum-(2*aFirst+2*aSecond+aThird))
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "sum/product input reuse failed at start %g: %g\n", 0, p2, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 2
  gaInput = 2
  gaSum = 1
  gaProduct = 2
  gaExpected = 1
endin

instr 3
  gaSum sum gaInput, gaInput
  gaProduct product gaInput, gaProduct
  gaExpected = 4
endin

instr 4
  aError = abs(gaSum-gaExpected) + abs(gaProduct-gaExpected)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "sum/product inactive output was not cleared: %g\n", 0, kError
    exitnowk(-1)
  endif
  gkChecks += 1
endin

instr 99
  if i(gkChecks) != 5 then
    prints "sum/product checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .006
i 1 .010625 .004
i 1 .020625 .001
i 2 .03 .004
i 3 .030625 .002
i 4 .03 .004
i 99 .04 .002
</CsScore>
</CsoundSynthesizer>
