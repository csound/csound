<CsTest>
description = "distort interpolates large finite table entries in configured precision"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
giEven ftgen 1, 0, 8, -2, -2e38, 2e38, -2e38, 2e38, -2e38, 2e38, -2e38, 2e38
giOdd ftgen 2, 0, -9, -2, -2e38, 2e38, -2e38, 2e38, -2e38, 2e38, -2e38, 2e38, -2e38
gkChecks init 0
instr Check
  iScale = 1000/32768
  aInput init iScale*p5/(ftlen(p4)/2)
  aOutput distort aInput, 1, p4, 0
  kOutput downsamp aOutput
  if !(abs(kOutput/iScale/1e38-p6) < .00001) then
    printks "Large table interpolation: displacement %g, expected %g, got %g (scaled)\n", 0, p5, p6, kOutput/iScale/1e38
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin
instr Done
  if i(gkChecks) != 20 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Check" 0 [1/8000] 1 -1.75 -1
i "Check" 0 [1/8000] 1 -1.5 0
i "Check" 0 [1/8000] 1 -1.25 1
i "Check" 0 [1/8000] 1 -0.75 1
i "Check" 0 [1/8000] 1 -0.5 0
i "Check" 0 [1/8000] 1 -0.25 -1
i "Check" 0 [1/8000] 1 0 -2
i "Check" 0 [1/8000] 1 0.25 -1
i "Check" 0 [1/8000] 1 0.5 0
i "Check" 0 [1/8000] 1 0.75 1
i "Check" 0 [1/8000] 1 1.25 1
i "Check" 0 [1/8000] 1 1.5 0
i "Check" 0 [1/8000] 1 1.75 -1
i "Check" 0 [1/8000] 2 -0.75 -1
i "Check" 0 [1/8000] 2 -0.5 -2
i "Check" 0 [1/8000] 2 -0.25 -1
i "Check" 0 [1/8000] 2 0 0
i "Check" 0 [1/8000] 2 0.25 1
i "Check" 0 [1/8000] 2 0.5 2
i "Check" 0 [1/8000] 2 0.75 1
i "Done" [2/8000] [1/8000]
e
</CsScore>
</CsoundSynthesizer>
