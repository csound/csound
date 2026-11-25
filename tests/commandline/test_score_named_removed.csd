<CsTest>
description = "a score's named event checks its instrument as a numbered one does"

[expect]
exit = 4
stderr = ["Setting instrument -Hold on", "instr Hold undefined", "instr -Hold undefined"]
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

; q took a negative name's number as an index. remove clears an instrument's
; definition and leaves its name, which still finds the number: the four
; events after it used the definition. Each crashed.
instr Hold
  prints("Hold played\n")
endin

instr Remove
  remove("Hold")
endin
</CsInstruments>
<CsScore>
q "-Hold" 0 1
i "Remove" 0.05 0
i "Hold" 0.1 0.1
i "-Hold" 0.15 0
d "Hold" 0.2 0
q "Hold" 0.25 0
e 0.3
</CsScore>
</CsoundSynthesizer>
