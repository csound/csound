<CsTest>
description = "A block comment in <CsInstruments> is closed only by a */ of its own"
args = ["-nd"]

[expect]
exit = 0
output = ["instr 1 ran", "instr 2 ran"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "instr 1 ran\n"
endin
; A line longer than the next, with a comment closer where that one ends: */
/*
</CsInstruments>
<CsVersion>
Before 1.0
</CsVersion>
*/
instr 2
  prints "instr 2 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
i 2 0 0
</CsScore>
</CsoundSynthesizer>
