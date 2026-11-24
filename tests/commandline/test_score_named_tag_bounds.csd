<CsTest>
description = "a score's named event whose tag takes its number past the instrument table is deleted"

[expect]
exit = 6
stderr = ["instr Hold.2e3 undefined", "instr -Hold.1e9 undefined", "instr Hold.1e9 undefined",
          "instr -Hold.1e11 undefined", "instr Hold.1e11 undefined", "instr -Hold.2e3 undefined"]
output = ["Hold plays, p1 1.5"]
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

; A tag is added to the name's number: "Hold.1e9" is instr 1 + 1e8. The
; table holds instrs 0 to 200, so "Hold.2e3", 201, is one past its end, and
; "Hold.1e11" is past an int too. Each event indexed the table with that
; number and crashed. Each is now deleted, as a numbered p1 past the table is.
instr Hold
  prints("Hold plays, p1 %g\n", p1)
endin
</CsInstruments>
<CsScore>
i "Hold.2e3" 0 0.1
i "-Hold.1e9" 0 0
d "Hold.1e9" 0 0
d "-Hold.1e11" 0 0
q "Hold.1e11" 0 0
q "-Hold.2e3" 0 0
i "Hold.5" 0.1 0
e 0.2
</CsScore>
</CsoundSynthesizer>
