<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKStifKarp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p6, p3, p7				;Pickup Position
kv2	=	p5					;String Sustain

asig	STKStifKarp cpspch(p4), 1, 4, kv1, 11, kv2, 1, 10
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0  2  5.00 0  100 100
i 1 +  40 5.00 127 1  127
i 1 10 32 5.00 127 1  10
e
</CsScore>
</CsoundSynthesizer>
