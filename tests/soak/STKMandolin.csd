<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKMandolin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6			;body size
kv3	=	p7				;sustain

asig	STKMandolin cpspch(ifrq), 1, 2, kv1, 4, 10, 11, kv3, 1, 100, 128, 100
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 .3 7.00 100 0 20
i 1 + .  8.00 10 100 20
i 1 + .  8.00 100 0 120
i 1 + 4  8.00 10 10 127
e
</CsScore>
</CsoundSynthesizer>