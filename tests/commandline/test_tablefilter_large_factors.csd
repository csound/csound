<CsTest>
description = "tablefilter includes large prime factors in denominator and threshold weights"

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
nchnls = 1
0dbfs = 1

; 10007 is the first prime beyond the opcode's stored prime table.
; Multiplying it by 2 also checks a denominator with a small and a large factor.
giSource ftgen 101, 0, -4, -2, .5, 1/10007, 1/20014, .25
giLight ftgen 201, 0, -4, -2, .5
giHeavy ftgen 202, 0, -4, -2, 1/10007, 1/20014, .25
giUpToPrime ftgen 203, 0, -4, -2, .5, 1/10007, .25

instr 1
 iMode = p4
 iThreshold = p5
 iExpected = p6
 iExpectedCount = p7
 iInitOutput ftgen 0, 0, -4, -2, 0
 iPerfOutput ftgen 0, 0, -4, -2, 0
 iCount tablefilteri iInitOutput, giSource, iMode, iThreshold
 kCount tablefilter iPerfOutput, giSource, iMode, iThreshold
 if iCount != iExpectedCount || kCount != iExpectedCount then
  printks "mode %g, threshold %g: init count=%g, perf count=%g, expected=%g\n", 0, iMode, iThreshold, iCount, kCount, iExpectedCount
  exitnowk(-1)
 endif
 kIndex = 0
 while kIndex < iExpectedCount do
  kExpected table kIndex, iExpected
  kInit table kIndex, iInitOutput
  kPerf table kIndex, iPerfOutput
  if kInit != kExpected || kPerf != kExpected then
   printks "mode %g, threshold %g, index %g: init=%g, perf=%g, expected=%g\n", 0, iMode, iThreshold, kIndex, kInit, kPerf, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
endin
</CsInstruments>
<CsScore>
; Mode 1 includes the threshold's weight; mode 2 includes it as well.
; Threshold 2 admits only 1/2 in mode 1, while threshold 4 rejects 1/2 in mode 2.
i 1 0.00 .004 1 2 201 1
i 1 0.01 .004 2 4 202 3
; A large prime in the threshold must also contribute its weight.
i 1 0.02 .004 1 10007 203 3
e
</CsScore>
</CsoundSynthesizer>
