<CsTest>
description = "readk holds complete values at EOF and reads text without a final newline"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
gkChecks init 0

instr 1
  fprints "readk_7.dat", "# comment\n; comment\n<comment>\n-1 -2 -3 -4 -5 -6 -7"
  fprints "readk_8.dat", "# comment\n-1.25 -2.25 -3.25 -4.25 -5.25 -6.25 -7.25"
  fprints "readk_empty.dat", "%s", ""
  fprints "readk_invalid.dat", "11 12 13 14\n21 bad\n31 32 33 34"
endin

instr 2
  SFile sprintf "readk_%d.dat", p4
  kCycle timeinstk
  dumpk -kCycle, SFile, p4, 0
  if kCycle == 7 then
    turnoff
  endif
endin

instr 3
  SFile sprintf "readk_%d.dat", p4
  if p5 != 0 then
    SFile = "readk_empty.dat"
  endif
  iPeriod = p6 / kr
  k1 readk SFile, p4, iPeriod
  k21, k22 readk2 SFile, p4, iPeriod
  k31, k32, k33 readk3 SFile, p4, iPeriod
  k41, k42, k43, k44 readk4 SFile, p4, iPeriod
  kCycle timeinstk
  kRead = int((kCycle - 1) / p6) + 1
  kEnd1 = min(kRead, 7)
  kEnd2 = min(kRead, 3) * 2
  kEnd3 = min(kRead, 2) * 3
  kEnd4 = 4
  iFraction = (p4 == 8 ? 0.25 : 0)
  if p5 != 0 then
    if k1 != 0 || k21 != 0 || k22 != 0 || k31 != 0 || k32 != 0 || k33 != 0 || k41 != 0 || k42 != 0 || k43 != 0 || k44 != 0 then
      printks "readk returned values from an empty file\n", 0
      exitnowk -1
    endif
  elseif k1 != -kEnd1-iFraction || k21 != 1-kEnd2-iFraction || k22 != -kEnd2-iFraction || k31 != 2-kEnd3-iFraction || k32 != 1-kEnd3-iFraction || k33 != -kEnd3-iFraction || k41 != 3-kEnd4-iFraction || k42 != 2-kEnd4-iFraction || k43 != 1-kEnd4-iFraction || k44 != -kEnd4-iFraction then
    printks "readk format %g, period %g, cycle %g: %g; %g %g; %g %g %g; %g %g %g %g\n", 0, p4, p6, kCycle, k1, k21, k22, k31, k32, k33, k41, k42, k43, k44
    exitnowk -1
  endif
  if kCycle == 25 then
    gkChecks += 1
    turnoff
  endif
endin

instr 4
  k1, k2, k3, k4 readk4 "readk_invalid.dat", 8, 0
  kCycle timeinstk
  kFirst = (kCycle <= 2 ? 11 : 31)
  if k1 != kFirst || k2 != kFirst+1 || k3 != kFirst+2 || k4 != kFirst+3 then
    printks "readk4 did not retain the last complete set after an invalid value\n", 0
    exitnowk -1
  endif
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 14 then
    prints "readk checks did not finish: %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 0 .01 1
i2 0 .01 4
i2 0 .01 5
i2 0 .01 6
i3 .02 .03 1 0 1
i3 .02 .03 4 0 1
i3 .02 .03 5 0 1
i3 .02 .03 6 0 1
i3 .02 .03 7 0 1
i3 .02 .03 8 0 1
i3 .06 .03 1 0 3
i3 .06 .03 4 0 3
i3 .06 .03 5 0 3
i3 .06 .03 6 0 3
i3 .06 .03 7 0 3
i3 .06 .03 8 0 3
i3 .1 .03 8 1 1
i4 .1 .01
i99 .15 .001
</CsScore>
</CsoundSynthesizer>
