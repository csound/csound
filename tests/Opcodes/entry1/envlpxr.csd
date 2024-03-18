<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0   ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o envlpxr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
 	 
icps	cpsmidi	 	 
iamp	ampmidi	.5	 
 	 	 	 
kenv	envlpxr	iamp, 0.2, 1, 1, 1, 0.01
asig	pluck	kenv, icps, 200, 2, 1	 
	outs	asig, asig
	 
endin
</CsInstruments>
<CsScore>
f 1 0 129 -7 0 128 1
f 2 0 4096 10 1	

f0 30	;runs 30 seconds
e
</CsScore>
</CsoundSynthesizer>