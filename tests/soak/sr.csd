<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
; -iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;use sr to find maximum harmonics

ihar	= int(sr/2/p4)		; maximum possible number of harmonics w/o aliasing
prints  "maximum number of harmonics = %d \\n", ihar
kenv	linen .5, 1, p3, .2	; envelope
asig	buzz  kenv, p4, ihar, 1
	outs  asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 4096 10 1	;sine wave

i 1 0 3 100	;different frequencies
i 1 + 3 1000
i 1 + 3 10000
e
</CsScore>
</CsoundSynthesizer>

