<CsTest>
description = "All Notes Off reaches the channels of its own MIDI port"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F all_notes_off_held.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

; A channel starts the instr of its number: 1 for port 0's first channel,
; 17 for port 1's, 33 for port 2's and 49 for port 3's.
instr 1
endin

instr 17
endin

instr 33
endin

instr 49
endin

instr Play
  iFile midifileopen strget(p4), p5
  midifileplay iFile
endin

; p4 and p6: instrs; p5 and p7: how many notes each should have.
instr Check
  iFirst active p4
  iSecond active p6
  if iFirst != p5 || iSecond != p7 then
    prints "at %.2f: instr %d has %d notes and instr %d %d, not %d and %d\n", \
           times:i(), p4, iFirst, p6, iSecond, p5, p7
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; all_notes_off.mid: a key on channel 1 at 0, All Notes Off on channel 2 at
; .25, its note-off at .75; all_notes_off_last.mid the same with All Notes
; Off on channel 16. all_notes_off_held.mid: a key on channel 1 from 0 to 1.
; Port 1's All Notes Off releases port 1's key, and not port 0's.
i "Play" 0 .01 "all_notes_off.mid" 1
i "Check" .1 0 1 1 17 1
i "Check" .5 0 1 1 17 0
; Port 0's releases port 0's key, and not port 1's.
i "Play" 1.5 .01 "all_notes_off.mid" 0
i "Play" 1.5 .01 "all_notes_off_held.mid" 1
i "Check" 1.6 0 1 1 17 1
i "Check" 2 0 1 0 17 1
; Port 2's, on its last channel, releases port 2's key, and not port 3's.
i "Play" 3 .01 "all_notes_off_last.mid" 2
i "Play" 3 .01 "all_notes_off_held.mid" 3
i "Check" 3.1 0 33 1 49 1
i "Check" 3.5 0 33 0 49 1
</CsScore>
<CsFileB filename="all_notes_off.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAEgCQPGSBcLF7AINggDwAAP8vAA==
</CsFileB>
<CsFileB filename="all_notes_off_last.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAEgCQPGSBcL97AINggDwAAP8vAA==
</CsFileB>
<CsFileB filename="all_notes_off_held.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAADQCQPmSHQIA+AAD/LwA=
</CsFileB>
</CsoundSynthesizer>
