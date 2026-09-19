<CsTest>
description = "String assignment takes an init-time copy while strcpyk follows changes"
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
  Ssource init "initial"
  Ssnapshot = Ssource
  ; This init-time construction must not be cleared during performance.
  Sbuilt = ""
  Sbuilt strcat Sbuilt, "built at init"

  kCycle init 0
  kCycle += 1
  if kCycle == 2 then
    Ssource strcpyk ""
  else
    Ssource sprintfk "changed on cycle %d", kCycle
  endif
  Slive strcpyk Ssource

  if strcmpk(Ssnapshot, "initial") != 0 || strcmpk(Sbuilt, "built at init") != 0 then
    printks "string assignment ran during performance\n", 0
    exitnowk -1
  endif
  if strcmpk(Slive, Ssource) != 0 then
    printks "strcpyk did not copy the current string\n", 0
    exitnowk -1
  endif
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 3 then
    prints "string assignment checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Check global and local control blocks, including delayed starts.
i 1 0 .25 32
i 1 .25 .25 4
i 1 .5 .25 1
i 99 .75 .01
e
</CsScore>
</CsoundSynthesizer>
