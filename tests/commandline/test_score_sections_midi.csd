<CsTest>
description = "Offline score sections finish when MIDI input is enabled but idle"
args = ["-n", "-d", "-m0", "--test", "-+rtmidi=null", "-M0"]

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

; Dummy MIDI input enables realtime event polling without hardware.
; Rendering is still offline. Each control block lasts one millisecond.
instr CountBlocks
  gkBlocks += 1
endin

instr CheckBlocks
  assert(i(gkBlocks) == p4)
  prints "completed %d blocks\n", i(gkBlocks)
endin
</CsInstruments>
<CsScore>
i "CountBlocks" 0     0.005
i "CheckBlocks" 0.005 0     5
s
; No realtime event is due while the second section waits for this note.
; Reusing the previous section-end result used to loop here forever.
i "CountBlocks" 0.002 0.005
i "CheckBlocks" 0.007 0     10
e
</CsScore>
</CsoundSynthesizer>
