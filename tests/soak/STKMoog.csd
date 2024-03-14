<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKMoog.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6				;filter Q

asig	STKMoog cpspch(ifrq), 1, 2,kv1, 4, 120, 11, 40, 1, 1, 128, 120
asig	=	asig * .3				;too loud
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 .5 6.00 100 0
i 1 + .  5.05 10 127
i 1 + .  7.06 100 0
i 1 + 3  7.00 10 10
e
</CsScore>
</CsoundSynthesizer>
