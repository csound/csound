<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKSaxofony.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifreq	=	p4
kv1	=	p5					;reed stiffness
kv3	line	p6, p3, p7				;blow position
kv6	line	0, p3, 127				;depth of low-frequency oscillator

asig	STKSaxofony cpspch(p4), 1, 2, kv1, 4, 100, 26, 70, 11, kv3, 1, kv6, 29, 100
asig	=	asig * .5				;too loud
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 3 6.00 30 100 10
i 1 + . 8.00 30 100 100
i 1 + . 7.00 90 127 30
e
</CsScore>
</CsoundSynthesizer>
