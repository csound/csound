<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0    ;;;realtime audio out and realtime midi in 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o midic7.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; This example expects MIDI controller input on channel 1
; run, play a note and move your midi controller 7 to see result

imax  = 1
imin  = 0
ichan = 1 
ictlno= 7 	; = midi volume

kamp	midic7	ictlno, imin, imax	
	printk2	kamp
asig	oscili	kamp, 220, 1
	outs	asig, asig

endin
</CsInstruments>
<CsScore>
; no score events allowed
f 0 20		;20 sec. for real-time MIDI events
f 1 0 4096 10 1	;sine wave

e
</CsScore>
</CsoundSynthesizer>
