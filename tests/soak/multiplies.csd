<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o multiplies.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kenv    expon 0.01, p3, 1
aout    poscil	0.8*kenv, 440, 1	;multiply amplitude from 0 to 1 * 0.8
printks "base amplitude * rising expon output = 0.8 * %f\n", .1, kenv
	outs 	aout, aout
endin

</CsInstruments>
<CsScore>

f 1 0 16384 10 1	; sine wave

i 1 0 2

e

</CsScore>
</CsoundSynthesizer>
