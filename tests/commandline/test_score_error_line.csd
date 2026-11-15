<CsTest>
description = "a score error says the line of the .csd it is on"

[expect]
exit = 0
output = ["Name missing not found", "in line 30 of", "illegal number format", "in line 31 of"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; The reader said where an error was through a stack of the inputs it read,
; each a file or a macro. It reads one expanded text now, whose entries have
; no macro, so it said "Internal error in print_input_backtrace()" instead of
; the line.
instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0
s
; a comment, and a blank line

n missing
i 1 0 2e
e
</CsScore>
</CsoundSynthesizer>
