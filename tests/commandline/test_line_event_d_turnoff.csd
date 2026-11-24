<CsTest>
description = "a line event's d turns off the held note its fractional p1 names"
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
; performed. A turnoff runs no init: a d that ties onto a note adds a pass.
giInits init 0
gkLast[] init 5

instr 1
  giInits += 1
  gkLast[p4] = times:k()
endin

instr Hold
  giInits += 1
  gkLast[p4] = times:k()
endin

instr Off
  scoreline_i({{
d 1.1 0 0
d "Hold.2" 0 0
}})
endin

; At 0.3 the notes turned off at 0.1 have stopped, and the others still play.
instr Check
  iStopped[] fillarray 0, 1, 1, 0, 0
  iFailed = 0
  if giInits != 4 then
    prints("%d init passes, expected 4\n", giInits)
    iFailed = 1
  endif
  iSlot = 1
  while iSlot < 5 do
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
i "Hold.2" 0 -1 2
i 1.3 0 -1 3
i "Hold.4" 0 -1 4
i "Off" 0.1 0
i "Check" 0.3 0
e
</CsScore>
</CsoundSynthesizer>
