<CsoundSynthesizer>
<CsOptions>
csound -m255 -RWfo jacko_test.wav
</CsOptions>
<CsInstruments>

; Unit tests for the Jack opcodes.
; To do:
; (1) MIDI stop setup for Aeolus.
; (2) Python score generator and client runner.
; (3) Test sequencer input to Csound.

sr    	   = 48000
ksmps 	   = 128
nchnls 	   = 2
0dbfs 	   = 1

	   JackInit		"default", "csound"
	   JackAudioInConnect 	"aeolus:out.L", "leftin"
	   JackAudioInConnect 	"aeolus:out.R", "rightin"
	   JackMidiOutConnect 	"midiout", "aeolus:Midi/in"

           ; Note that Jack enables audio to be output to a regular
 	   ; Csound soundfile and, at the same time, to a sound 
	   ; card in real time to the system client via Jack. 

       	   JackAudioOutConnect 	"leftout", "system:playback_1"
	   JackAudioOutConnect 	"rightout", "system:playback_2"
	   JackInfo

	   ; Turning freewheeling on seems to turn system playback off.
	   ; This is good!

	   JackFreewheel	0
	   JackOn

	   alwayson		"jackin"

	   instr 1
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ichannel   =			p1 - 1
itime 	   = 			p2
iduration  = 			p3
ikey 	   = 			p4
ivelocity  = 			p5
	   JackNoteOut 		"midiout", ichannel, ikey, ivelocity
	   print 		itime, iduration, ichannel, ikey, ivelocity
	   endin

	   instr jackin
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
aleft 	   JackAudioIn		"leftin"
aright 	   JackAudioIn 		"rightin"

	   ; Select Aeolus' stops using MIDI controller number 98.

	   ; Aeolus can use MIDI controller 98 to control stops.
	   ; Format is 01mm0ggg, mm 10 to set stops, ggg group (or Division, 0 based).
	   ; Then 000bbbbb is button number (0 based).

	   ; Enable stops for Divison I: b1100010 (98 [this controller VALUE is a coincidence]).

	   JackMidiOut          "midiout", 176, 0, 98, 98 

	   ; Stops: Principal 8 (0), Principal 4 (1) , Flote 8 (8) , Flote 2 (10)

	   JackMidiOut          "midiout", 176, 0, 98, 0
	   JackMidiOut          "midiout", 176, 0, 98, 1
	   JackMidiOut          "midiout", 176, 0, 98, 8
	   JackMidiOut          "midiout", 176, 0, 98, 10

	   ; Send audio coming from Aeolus
	   ; not only to the Jack system out (sound card),
	   ; but also to the output soundfile.

 	   JackAudioOut 	"leftout", aleft
	   JackAudioOut 	"rightout", aright
	   outs  		aright, aleft
	   endin

</CsInstruments>
<CsScore>

i 1 1 30 60 60
i 1 2 30 64 60
i 1 3 30 71 60
e 2

</CsScore>
</CsoundSynthesizer>

