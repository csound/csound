<CsTest>
description = "betarand handles large and mixed shapes without losing the distribution"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
instr 1
  seed 12345
  iSum = 0
  iSquares = 0
  iCount = 0
  while iCount < 4096 do
    iValue betarand 1,p4,p5
    if !(iValue >= 0 && iValue <= 1) then
      exitnow(-1)
    endif
    iSum += iValue
    iSquares += iValue*iValue
    iCount += 1
  od
  iMean = p4/(p4+p5)
  iVariance = iMean*(1-iMean)/(p4+p5+1)
  if abs(iSum/iCount-iMean) > .01 || abs(iSquares/iCount-(iSum/iCount)^2-iVariance) > .25*iVariance then
    prints "betarand distribution mismatch for %g, %g\n",p4,p5
    exitnow(-1)
  endif
  iTable ftgen 0,0,-16,-21,9,1,p4,p5
  iIndex = 0
  while iIndex < 16 do
    iValue table iIndex,iTable
    if !(iValue >= 0 && iValue <= 1) then
      exitnow(-1)
    endif
    iIndex += 1
  od
  kValue betarand 1,p4,p5
  aValue betarand 1,p4,p5
  kAudio downsamp aValue
  if !(kValue >= 0 && kValue <= 1 && kAudio >= 0 && kAudio <= 1) then
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .004 40 80
i 1 .01 .004 .5 20
e
</CsScore>
</CsoundSynthesizer>
