<CsTest>
description = "an m statement that names its mark again moves it, as when another mark comes between"

[expect]
exit = 0
output = ["each n repeated the section its name last marked"]
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

; m looks for its name among the marks to move it there, but stopped short of
; the last one: a name marked again right after itself got a second entry,
; and n found the first. With another mark between them, it moved.
giSum init 0

instr Note
  giSum += p4
endin

instr Check
  if giSum != 49 then
    prints("the notes added up to %d, not 49\n", giSum)
    exitnow(1)
  endif
  prints("each n repeated the section its name last marked\n")
endin
</CsInstruments>
<CsScore>
m a
i "Note" 0 0 1
s
m a
i "Note" 0 0 2
s
n a
m b
i "Note" 0 0 4
s
m other
i "Note" 0 0 8
s
m b
i "Note" 0 0 16
s
n b
i "Check" 0 0
e
</CsScore>
</CsoundSynthesizer>
