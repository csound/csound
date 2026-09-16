<CsTest>
description = "betarand stays in range and preserves small-shape distribution means"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  seed 1
  iN = 0
  iSum = 0
draw:
  iValue betarand 1, p4, p5
  if !(iValue >= 0 && iValue <= 1) then
    prints "betarand alpha=%g beta=%g returned %g\n", p4, p5, iValue
    exitnow -1
  endif
  iSum += iValue
  iN += 1
  if iN < 4000 igoto draw
  iExpected = p4/(p4+p5)
  if abs(iSum/iN-iExpected) > .04 then
    prints "betarand alpha=%g beta=%g mean %g, expected %g\n", p4, p5, iSum/iN, iExpected
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001 .001 .001
i1 .01 .001 .001 .003
i1 .02 .001 .003 .001
i1 .03 .001 1e-300 1e-300
i1 .04 .001 1e-300 3e-300
i1 .05 .001 1 1
i1 .06 .001 1 2
</CsScore>
</CsoundSynthesizer>
