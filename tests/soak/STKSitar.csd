<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKSitar.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1  ;STKSitar - has no controllers

ifrq	=	p4

asig	STKSitar cpspch(p4), 1
asig	=	asig * 2			;amplify
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 4 6.00 
i 1 + 2 7.05
i 1 + 7 5.05
e
</CsScore>
</CsoundSynthesizer>