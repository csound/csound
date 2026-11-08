<CsTest>
description = "Offline score sections finish after a queued realtime note"
args = ["-n", "-d", "-m0", "--run-unit-tests"]

[expect]
exit = 0
output = ["completed 5 blocks", "completed 10 blocks"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
gkBlocks init 0

instr CountBlocks
  gkBlocks += 1
endin

instr CheckBlocks
  assert(i(gkBlocks) == p4)
  prints "completed %d blocks\n", i(gkBlocks)
endin

; This note enables realtime event polling without MIDI or --realtime.
; The queue is empty by the time the first score section ends.
event_i "i", "CountBlocks", 0, 0.005
</CsInstruments>
<CsScore>
i "CheckBlocks" 0.005 0     5
s
; A score event in the future must not repeat the previous section end.
i "CountBlocks" 0.002 0.005
i "CheckBlocks" 0.007 0     10
e
</CsScore>
</CsoundSynthesizer>
