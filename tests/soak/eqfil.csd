<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o eqfil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

kcf	= p4
kfe	expseg	10, p3*0.9, 1800, p3*0.1, 175
kenv	linen	.03, 0.05, p3, 0.05 ;low amplitude is needed to avoid clipping
asig	buzz	kenv, kfe, sr/(2*kfe), 1
afil	eqfil	asig, kcf, 200, 10
	outs	afil*20, afil*20

endin
</CsInstruments>
<CsScore>
; a sine wave.
f 1 0 16384 10 1

i 1 0 10 200	;filter centre freq=200
i 1 + 10 1500	;filter centre freq=1500
e
</CsScore>
</CsoundSynthesizer>
