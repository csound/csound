<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
giSource0 ftgen 101, 0, -8, -2, 0.0, 0.25, 0.5, 0.75, 1.0, 0.3333333333333333, 0.6666666666666666, -0.5
giSource1 ftgen 102, 0, -5, -2, 0.0, 0.25, 0.5, 0.75, 1.0
giExpected0 ftgen 110, 0, 8, -2, 0.0, 0.5, 1.0, -0.5
giExpected1 ftgen 111, 0, 8, -2, 0.25, 0.5, 0.75, 0.3333333333333333, 0.6666666666666666, -0.5
giExpected2 ftgen 112, 0, 8, -2, 0.0, 0.25, 0.5, 0.75, 1.0, -0.5
giExpected3 ftgen 113, 0, 8, -2, 0.25, 0.75, 0.3333333333333333, 0.6666666666666666
giExpected4 ftgen 114, 0, 8, -2, 0.0, 0.5, 1.0, 0.0, 0.5
giExpected5 ftgen 115, 0, 8, -2, 0.25, 0.5, 0.75, 0.25, 0.5
giExpected6 ftgen 116, 0, 8, -2, 0.0, 0.25, 0.5, 0.75, 1.0, 0.0, 0.25, 0.5
giExpected7 ftgen 117, 0, 8, -2, 0.25, 0.75, 0.25

instr 1
 iOutI ftgen 0, 0, 8, -2, -99
 iOutK ftgen 0, 0, 8, -2, -99
 iCount tablefilteri iOutI, p4, p6, p5
 if iCount != p8 then
  prints "tablefilteri count=%g expected=%g\n", iCount, p8
  exitnow(-1)
 endif
 kMode init p6
 kThreshold init p5
 kCount tablefilter iOutK, p4, kMode, kThreshold
 if kMode != p6 || kThreshold != p5 || kCount != p8 then
  printks "tablefilter mode=%g threshold=%g count=%g expected=%g\n", 0, kMode, kThreshold, kCount, p8
  exitnowk(-1)
 endif
 kIndex = 0
 while kIndex < kCount do
  kInit table kIndex, iOutI
  kPerf table kIndex, iOutK
  kExpected table kIndex, p7
  if !(abs(kInit-kExpected)+abs(kPerf-kExpected) < .00001) then
   printks "tablefilter index=%g init=%g perf=%g expected=%g\n", 0, kIndex, kInit, kPerf, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 kFirst init 1
 if kFirst == 1 then
  gkChecks += 1
  kFirst = 0
 endif
endin
instr 99
 if i(gkChecks) != 8 then
  prints "tablefilter checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0.0 .0078125 101 2 1 110 4
i 1 0.015625 .0078125 101 2 2 111 6
i 1 0.03125 .0078125 101 4 1 112 6
i 1 0.046875 .0078125 101 4 2 113 4
i 1 0.0625 .0078125 102 2 1 114 5
i 1 0.078125 .0078125 102 2 2 115 5
i 1 0.09375 .0078125 102 4 1 116 8
i 1 0.109375 .0078125 102 4 2 117 3
i 99 .15 .001
e
</CsScore>
</CsoundSynthesizer>
