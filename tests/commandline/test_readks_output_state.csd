<CsTest>
description = "readks grows existing strings and restarts its line buffer on reinit"

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
gSLine init "x"
strset 9191, "readks_lines.dat"

instr 1
  iCount = 0
  while iCount < 9 do
    gSLine strcat gSLine, gSLine
    iCount += 1
  od
  gSLine strcat gSLine, "\n"
  fprints "readks_lines.dat", "%s", gSLine
endin

instr 2
  SText init "short"
  SNumeric init "short"
  kCycle timeinstk
  if kCycle == 4 then
    reinit read
  endif
read:
  SText readks "readks_lines.dat", .01
  SNumeric readks 9191, .01
  rireturn
  if kCycle >= 1 then
    SCopy = SText
  endif
  if strcmpk(SText, gSLine) != 0 || strcmpk(SNumeric, gSLine) != 0 || strcmpk(SCopy, gSLine) != 0 then
    printks "readks did not read or hold the full line\n", 0
    exitnowk -1
  endif
  if kCycle == 7 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 2 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .01
i2 .03 .01
i99 .05 .001
</CsScore>
</CsoundSynthesizer>
