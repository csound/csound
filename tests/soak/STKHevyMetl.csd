<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKHevyMetl.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv1	line	p5, p3, p6				;Total Modulator Index
kv2	line	p7, p3, 0				;Modulator Crossfade

asig	STKHevyMetl cpspch(ifrq), 1, 2, kv1, 4, kv2, 11, 0, 1, 100, 128, 40
	outs asig, asig
endin

</CsInstruments>
<CsScore>
i 1 0 7  8.05 100 0  100
i 1 3 7   9.03 20 120  0
i 1 3 .5  8.05 20 120  0
i 1 4 .5  9.09 20 120  0
i 1 5 3   9.00 20 120  0

e
</CsScore>
</CsoundSynthesizer>