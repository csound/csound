<CsTest>
description = "pgmassign reaches the channels of every MIDI port"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F pgmassign_ports_empty.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

giHeard[] init 1025
giAlso[] init 1025

; A channel without an instr of its number starts the first instr, and
; keeps it on a program with no instr of its number - 10 here.
instr 1
  prints "channel %d did not change program\n", midichn()
  exitnow -1
endin

instr Heard
  iChannel = midichn()
  giHeard[iChannel] = giHeard[iChannel] + 1
endin

instr Also
  iChannel = midichn()
  giAlso[iChannel] = giAlso[iChannel] + 1
endin

; p4: the program, 0 for all; p5: 0 for Heard, 1 for Also; p6: the channel,
; 0 for all.
instr Assign
  pgmassign p4, nstrnum(p5 == 0 ? "Heard" : "Also"), p6
endin

instr Play
  iFile midifileopen "pgmassign_ports.mid", p4
  midifileplay iFile
endin

instr Check
  if giHeard[17] != 1 || giHeard[33] != 1 || giHeard[993] != 1 || \
     giHeard[1009] != 0 || giAlso[1009] != 1 then
    print giHeard[17], giHeard[33], giHeard[993], giHeard[1009], giAlso[1009]
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Each file plays program 10, then a note, on its port's first channel.
; Program 10 on channel 17, port 1's first.
i "Assign" 0 0 10 0 17
i "Play" .25 .01 1
; Every program on channel 33, port 2's first.
i "Assign" 1 0 0 0 33
i "Play" 1.25 .01 2
; Every program on every channel: port 62's first, 993.
i "Assign" 2 0 0 0 0
i "Play" 2.25 .01 62
; Program 10 on every channel, to Also: port 63's first, 1009.
i "Assign" 3 0 10 1 0
i "Play" 3.25 .01 63
i "Check" 4 0
</CsScore>
<CsFileB filename="pgmassign_ports.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAEADACQCQPGSDYIA8AAD/LwA=
</CsFileB>
<CsFileB filename="pgmassign_ports_empty.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAABYNg/y8A
</CsFileB>
</CsoundSynthesizer>
