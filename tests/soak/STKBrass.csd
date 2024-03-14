<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKBrass.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6					;lip tension
kv3	line	p7, p3, p8					;speed of low-frequency oscillator

asig	STKBrass cpspch(ifrq), 1, 2, kv1, 4, 100, 11, kv3, 1, 10, 128, 40
asig	=	asig *3						;amplify
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2 8.05 100 120 50 0 
i 1 + 3 9.00  80 82 10 0
e
</CsScore>
</CsoundSynthesizer>
