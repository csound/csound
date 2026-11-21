<CsTest>
description = "an n statement on the line after a note repeats its section, and is not that note's p-field"

[expect]
exit = 0
output = ["the n after a note repeated its section"]
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

; A statement goes on into the next line when that line begins with a
; p-field, and an n may begin one, as np carries. So the n statement after a
; note was read as the note's p5, and the mark's name as a stray character:
; the section was not repeated. An np carry that begins a line still goes on
; the note before it.
giNotes init 0
giSum init 0

instr Note
  if pcount() != 4 then
    prints("note %g has %d p-fields, not 4\n", p4, pcount())
    exitnow(1)
  endif
  giNotes += 1
  giSum += p4
endin

instr Carry
  if p4 == 5 && p5 != 7 then
    prints("the np4 on its own line gave %g, not 7\n", p5)
    exitnow(1)
  endif
endin

instr Check
  if giNotes != 3 || giSum != 4 then
    prints("%d notes added up to %d, not 3 to 4\n", giNotes, giSum)
    exitnow(1)
  endif
  prints("the n after a note repeated its section\n")
endin
</CsInstruments>
<CsScore>
m a
i "Note" 0 0 1
s
i "Note" 0 0 2
n a
i "Carry" 0 0 5
np4
i "Carry" 0 0 7 0
i "Check" 0.1 0
e
</CsScore>
</CsoundSynthesizer>
