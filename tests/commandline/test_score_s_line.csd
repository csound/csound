<CsTest>
description = "the rest of an s line is read as any line: a ; or a number against the letter, a macro, and the end of a macro's text"

[expect]
exit = 0
output = ["every s read the rest of its line"]
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

; An s or e copied the rest of its line by hand. A character against the
; letter was dropped: s;i "Wrong" played its commented note, and s1 lost its
; time, its 1 dropped and its 0 an illegal opcode. A macro after a blank was
; not expanded, so s $T lost its time too. And at the end of a macro's text,
; s and a blank copied the text's end, a NUL, and the score ended there.
giNotes init 0
giWrong init 0

instr Note
  giNotes += 1
endin

instr Wrong
  prints("a commented note played\n")
  giWrong += 1
endin

; p4 is the time its section begins at
instr Start
  if abs(times:i() - p4) > 0.001 then
    prints("a section began at %g, not %g\n", times:i(), p4)
    giWrong += 1
  endif
endin

instr Check
  if giWrong == 0 && giNotes == 3 then
    prints("every s read the rest of its line\n")
  else
    prints("%d of 3 notes played, and %d went wrong\n", giNotes, giWrong)
  endif
endin
</CsInstruments>
<CsScore>
#define T #2#
#define END #s #
i "Note" 0 0
s;i "Wrong" 0 0
i "Note" 0 0
s1
i "Start" 0 0 1
s $T
i "Start" 0 0 3
$END
i "Note" 0 0
i "Check" 0 0
</CsScore>
</CsoundSynthesizer>
