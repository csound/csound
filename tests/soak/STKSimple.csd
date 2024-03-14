<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKSimple.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6				;Filter Pole Position
kv2	line	20, p3, 90				;Noise/Pitched Cross-Fade

asig	STKSimple cpspch(p4), 1, 2, kv1, 4, kv2, 11, 10, 128, 120
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 5 7.00 100 0
i 1 + .  7.05 10 127
i 1 + .  8.03 100 0
i 1 + .  5.00 10 10
e
</CsScore>
</CsoundSynthesizer>
