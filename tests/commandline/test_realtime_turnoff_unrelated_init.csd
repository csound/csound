<CsTest>
description = "under --realtime, an unrelated init does not hold a turnoff of an active note, nor the score after it"
args = ["-d", "-odac", "-+rtaudio=null", "--realtime"]

[expect]
exit = 0
output = ["the turnoff and the table did not wait"]
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

; Note 1.1 is active when Slow is queued, and its turnoff comes in the same
; k-cycle. Slow's init runs on the allocation thread for 0.2 s of the
; performance. The turnoff waited for it, and the score after it, the table
; at 0.2, waited too. At the end of Slow's init the note is off and the
; table made. Clock gives the performance's time to an init.
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
  if active:i(1, 0, 1) != 0 then
    prints("the turnoff waited for an unrelated init\n")
    exitnow(1)
  endif
  if ftexists:i(2) == 0 then
    prints("the score waited for an unrelated init\n")
    exitnow(1)
  endif
  prints("the turnoff and the table did not wait\n")
  scoreline_i("e")
endin
</CsInstruments>
<CsScore>
i "Clock" 0 -1
i 1.1 0.05 -1
i "Slow" 0.1 0
i -1.1 0.101 0
f 2 0.2 8 -2 7
e 5
</CsScore>
</CsoundSynthesizer>
