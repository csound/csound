<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -M0 --midi-key-cps=4 --midi-velocity-amp=5   ;;;realtime audio out and realtime midi in, midi key cps is routed to p4 and velocity to p5
; For Non-realtime ouput leave only the line below:
; -o midic14.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by tgrey - 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
	; This example expects MIDI controller input on channel 1
	; run, play a note and move your midi controllers 1 and 7 to see results
	ictlno1= 1 	; = cc #1 midi mod wheel (course tuning)
	ictlno2= 7 	; = cc #7 midi volume (fine tuning)

	; max range is 3 octaves: (2^3) = 8
	imax  = 8

	; read both controllers, scaling them between 1 and imax
	kTune midic14 ictlno1, ictlno2, 1, imax	
	printk2	kTune

	; generate tones
	asig oscili p5, p4*kTune
	aref oscili p5, p4

	; combine detuned tone and reference tone
	; creates a beat effect from the detune
	asig=(asig+aref)*.5

	outs asig, asig
endin
</CsInstruments>
<CsScore>
; run for 60 seconds
f0 60
e
</CsScore>
</CsoundSynthesizer>
