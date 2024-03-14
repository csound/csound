<CsoundSynthesizer>
<CsOptions>
-n ; no sound
</CsOptions>
<CsInstruments>

sr      = 48000 ; one possible Jack setting
ksmps   = 128
nchnls  = 2

; by Menno Knevel - 2023

JackoInit   "default", "csound6"						; Csound as a Jack client
JackoMidiOutConnect "midioutMAUDIO", "M-Audio-Delta-1010:midi/capture_1"	; create 1 Midi port
JackoMidiOutConnect "midioutEDIROL", "UM-3:midi/capture_1"			; create 2nd Midi port

	instr 1	; send notes to the M-Audio Midi port	

irandom     random      30, 80
JackoNoteOut "midioutMAUDIO", 1-1, irandom, 100		    ; channel range 0-15
	endin

      instr 2 ; send notes to Edirol Midi port	

irandom     random      30, 80
JackoNoteOut "midioutEDIROL", 1-1, irandom, 100
	endin

</CsInstruments>
<CsScore>
s
i1 1 .1
i1 2 1
i1 4 2
s
i2 1 .1
i2 2 1
i2 4 2
e 
</CsScore>
</CsoundSynthesizer>
