<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKPlucked.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1  ;STKPlucked - has no controllers

ifrq	=	p4

asig	STKPlucked cpspch(ifrq), 1
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2 6.00 
i 1 + 8 5.00
i 1 + .5 8.00
e
</CsScore>
</CsoundSynthesizer>