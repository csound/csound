<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKRhodey.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6				;(FM) Modulator Index One
kv5	=	p7					;ADSR 2 and 4 target
asig	STKRhodey cpspch(p4), 1, 2, kv1, 4, 10, 11, 100, 1, 3, 128, kv5
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 .5 7.00 75  0    0
i 1 + .  8.00 120 0    120
i 1 + 1  6.00 50  120  50
i 1 + 4  8.00 10  120  100
e
</CsScore>
</CsoundSynthesizer>
