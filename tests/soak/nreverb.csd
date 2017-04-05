<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o nreverb.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

gaout init  0

instr 1
a1	oscil	15000, 440, 1
	out	a1
 
gaout = gaout+a1
endin

instr 99

a2	nreverb	gaout, 2, .3
	out	a2*.15		;volume of reverb		
	 
gaout = 0
endin

</CsInstruments>
<CsScore>

; Table 1: an ordinary sine wave.
f 1 0 32768 10 1 
         
i 1 0 .5
i 1 1 .5
i 1 2 .5
i 1 3 .5
i 1 4 .5
i 99 0 9 
e


</CsScore>
</CsoundSynthesizer>
