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
giConfig ftgen 0, 0, 2, -2, 0, -1
giPos1 ftgen 0, 0, 2, -2, 0, 1
giPos2 ftgen 0, 0, 4, -2, 0, 1, 2, 3
giPos3 ftgen 0, 0, 8, -2, 0, 1, 2, 3, 4, 5, 6, 7
giSnapshots ftgen 0, 0, 16, -2, 0, 100, 10, 110, 20, 120, 30, 130, 40, 140, 50, 150, 60, 160, 70, 170

instr 1
 iOut1 ftgen 0, 0, 2, -2, -10, 77
 iConf1 ftgen 0, 0, 2, -2, -10, 77
 iOut2 ftgen 0, 0, 2, -2, -10, 77
 iConf2 ftgen 0, 0, 2, -2, -10, 77
 iOut3 ftgen 0, 0, 2, -2, -10, 77
 iConf3 ftgen 0, 0, 2, -2, -10, 77
 hvs1 p4, 2, 2, iOut1, giPos1, giSnapshots
 hvs1 p4, 2, 2, iConf1, giPos1, giSnapshots, giConfig
 hvs2 p4, p5, 2, 2, 2, iOut2, giPos2, giSnapshots
 hvs2 p4, p5, 2, 2, 2, iConf2, giPos2, giSnapshots, giConfig
 hvs3 p4, p5, p6, 2, 2, 2, 2, iOut3, giPos3, giSnapshots
 hvs3 p4, p5, p6, 2, 2, 2, 2, iConf3, giPos3, giSnapshots, giConfig
 kExpected[] fillarray 10*p4, 10*p4+20*p5, 10*p4+20*p5+40*p6
 kFirst1 table 0, iOut1
 kSecond1 table 1, iOut1
 kConfigFirst1 table 0, iConf1
 kConfigSecond1 table 1, iConf1
 kError1 = abs(kFirst1-kExpected[0])+abs(kSecond1-100-kExpected[0])
 kError1 += abs(kConfigFirst1-kExpected[0])+abs(kConfigSecond1-77)
 if !(kError1 < .00001) then
  printks "hvs1 at (%g,%g,%g) error=%g\n", 0, p4, p5, p6, kError1
  exitnowk(-1)
 endif
 kFirst2 table 0, iOut2
 kSecond2 table 1, iOut2
 kConfigFirst2 table 0, iConf2
 kConfigSecond2 table 1, iConf2
 kError2 = abs(kFirst2-kExpected[1])+abs(kSecond2-100-kExpected[1])
 kError2 += abs(kConfigFirst2-kExpected[1])+abs(kConfigSecond2-77)
 if !(kError2 < .00001) then
  printks "hvs2 at (%g,%g,%g) error=%g\n", 0, p4, p5, p6, kError2
  exitnowk(-1)
 endif
 kFirst3 table 0, iOut3
 kSecond3 table 1, iOut3
 kConfigFirst3 table 0, iConf3
 kConfigSecond3 table 1, iConf3
 kError3 = abs(kFirst3-kExpected[2])+abs(kSecond3-100-kExpected[2])
 kError3 += abs(kConfigFirst3-kExpected[2])+abs(kConfigSecond3-77)
 if !(kError3 < .00001) then
  printks "hvs3 at (%g,%g,%g) error=%g\n", 0, p4, p5, p6, kError3
  exitnowk(-1)
 endif
 kConfig0 table 0, giConfig
 kConfig1 table 1, giConfig
 if kConfig0 != 0 || kConfig1 != -1 then
  printks "HVS changed the configuration table\n", 0
  exitnowk(-1)
 endif
 kFirst init 1
 if kFirst == 1 then
  gkChecks += 1
  kFirst = 0
 endif
endin

instr 99
 if i(gkChecks) != 10 then
  prints "HVS checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0.0 .0078125 0 0 0
i 1 0.015625 .0078125 0 0 1
i 1 0.03125 .0078125 0 1 0
i 1 0.046875 .0078125 0 1 1
i 1 0.0625 .0078125 1 0 0
i 1 0.078125 .0078125 1 0 1
i 1 0.09375 .0078125 1 1 0
i 1 0.109375 .0078125 1 1 1
i 1 0.125 .0078125 0.25 0.5 0.75
i 1 0.140625 .0078125 0.75 0.25 0.5
i 99 .2 .001
e
</CsScore>
</CsoundSynthesizer>
