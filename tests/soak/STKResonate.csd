<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKResonate.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 ; frequency and amplitude of STKResonate have no effect

kv2	=	p4				;pole radii
kv1	line	100, p3, 0			;resonance freq + notch freq
kv3	=	kv1
asig	STKResonate 1, 1, 2, kv1, 4, kv2, 1, 10, 11, kv3, 128, 100
asig	=	asig * .3			;too loud
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 1 0 
i 1 + . >
i 1 + . >
i 1 + . >
i 1 + . 120
e
</CsScore>
</CsoundSynthesizer>
