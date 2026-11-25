<CsTest>
description = "under --realtime, a turnoff waits for its note queued behind an unrelated init"
args = ["-d", "-odac", "-+rtaudio=null", "--realtime"]

[expect]
exit = 0
output = ["no note plays"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-m128 -b 10 -B 20
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

; Note 1.2 is queued behind Slow, whose init runs for 0.2 s of the
; performance, and its turnoff comes in the same k-cycle: the turnoff waits
; for the note, and so for Slow. Check is read after the turnoff. Clock
; gives the performance's time to an init.
gkNow init 0

instr Clock
  gkNow = times:k()
endin

instr 1
endin

instr Slow
  iStart = i(gkNow)
  iSpin = 0
  while i(gkNow) < iStart + 0.2 && iSpin < 1e9 do
    iSpin += 1
  od
endin

instr Check
  if active:i(1, 0, 1) != 0 then
    prints("note 1.2 still plays\n")
    exitnow(1)
  endif
  prints("no note plays\n")
  scoreline_i("e")
endin
</CsInstruments>
<CsScore>
i "Clock" 0 -1
i "Slow" 0.1 0
i 1.2 0.1 -1
i -1.2 0.101 0
i "Check" 0.2 0
e 5
</CsScore>
</CsoundSynthesizer>
