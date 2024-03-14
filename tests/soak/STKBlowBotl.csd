<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKBlowBotl.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ipch	= p4
kv1	line p5, p3, p6					;gain of noise
kv4	line	100, p3, 70				;volume

asig	STKBlowBotl cpspch(ipch), 1, 4, kv1, 11, 10, 1, 50, 128, kv4
asig	=	asig * .5				;too loud
	outs	asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2 9.00 20 100
i 1 + 5 8.03 120 0
e
</CsScore>
</CsoundSynthesizer>
