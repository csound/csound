<CsTest>
description = "Poisson samples have the expected mean and variance"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
seed 314159
instr 1
  ; At this mean, single precision cannot resolve the standard deviation.
  if p4 > 1e12 && 1 + 1e-8 == 1 then
    turnoff
    igoto done
  endif
  iSum = 0
  iSquares = 0
  iCount = 0
  while iCount < 10000 do
    iSample poisson p4
    iDelta = iSample - p4
    iSum += iDelta
    iSquares += iDelta * iDelta
    iCount += 1
  od
  if abs(iSum / iCount) > .1 * sqrt(p4) || abs(iSquares / iCount / p4 - 1) > .1 then
    exitnow -1
  endif
done:
endin
</CsInstruments>
<CsScore>
i 1 0 .01 4
i 1 .01 .01 64
i 1 .02 .01 1000
i 1 .03 .01 1000000
i 1 .04 .01 1e16
e
</CsScore>
</CsoundSynthesizer>
