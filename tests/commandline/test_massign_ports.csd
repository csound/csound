<CsTest>
description = "massign reaches the channels of every MIDI port"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F massign_ports_empty.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

; Port p's channels are 16p + 1 to 16p + 16; channel 0 is all of them.
massign 0, 0

giHeard[] init 1025

; A channel without an instr of its number starts the first instr.
instr 1
  prints "channel %d was not muted\n", midichn()
  exitnow -1
endin

instr Heard
  iChannel = midichn()
  giHeard[iChannel] = giHeard[iChannel] + 1
endin

instr Assign
  massign p4, "Heard"
endin

instr Play
  iFile midifileopen "massign_ports.mid", p4
  midifileplay iFile
endin

instr Check
  if giHeard[17] != 1 || giHeard[32] != 0 || giHeard[1009] != 1 || giHeard[1024] != 1 then
    print giHeard[17], giHeard[32], giHeard[1009], giHeard[1024]
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Port 1 (channels 17 and 32) and port 63 (1009 and 1024): muted.
i "Play" 0 .01 1
i "Play" 0 .01 63
; One channel past the first port.
i "Assign" .75 0 17
i "Play" 1 .01 1
; Every channel of every port.
i "Assign" 1.75 0 0
i "Play" 2 .01 63
i "Check" 3 0
</CsScore>
<CsFileB filename="massign_ports.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAFgCQPGQAn0Bkg2CAPAAAj0AAg2D/LwA=
</CsFileB>
<CsFileB filename="massign_ports_empty.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAABYNg/y8A
</CsFileB>
</CsoundSynthesizer>
