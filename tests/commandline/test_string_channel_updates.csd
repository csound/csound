<CsTest>
description = "String channel reads publish changed and empty text to assignments"

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
gkChecks init 0

instr 1
  setksmps p4
  Sname sprintf "text-%d", p4
  kCycle init 0
  kCycle += 1
  ; Repeat each value, grow beyond the initial buffer, and then clear it.
  if kCycle == 3 || kCycle == 4 then
    Ssent strcpyk ""
  else
    Ssent sprintfk "%0500d", int((kCycle - 1) / 2)
  endif
  chnset Ssent, Sname
  Sreceived chnget Sname
  Scopy = Sreceived
  SreceivedK chngetks Sname
  ScopyK = SreceivedK
  if strcmpk(Sreceived, Ssent) != 0 || strcmpk(Scopy, Ssent) != 0 || strcmpk(SreceivedK, Ssent) != 0 || strcmpk(ScopyK, Ssent) != 0 then
    printks "stale string-channel read with ksmps=%d on cycle %d\n", 0, p4, kCycle
    exitnowk(-1)
  endif
  if kCycle == 6 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  chnset "first", "array-first"
  chnset "second", "array-second"
  Snames[] fillarray "array-first", "array-second"
  Svalues[] chngets Snames
  if strcmp(Svalues[0], "first") != 0 || strcmp(Svalues[1], "second") != 0 then
    prints "string-channel array init changed\n"
    exitnow(-1)
  endif
  kCycle init 0
  kCycle += 1
  Ssent sprintfk "%d", kCycle
  chnset Ssent, "array-first"
  chnset "", "array-second"
  Supdated[] chngets Snames
  Sfirst = Supdated[0]
  Ssecond = Supdated[1]
  if strcmpk(Sfirst, Ssent) != 0 || strcmpk(Ssecond, "") != 0 then
    printks "string-channel array update failed\n", 0
    exitnowk(-1)
  endif
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 4 then
    prints "string-channel checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .5 1
i 1 .0625 .5 4
i 1 .125 .5 32
i 2 0 .5
i 99 .75 .01
</CsScore>
</CsoundSynthesizer>
