<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKBeeThree.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kv1	=	p6					;feedback of operator 4
kv2	line	p4, p3, p5				;gain of operator 3
kv5	line	0, p3, 100
ipch	=	p7

asig	STKBeeThree	cpspch(ipch), 1, 2, kv1, 4, kv2, 11, 50, 1, 0, 128, kv5
	outs	asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2 20 100 127 8.00
i 1 + 8 120 0 0 6.09
e
</CsScore>
</CsoundSynthesizer>
