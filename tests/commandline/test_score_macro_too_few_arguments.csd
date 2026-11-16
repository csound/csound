<CsTest>
description = "a score macro called with too few arguments is said and replaced, and the score goes on"

[expect]
exit = 0
output = ["Too few arguments to macro", "the score went on after the call"]
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

; After saying there were too few arguments, the lexer read the next
; argument all the same, to the end of the score, which stopped with
; "unexpected EOF!!". The call is now replaced with $error, as an undefined
; macro's is, and the reader says so and drops the rest of its line.
instr Note
endin

instr Check
  prints("the score went on after the call\n")
endin
</CsInstruments>
<CsScore>
#define PAIR(x'y) #3#
i "Note" 0 0 $PAIR(1)
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
