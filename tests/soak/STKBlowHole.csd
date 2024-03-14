<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKBlowHole.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	

ipch	=	p4
kv1	=	p7						;stiffness of reed
kv3	line	p5, p3, p6					;state of tonehole

asig	STKBlowHole	cpspch(ipch), 1, 2, kv1, 4, 100, 11, kv3, 1, 10, 128, 100
	outs	asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 4 10.00 20 127 100
i 1 + 7 6.09 120  0  10
e
</CsScore>
</CsoundSynthesizer>
