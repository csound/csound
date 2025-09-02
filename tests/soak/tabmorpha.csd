<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tabmorpha.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs   =1

giSine   ftgen 1, 0, 8193, 10, 1				;sine wave
giSquare ftgen 2, 0, 8193, 7, 1, 4096, 1, 0, -1, 4096, -1	;square wave
giTri    ftgen 3, 0, 8193, 7, 0, 2048, 1, 4096, -1, 2048, 0	;triangle wave
giSaw    ftgen 4, 0, 8193, 7, 1, 8192, -1			;sawtooth wave, downward slope

	
instr	1

iamp	= .7
aindex	phasor 110			; read table value at this index
aweightpoint = 0			; set weightpoint
atabnum1 line 0, p3, 3			; morph through all tables
atabnum2 = 2				; set to triangle wave
asig	 tabmorpha aindex, aweightpoint, atabnum1,atabnum2, giSine, giSquare, giTri, giSaw
asig	 = asig*iamp
	 outs asig, asig

endin

instr	2
iTables[] fillarray 1, 2, 3, 4
iamp	= .7
aindex	phasor 110			; read table value at this index
aweightpoint = 0			; set weightpoint
atabnum1 line 0, p3, 3			; morph through all tables
atabnum2 = 2				; set to triangle wave
asig	 tabmorpha aindex, aweightpoint, atabnum1,atabnum2, iTables
asig	 = asig*iamp
	 outs asig, asig

endin


</CsInstruments>
<CsScore>

i1 0 10
i2 + 10
e
</CsScore>
</CsoundSynthesizer>

