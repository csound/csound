<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    -M1  ;;;realtime audio out and midi in 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o midinoteonpch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

midinoteonpch p4, p5				;gets a MIDI note number value as octave-point-decimal value into p4, and MIDI velocity into p5

print 	p4					;display the pitch value when it changes and when key is pressed 
kvel =  kvel/127				;scale midi velocity to 0-1
ipch =	p4
icps =	cpspch(ipch)				;convert octave-point-decimal value into Hz
kenv madsr 0.5, 0.8, 0.8, 0.5			;amplitude envelope multiplied by
asig pluck kenv*kvel, icps, icps, 2, 1		;velocity value			
     outs  asig, asig				

endin
</CsInstruments>
<CsScore>
f 0 30	;runs for 30 seconds
f 2 0 4096 10 1	

i 1 0 2 8.09 100	; play these notes from score as well
i 1 + 2 9.05 100
e

</CsScore>
</CsoundSynthesizer>
