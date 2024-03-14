<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKFMVoices.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6					;vowel
kv2	line	p7, p3, p8					;specral tilt

asig	STKFMVoices cpspch(ifrq), 1, 2, kv1, 4, kv2, 11, 10, 1, 10, 128, 50
asig	=	asig * 4					;amplify
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 5 5.00 10 120 0 0 
i 1 + 2 8.00 80 82 127 0
e
</CsScore>
</CsoundSynthesizer>
