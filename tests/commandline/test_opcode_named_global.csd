<CsTest>
description = "a global named after an opcode keeps its value through a later compile"

[expect]
exit = 0
output = ["both globals kept their values"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

; Every opcode's name is also a read-only global, an opcode reference, and
; each compilation refreshes them by name. A global of the orchestra's own
; with an opcode's name was taken for one and written as one: its value lost,
; and the write ran past the end of its memory.
table@global:i = 5
oscil@global:i[] = fillarray(1, 2, 3)

; The compile after the one that declared them: without it there is nothing
; to keep a value through, and the check would pass on none.
instr Recompile
  compiled:i = compilestr("instr Later\nendin\n")
  if compiled != 0 then
    prints("compilestr failed\n")
    exitnow(1)
  endif
endin

instr Check
  if table != 5 || lenarray(oscil) != 3 || oscil[2] != 3 then
    prints("table is %g, oscil holds %d\n", table, lenarray(oscil))
    exitnow(1)
  endif
  prints("both globals kept their values\n")
endin
</CsInstruments>
<CsScore>
i "Recompile" 0 0
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
