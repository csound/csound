<CsTest>
description = "String copies remain visible in each local control cycle"

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
  kCycle init 0
  kCycle += 1
  Ssource sprintfk "%d", kCycle
  Sfirst = Ssource
  Ssecond = Sfirst
  Sthird = Ssecond
  Sexplicit strcpyk Ssource
  Scopy = Sexplicit
  Svalues[] init 1
  Svalues[0] = Ssource
  Sread = Svalues[0]
  SarrayCopy = Sread
  if strcmpk(Sfirst, Ssource) != 0 || strcmpk(Ssecond, Ssource) != 0 || strcmpk(Sthird, Ssource) != 0 || strcmpk(Scopy, Ssource) != 0 || strcmpk(SarrayCopy, Ssource) != 0 then
    printks "stale string copy with ksmps=%d at local cycle %d\n", 0, p4, kCycle
    exitnowk(-1)
  endif
  if kCycle == 12 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 3 then
    prints "local string copy checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 .0625 .5 1
i 1 .125 .5 4
i 1 .1875 .5 32
i 99 .75 .01
</CsScore>
</CsoundSynthesizer>
