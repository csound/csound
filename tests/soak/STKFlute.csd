<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKFlute.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6				;jet delay
kv4	line	0, p3, 100				;vibrato depth

asig	STKFlute cpspch(ifrq), 1, 2, kv1, 4, 100, 11, 100, 1, kv4, 128, 100
	outs asig, asig
endin

</CsInstruments>
<CsScore>
i 1 0 5 8.00 0 0
i 1 7 3 9.00 20 120 
e
</CsScore>
</CsoundSynthesizer>