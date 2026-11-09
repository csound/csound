<CsTest>
description = "A string code with no string behind it gives an opcode a name it cannot open, not a crash"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "readf: failed to open file", "ftsave: unable to open file"]
output = ["reached the end of the run"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

giA ftgen 1, 0, 8, 10, 1

; p4 is a string p-field here: as a number it is its string code, and copied
; to instr 2, whose note has no string, it has nothing to look up.
instr 1
  iCode = p4
  schedule 2, 0, 0.01, iCode
endin

instr 2
  S_, i_ readfi p4
endin

instr 3
  iCode = p4
  schedule 4, 0, 0.01, iCode
endin

instr 4
  ftsave p4, 0, 1
endin

instr 5
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 "unused"
i 3 0 0.01 "unused"
i 5 0.1 0.01
</CsScore>
</CsoundSynthesizer>
