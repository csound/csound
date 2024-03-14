<CsoundSynthesizer>
<CsOptions>
csound -m255 -M0 -+rtmidi=null -RWf --midi-key=4 --midi-velocity=5 -o jacko_test.wav
</CsOptions>
<CsInstruments>

;;;;
;;;; NOTE: this csd must be run after starting "aeolus -t".
;;;;

sr    	   = 48000
	   ; The control rate must be BOTH a power of 2 (for Jack)
	   ; AND go evenly into sr. This is about the only one that works!
ksmps 	   = 128
nchnls 	   = 2
0dbfs 	   = 1

	   JackoInit		"default", "csound"

	   ; To use ALSA midi ports, use "jackd -Xseq"
	   ; and use "jack_lsp -A -c" or aliases from JackInfo,
	   ; probably together with information from the sequencer,
	   ; to figure out the damn port names.

	   ; JackoMidiInConnect   "alsa_pcm:in-131-0-Master", "midiin"
	   JackoAudioInConnect 	"aeolus:out.L", "leftin"
	   JackoAudioInConnect 	"aeolus:out.R", "rightin"
	   JackoMidiOutConnect 	"midiout", "aeolus:Midi/in"

           ; Note that Jack enables audio to be output to a regular
 	   ; Csound soundfile and, at the same time, to a sound 
	   ; card in real time to the system client via Jack. 

       	   JackoAudioOutConnect "leftout", "system:playback_1"
	   JackoAudioOutConnect "rightout", "system:playback_2"
	   JackoInfo

	   ; Turning freewheeling on seems automatically 
           ; to turn system playback off. This is good!

	   JackoFreewheel	1
	   JackoOn

	   alwayson		"jackin"

	   instr 1
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ichannel   =			p1 - 1
itime 	   = 			p2
iduration  = 			p3
ikey 	   = 			p4
ivelocity  = 			p5
	   JackoNoteOut 	"midiout", ichannel, ikey, ivelocity
	   print 		itime, iduration, ichannel, ikey, ivelocity
	   endin

	   instr jackin
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	   JackoTransport	3, 1.0
aleft 	   JackoAudioIn		"leftin"
aright 	   JackoAudioIn 	"rightin"

	   ; Aeolus uses MIDI controller 98 to control stops. 
           ; Only 1 data value byte is used, not the 2 data 
	   ; bytes often used  with NRPNs. 
           ; The format for control mode is 01mm0ggg:
	   ; mm 10 to set stops, 0, ggg group (or Division, 0 based).
	   ; The format for stop selection is 000bbbbb:   
	   ; bbbbb for button number (0 based).

	   ; Mode to enable stops for Divison I: b1100010 (98 
           ; [this controller VALUE is a pure coincidence]).

	   JackoMidiOut          "midiout", 176, 0, 98, 98 

	   ; Stops: Principal 8 (0), Principal 4 (1) , Flote 8 (8) , Flote 2 (10)

	   JackoMidiOut          "midiout", 176, 0, 98, 0
	   JackoMidiOut          "midiout", 176, 0, 98, 1
	   JackoMidiOut          "midiout", 176, 0, 98, 8
	   JackoMidiOut          "midiout", 176, 0, 98, 10

	   ; Sends audio coming in from Aeolus out
	   ; not only to the Jack system out (sound card),
	   ; but also to the output soundfile.
           ; Note that in freewheeling mode, "leftout"
           ; and "rightout" simply go silent.

 	   JackoAudioOut 	"leftout", aleft
	   JackoAudioOut 	"rightout", aright
	   outs  		aright, aleft
	   endin

</CsInstruments>
<CsScore>
f 0 30
i 1 1 30 60 60
i 1 2 30 64 60
i 1 3 30 71 60
e 2
</CsScore>
</CsoundSynthesizer>

