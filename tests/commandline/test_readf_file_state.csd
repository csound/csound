<CsTest>
description = "readf and readfi retain EOF, publish string updates and restart cleanly"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
strset 9177, "readf_lines.txt"
giChecks init 0
gkChecks init 0

instr 1
  setksmps p4
  SText, kLine readf "readf_lines.txt"
  SNumeric, kNumeric readf 9177
  SCopy strcpyk SText
  SNumericCopy strcpyk SNumeric
  kCycle timeinstk
  kExpected = (kCycle <= 2 ? kCycle : -1)
  if kLine != kExpected || kNumeric != kExpected then
    printks "readf cycle %g: expected %g, got %g and %g\n", 0, kCycle, kExpected, kLine, kNumeric
    exitnowk -1
  endif
  if kCycle == 1 then
    kText strcmpk SCopy, "first\n"
    kNumericText strcmpk SNumericCopy, "first\n"
  elseif kCycle == 2 then
    kText strcmpk SCopy, "second\n"
    kNumericText strcmpk SNumericCopy, "second\n"
  else
    kText strcmpk SCopy, ""
    kNumericText strcmpk SNumericCopy, ""
  endif
  if kText != 0 || kNumericText != 0 then
    printks "readf did not update its copied text on cycle %g\n", 0, kCycle
    exitnowk -1
  endif
  if kCycle > 2 then
    kLength strlenk SText
    kNumericLength strlenk SNumeric
    if kLength != 0 || kNumericLength != 0 then
      printks "readf returned text after EOF\n", 0
      exitnowk -1
    endif
  endif
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  iAttempt = 0
read:
  SText, iLine readfi "readf_lines.txt"
  SNumeric, iNumeric readfi 9177
  iAttempt += 1
  iExpected = (iAttempt <= 2 ? iAttempt : -1)
  if iLine != iExpected || iNumeric != iExpected then
    prints "readfi call %g: expected %g, got %g and %g\n", iAttempt, iExpected, iLine, iNumeric
    exitnow -1
  endif
  if iAttempt > 2 then
    if strlen(SText) != 0 || strlen(SNumeric) != 0 then
      prints "readfi returned text after EOF\n"
      exitnow -1
    endif
  elseif iAttempt == 1 then
    if strcmp(SText, "first\n") != 0 || strcmp(SNumeric, "first\n") != 0 then
      prints "readfi did not restart at the first line\n"
      exitnow -1
    endif
  endif
  if iAttempt < p4 igoto read
  giChecks += 1
endin

instr 3
  kCycle timeinstk
  if kCycle == 3 then
    reinit read
  endif
read:
  SText, kLine readf "readf_lines.txt"
  rireturn
  kExpected = (kCycle <= 2 ? kCycle : (kCycle <= 4 ? kCycle-2 : -1))
  if kLine != kExpected then
    printks "readf reinit cycle %g: expected %g, got %g\n", 0, kCycle, kExpected, kLine
    exitnowk -1
  endif
  if kCycle == 6 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if giChecks != 4 || i(gkChecks) != 5 then
    prints "readf file state checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01 32
i1 .02 .01 32
i1 .04 .01 1
i1 .06 .01 4
; Finish at EOF, then stop early and reuse the same instrument instance.
i2 0 .01 5
i2 .02 .01 1
i2 .04 .01 1
i2 .06 .01 5
i3 0 .01
i99 .1 .01
</CsScore>
</CsoundSynthesizer>
