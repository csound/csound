<CsTest>
description = "under --realtime, a turnoff waits for an earlier note of its instrument whose init can change p1, and not when its note is active"
args = ["-d", "-odac", "-+rtaudio=null", "--realtime"]

[expect]
exit = 0
output = ["the active note did not wait", "no note of instr 1 plays"]
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

; A note's init runs for p4 seconds of the performance, then sets p1 to p5
; if p5 is not 0. Clock gives the performance's time to an init.
; Instr 2: note 2.3 is active, 2.5's init is slow and keeps its p1, and
; 2.3's turnoff comes in its k-cycle: it does not wait for 2.5, and when
; 2.5's init ends, 2.3 is off.
; Instr 1: 1.1's init is slow and makes it 1.2, and the turnoff of 1.2
; comes in its k-cycle: it missed the note, which had p1 1.1 until then. It
; now waits for the earlier note of its instrument. Check is read after it.
gkNow init 0

instr Clock
  gkNow = times:k()
endin

opcode Spin(seconds:i):void
  iStart = i(gkNow)
  iSpin = 0
  while i(gkNow) < iStart + seconds && iSpin < 1e9 do
    iSpin += 1
  od
endop

instr 1
  Spin(p4)
  if p5 != 0 then
    p1 = p5
  endif
endin

instr 2
  if p4 != 0 then
    Spin(p4)
    if active:i(2, 0, 1) != 1 then
      prints("the turnoff of an active note waited\n")
      exitnow(1)
    endif
    prints("the active note did not wait\n")
  endif
endin

instr Check
  if active:i(1, 0, 1) != 0 then
    prints("a note of instr 1 still plays\n")
    exitnow(1)
  endif
  prints("no note of instr 1 plays\n")
  scoreline_i("e")
endin
</CsInstruments>
<CsScore>
i "Clock" 0 -1
i 2.3 0.05 -1 0 0
i 2.5 0.1 -1 0.2 0
i -2.3 0.101 0
i 1.1 0.6 -1 0.2 1.2
i -1.2 0.601 0
i "Check" 0.7 0
e 5
</CsScore>
</CsoundSynthesizer>
