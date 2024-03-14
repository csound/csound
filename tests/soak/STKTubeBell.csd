<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKTubeBell.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv2	line	p6, p3, p7				;Crossfade of Outputs
kv1	=	p5					;(FM) Modulator Index One

asig	STKTubeBell cpspch(p4), 1, 2, kv1, 4, kv2, 11, 10, 1, 70, 128,50
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2 7.05   0 100 100
i 1 + 4 9.00  127 127 30
i 1 + 1 10.00 127 12 30
i 1 + 3 6.08 127 1 100
e
</CsScore>
</CsoundSynthesizer>