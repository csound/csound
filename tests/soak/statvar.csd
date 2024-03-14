<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d   -M0     ;;;RT audio I/O, midi in
; For Non-realtime ouput leave only the line below:
; -o statevar.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

	instr 1

kenv		linseg	0,0.1,1, p3-0.2,1, 0.1, 0
asig		buzz		16000*kenv, 100, 100, 1
kf		expseg	100, p3/2, 5000, p3/2, 1000
ahp,alp,abp,abr statevar	asig, kf, 200
		outs		alp,ahp					; lowpass left, highpass right
	
	endin
	
</CsInstruments>
<CsScore>
; Table #1, a sine wave.
f 1 0 16384 10 1

i1 0 2 

</CsScore>
</CsoundSynthesizer>