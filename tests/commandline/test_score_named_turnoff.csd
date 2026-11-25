<CsTest>
description = "a score's quoted negative name and its d statement turn a held named note off"
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
; performed. A turnoff runs no init: a negative name that starts a note, or a
; d that ties onto one, adds a pass.
giInits init 0
gkLast[] init 6

instr 1
  giInits += 1
  gkLast[p4] = times:k()
endin

instr Hold
  giInits += 1
  gkLast[p4] = times:k()
endin

; At 0.3 the notes turned off at 0.1 have stopped, and the others still play.
instr Check
  iStopped[] fillarray 0, 1, 1, 0, 1, 1
  iFailed = 0
  if giInits != 5 then
    prints("%d init passes, expected 5\n", giInits)
    iFailed = 1
  endif
  iSlot = 1
  while iSlot < 6 do
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
i "Hold" 0 -1 1
i "Hold.1" 0 -1 2
i "Hold.2" 0 -1 3
i "Hold.3" 0 -1 4
i 1.5 0 -1 5
i "-Hold" 0.1 0
i "-Hold.1" 0.1 0
d "Hold.3" 0.1 0
i -1.5 0.1 0
i "Check" 0.3 0
e
</CsScore>
</CsoundSynthesizer>
