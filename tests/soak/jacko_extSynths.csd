<CsoundSynthesizer>
<CsOptions>
-o dac -+rtmidi=null -Ma; 
</CsOptions>
<CsInstruments>

sr     = 48000
ksmps  = 128
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2023

JackoInit   "default", "csound6"						; Csound as a Jack client
JackoMidiInConnect "UM-3:midi/playback_2", "midiinEDIROL" 	                ; create 1 Midi in port 
JackoMidiOutConnect "midioutMAUDIO", "M-Audio-Delta-1010:midi/capture_1"	; create 1 Midi output port
JackoMidiOutConnect "midioutEDIROL", "UM-3:midi/capture_1"			; create 2nd Midi output port

instr 1	; get notes from score AND from the EDIROL Midi port "midiinEDIROL"

midinoteoncps p4, p5				            ;Gets a MIDI note number as a cycles-per-second frequency
ikey = p4
ivel = p5/127
kenv madsr 0.05, 0.8, 0.8, 0.5		
aout pluck kenv*ivel, ikey, ikey*.2, 2, 1	
outs aout, aout
endin

instr 2	; send notes to the M-Audio Midi port (external synth #1)	
irandom     random      50, 100
JackoNoteOut "midioutMAUDIO", 1-1, irandom, 100		    ; channel range = 0-15
endin

instr 3 ; send notes to Edirol Midi port (external synth #2)	
irandom     random      30, 80
JackoNoteOut "midioutEDIROL", 1-1, irandom, 100
endin

</CsInstruments>
<CsScore>
f 0 20 ; run for 20 seconds, allows playing instr 1 on keyboard
f 2 0 4096 10 1	

i1 1 3 100 100    ; score notes for instr 1
i1 10 3 1200 100

i2 1 .1
i2 2 1
i2 4 2

i3 11 .1
i3 12 1
i3 14 2
e 
</CsScore>
</CsoundSynthesizer>
