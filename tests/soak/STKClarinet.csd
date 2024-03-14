<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKclarinet.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv5	=	p5						;breath pressure
kv1	line	p6, p3, p7					;reed stiffness
asig	STKClarinet cpspch(p4), 1, 2, kv1, 4, 100, 11, 60, 1, 10, 128, kv5
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 3  8.00 100 127 10
i 1 + 10 8.08  80  60 100
e
</CsScore>
</CsoundSynthesizer>