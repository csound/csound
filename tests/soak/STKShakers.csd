<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKShakers.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4

asig	STKShakers cpspch(p4), 1, 2, 10, 4, 10, 11, 10, 1, 112, 128, 80, 1071, 5
asig	=	asig *2				;amplify
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0.2 .5 7.00 75  0  20

e
</CsScore>
</CsoundSynthesizer>
