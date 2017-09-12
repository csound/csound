<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o distort.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

gifn	ftgen	0,0, 257, 9, .5,1,270	; define a sigmoid, or better 
;gifn	ftgen	0,0, 257, 9, .5,1,270,1.5,.33,90,2.5,.2,270,3.5,.143,90,4.5,.111,270

instr 1

kdist	line	0, p3, 2		; and over 10 seconds
asig	poscil	0.3, 440, 1
aout	distort	asig, kdist, gifn	; gradually increase the distortion
	outs	aout, aout

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1
i 1 0 10
e

</CsScore>
</CsoundSynthesizer>
