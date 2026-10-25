<CsTest>
description = "an orchestra block read from -L sends a score line from its global code, and it plays"
args = ["-nd", "-L", "line_event_reentry.orc"]

[expect]
exit = 0
output = ["the line a compiled block sent was played"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

; line_event_reentry.orc is an orchestra block, { ... }, which sense_line
; compiles as it reads the -L input; its global code sends a score line
; with scorelinei, which goes back into the line-event buffer sense_line is
; reading. That buffer's lock must not be held across the compile: it is
; not recursive, and the same thread taking it again hung or ended Csound.
instr 1
  prints("the line a compiled block sent was played\n")
endin
</CsInstruments>
<CsScore>
f 0 0.1
</CsScore>
</CsoundSynthesizer>
