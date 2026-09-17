<CsTest>
description = "getftargs tracks every trigger and publishes string updates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
giTable ftgen 1, 0, 8, -2, 3, 5, 7
gkChecks init 0

instr 1
  iTriggers[] fillarray 1, 0, 1, -1, 1, 2, 2, 0, 2
  kCycle init 0
  kPrevious init 0
  kTrig init 0
  Sargs init "untouched"
  kTrig = iTriggers[kCycle]
  ; A refresh restores the table arguments; no refresh leaves this marker.
  Sargs strcpyk "untouched"
  Sargs getftargs giTable, kTrig
  kRefresh = (kTrig > 0 && kTrig != kPrevious ? 1 : 0)
  if (kRefresh == 1 && strcmpk(Sargs, "3 5 7") != 0) || (kRefresh == 0 && strcmpk(Sargs, "untouched") != 0) then
    printks "getftargs missed a trigger transition on cycle %d: [%s]\n", 0, kCycle, Sargs
    exitnowk(-1)
  endif
  kPrevious = kTrig
  kCycle += 1
  if kCycle == lenarray(iTriggers) then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  ; Table zero has no saved GEN arguments, so it also exercises empty output.
  Sexpected = (p4 == 0 ? "" : "3 5 7")
  kCycle init 0
  kTrig init 0
  kCycle += 1
  kTrig = (kCycle % 2 == 0 ? kCycle : 0)
  Sargs getftargs p4, kTrig
  Scopy strcpyk "untouched"
  Scopy = Sargs
  if kTrig > 0 then
    if strcmpk(Scopy, Sexpected) != 0 then
      printks "getftargs update did not reach string assignment for table %d\n", 0, p4
      exitnowk(-1)
    endif
  elseif strcmpk(Scopy, "untouched") != 0 then
    printks "getftargs marked an unchanged string as updated\n", 0
    exitnowk(-1)
  endif
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  kTrig init p4
  kCycle init 0
  kCycle += 1
  Sargs init "untouched"
  Sargs getftargs giTable, kTrig
  Sexpected = (p4 > 0 ? "3 5 7" : "untouched")
  if strcmp(Sargs, Sexpected) != 0 then
    prints "getftargs init trigger changed\n"
    exitnow(-1)
  endif
  ; A constant positive trigger must not refresh at performance time.
  if kCycle > 1 && strcmpk(Sargs, "untouched") != 0 then
    printks "getftargs refreshed on a constant trigger\n", 0
    exitnowk(-1)
  endif
  Sargs strcpyk "untouched"
  if kCycle == 2 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 6 then
    prints "getftargs checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .5
i 2 0 .5 1
i 2 0 .5 0
i 3 0 .5 1
i 3 0 .5 0
i 3 0 .5 -1
i 99 .6 .01
</CsScore>
</CsoundSynthesizer>
