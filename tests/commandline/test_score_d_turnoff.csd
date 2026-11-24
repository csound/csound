<CsTest>
description = "a score's d statement turns a held note off, whatever its p3"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

; Every note counts its init pass, and its slot (p4) the time it last
; performed. A turnoff runs no init: a d that ties onto a note, or starts one,
; adds a pass.
giInits init 0
gkLast[] init 4

instr 1
  giInits += 1
  gkLast[p4] = times:k()
endin

; At 0.3 the notes turned off at 0.1 have stopped, and the other still plays.
instr Check
  iStopped[] fillarray 0, 1, 1, 0
  iFailed = 0
  if giInits != 3 then
    prints("%d init passes, expected 3\n", giInits)
    iFailed = 1
  endif
  if active:i(1) != 1 then
    prints("%d notes of instr 1 play, expected 1\n", active:i(1))
    iFailed = 1
  endif
  iSlot = 1
  while iSlot < 4 do
    iStop = i(gkLast, iSlot) < 0.2 ? 1 : 0
    if iStop != iStopped[iSlot] then
      prints("slot %d: stopped %d, expected %d\n", iSlot, iStop, iStopped[iSlot])
      iFailed = 1
    endif
    iSlot += 1
  od
  if iFailed == 1 then
    exitnow(1)
  endif
endin
</CsInstruments>
<CsScore>
i 1.1 0 -1 1
i 1.2 0 -1 2
i 1.3 0 -1 3
d 1.1 0.1 0
; p3 is ignored
d 1.2 0.1 0.5
; nothing to turn off: no note
d 1.4 0.1 0
i "Check" 0.3 0
e
</CsScore>
</CsoundSynthesizer>
