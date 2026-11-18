<CsTest>
description = "a note whose p1 is past every instrument's number carries no p-field into another instrument's note"

[expect]
exit = 1
stderr = ["instr 65537"]
output = ["instr 1 took nothing from instr 65537"]
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

; The score's sorter keeps a note's instrument in an int16, and converted p1
; to it, which is undefined past the int16's range. Here 65537 came out as 1,
; so instr 1's next note took its missing p4 from the note for 65537, which
; plays nothing: there is no such instrument, and it is deleted.
instr 1
  if pcount() != 3 then
    prints("instr 1 has %d p-fields, p4 %g, from instr 65537\n", pcount(), p4)
    exitnow(1)
  endif
  prints("instr 1 took nothing from instr 65537\n")
endin
</CsInstruments>
<CsScore>
i 65537 0 0 5
i 1 0.1 0
e
</CsScore>
</CsoundSynthesizer>
