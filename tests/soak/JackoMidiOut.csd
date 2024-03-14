<CsoundSynthesizer>
<CsOptions>
-n ; no sound
</CsOptions>
<CsInstruments>

sr      = 48000 ; one possible Jack setting
ksmps   = 128
nchnls  = 2

; by Menno Knevel - 2023
; The example shows how Control Changes and Program Change 
; to drive an external synthesizer

JackoInit   "default", "csound6"                        ; Csound as a Jack client
JackoMidiOutConnect "midioutMAUDIO", "M-Audio-Delta-1010:midi/capture_1"

instr 1	

irandom     random      30, 80
JackoMidiOut  "midioutMAUDIO", 192, 1-1, 21             ; Program Change (GM Musette)
JackoMidiOut  "midioutMAUDIO", 176, 1-1, 74, p4         ; Control Change Brightness
JackoNoteOut  "midioutMAUDIO", 1-1, irandom, 100        ; channel range 0-15

endin

</CsInstruments>
<CsScore>

i1 1 1 100
i1 2 1 50
i1 3 3 30
i1 6 2 127
e 
</CsScore>
</CsoundSynthesizer>
