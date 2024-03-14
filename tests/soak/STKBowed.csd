<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKBowed.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ipch	= p4
kv2	= p7							;position on bow
kv1	line	p5, p3, p6					;bow pressure
kv4	line	0, p3, 7					;depth of low-frequency oscillator

asig	STKBowed cpspch(ipch), 1, 2, kv1, 4, kv2, 11, 40, 1, kv4, 128, 100
asig	=	asig*2						;amplify
	outs	asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 15 6.00 20 100 115
i 1 17 3 7.00 120 0  0
i 1 21 3 7.09 120 0  30
i 1 21 4 7.03 50  0  0
e
</CsScore>
</CsoundSynthesizer>
