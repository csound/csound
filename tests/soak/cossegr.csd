<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0   ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o linsegr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

	 	 
icps	cpsmidi	 	 
iamp	ampmidi	.3	 
 	 	 	 
kenv	cossegr	1, .05, 0.5, 1, 0
asig	pluck	kenv, icps, 200, 1, 1	 
	outs	asig, asig
	 
endin
</CsInstruments>
<CsScore>
f 1 0 4096 10 1	;sine wave

f0 30	;runs 30 seconds
e
</CsScore>
</CsoundSynthesizer>