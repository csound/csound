<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKWhistle.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6				;Blowing Frequency Modulation
kv3	=	p7					;Fipple Modulation Frequency

asig	STKWhistle cpspch(p4), 1, 4, 20, 11, kv3, 1, 100, 2, kv1, 128, 127
asig	=	asig*.7				;too loud
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 .5 9.00 100 30 30
i 1 1 3  9.00 100  0 20
i 1 4.5 . 9.00 1  0 100 
e
</CsScore>
</CsoundSynthesizer>
