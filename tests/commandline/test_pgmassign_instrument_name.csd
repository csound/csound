<CsTest>
description = "pgmassign assigns a program to a named instrument"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F pgmassign_name_empty.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

giHeard[] init 2

; A program with no instr of its number leaves channel 1 on instr 1.
instr 1
  prints "program %d did not reach its named instrument\n", midipgm()
  exitnow -1
endin

instr Heard
  giHeard[0] = giHeard[0] + 1
endin

instr Also
  giHeard[1] = giHeard[1] + 1
endin

; The name as a string argument.
pgmassign 10, "Heard"

; The name as a score p-field.
instr Assign
  pgmassign 11, p4
endin

giCarried init 0

; A string code with no string behind it - a NaN, or a p-field string
; carried to an instr started by number - names no instrument.
instr 99
  iNaN = sqrt(-1)
  pgmassign 12, iNaN
  pgmassign 13, giCarried
endin

instr Carry
  giCarried = p5
endin

instr Play
  iFile midifileopen strget(p4)
  midifileplay iFile
endin

instr Check
  if giHeard[0] != 1 || giHeard[1] != 1 then
    print giHeard[0], giHeard[1]
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Program 10, then a note, on channel 1.
i "Play" 0 .01 "pgmassign_name_10.mid"
i "Assign" .75 0 "Also"
; Program 11, then a note.
i "Play" 1 .01 "pgmassign_name_11.mid"
i "Carry" 1.4 0 "x" "Heard"
i 99 1.5 0
i "Check" 2 0
</CsScore>
<CsFileB filename="pgmassign_name_10.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAEADACQCQPGSDYIA8AAD/LwA=
</CsFileB>
<CsFileB filename="pgmassign_name_11.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAEADACgCQPGSDYIA8AAD/LwA=
</CsFileB>
<CsFileB filename="pgmassign_name_empty.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAABYNg/y8A
</CsFileB>
</CsoundSynthesizer>
