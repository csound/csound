<CsTest>
description = "readks preserves LF and CRLF lines, grows strings and restarts on reinit"

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
  gSLine = "x"
  iCount = 0
  while iCount < 9 do
    gSLine strcat gSLine, gSLine
    iCount += 1
  od
  if p4 == 0 then
    gSLine strcat gSLine, "\n"
  else
    gSLine strcat gSLine, "\r\n"
  endif
  ; Format 1 writes raw bytes, avoiding Windows text-mode translation.
  kByte timeinstk
  kValue = (kByte <= 512 ? 120 : (p4 == 1 && kByte == 513 ? 13 : 10))
  dumpk kValue, "readks_lines.dat", 1, 0
  if kByte == 513+p4 then
    turnoff
  endif
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
  if i(gkChecks) != 4 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .515 0
i2 .52 .01
i2 .54 .01
i1 .6 .515 1
i2 1.12 .01
i2 1.14 .01
i99 1.16 .001
</CsScore>
</CsoundSynthesizer>
