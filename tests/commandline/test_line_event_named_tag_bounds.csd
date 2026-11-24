<CsTest>
description = "a line event's named p1 whose tag takes its number past the instrument table is refused"

[expect]
exit = 0
stderr = ["instrument Hold.2e3 undefined", "instrument -Hold.1e9 undefined", "instrument Hold.1e9 undefined",
          "instrument -Hold.1e11 undefined", "instrument Hold.1e11 undefined", "instrument -Hold.2e3 undefined"]
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

; A tag is added to the name's number: "Hold.1e9" is instr 1 + 1e8, past
; the table of instrs 0 to 200, and "Hold.1e11" past an int, whose
; conversion was undefined. Each line is refused as a name not found is.
instr Hold
  prints("Hold plays, p1 %g\n", p1)
endin

instr Send
  scoreline_i({{
i "Hold.2e3" 0 0.1
i "-Hold.1e9" 0 0
d "Hold.1e9" 0 0
d "-Hold.1e11" 0 0
q "Hold.1e11" 0 0
q "-Hold.2e3" 0 0
i "Hold.5" 0 0
}})
endin
</CsInstruments>
<CsScore>
i "Send" 0.1 0
e 0.3
</CsScore>
</CsoundSynthesizer>
