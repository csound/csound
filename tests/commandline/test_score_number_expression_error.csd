<CsTest>
description = "a b, B, C or v whose [] has an error is ignored, the next line is read, and the message ends its line"

[expect]
exit = 0
output = ["every note after a b, B, C or v error played, unmoved"]
output_regex = ['''unmatched '\)' in \[\] expression\n  section 1:  at position 4\n''']
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

; An unmatched bracket or a missing operand in a [] is said, and the rest of
; its line is dropped. b, B, C and v then dropped the rest of their line again,
; which was the next line: the note after each of them was lost. And they set
; the clock, the carry or the warp from what the error left: here the clock to
; 5, the carry off and the warp to 0. They are ignored now, as without a
; number. Each note says what it adds up to and the p3 it must have. The
; message of such an error ran on into where it was.
giSum init 0
giWrong init 0

instr Note
  if p2 != 0 || p3 != p5 then
    prints("note %g is at %g for %g\n", p4, p2, p3)
    giWrong += 1
  endif
  giSum += p4
endin

; A note with an error in a [] still plays, with what the error left.
instr Mark
endin

instr Check
  if giSum != 18 || giWrong != 0 then
    prints("the notes added up to %d, not 18\n", giSum)
    exitnow(1)
  endif
  prints("every note after a b, B, C or v error played, unmoved\n")
endin
</CsInstruments>
<CsScore>
b [5)]
i "Note" 0 0 1 0
B [(5]
i "Note" 0 0 2 0
C [0)]
i "Note" 0 0 3 0
i "Note" 0 0
v [)]
i "Note" 0 0.01 4 0.01
i "Mark" 0 0 [5)]
i "Note" 0 0 5 0
i "Check" 0.1 0
e
</CsScore>
</CsoundSynthesizer>
