<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKModalBar.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	=	p5					;stick hardness					;

asig	STKModalBar cpspch(ifrq), 1, 2, kv1, 4, 120, 11, 0, 1, 0, 8, 10, 16, 1
asig	=	asig * 3				;amplify
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2 8.00 0 
i 1 + 2 8.05 120
e
</CsScore>
</CsoundSynthesizer>
