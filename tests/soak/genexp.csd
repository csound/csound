<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o genexp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1

aswp	linseg	0.01, p3*.5, .49, p3*.5, 0.01		;index sweep function
aindex	poscil	aswp, 110, 1				;sound to waveshape
atable	tablei	aindex, p4, 1, .5			;waveshape index
aenv	linen	0.8, .01, p3, .02			;amplitude envelope
asig	= (atable*aenv)*p5				;impose envelope and scale
asig    dcblock2 asig					;get rid of DC
	outs    asig, asig

endin		
</CsInstruments>
<CsScore>
f 1 0 8192 10 1	;sine wave
f 2 0 8192 "exp" 0 15 0
f 3 0 8192 "exp" 0 3 0

i1 0 3 2 2
i1 + 3 3 3

e
</CsScore>
</CsoundSynthesizer>

