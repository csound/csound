<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o downsamp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr	1

ifrq	= cpspch(p4)
ain	diskin2 "beats.wav", 1
aenv	follow	ain, .001	;take the amplitude every 1/1000th of a second
alow	tone	aenv, 25	;lowpass-filter (25 Hz) for a clean signal
kenv	downsamp alow
asig    pluck   kenv, ifrq, 15, 0, 1
	outs	asig, asig
	endin

</CsInstruments>
<CsScore>

i 1 0 2	 9
i 1 + .	 7
i 1 + .	 5

e
</CsScore>
</CsoundSynthesizer>

