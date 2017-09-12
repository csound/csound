<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o abs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
instr 1
	imaxamp    =           10000
	kshapeamt  line        p5, p3, p6
	aosc       oscili      1.0, cpspch(p4), 1
	aout       powershape  aosc, kshapeamt
	adeclick   linseg      0.0, 0.01, 1.0, p3 - 0.06, 1.0, 0.05, 0.0
	
		     out         aout * adeclick * imaxamp
endin

</CsInstruments>
<CsScore>
f1 0 32768 10 1

i1 0 1    7.00  0.000001 0.8
i1 + 0.5  7.02  0.01   1.0
i1 + .    7.05  0.5    1.0
i1 + .    7.07  4.0    1.0
i1 + .    7.09  1.0    10.0
i1 + 2    7.06  1.0    25.0

e

</CsScore>
</CsoundSynthesizer>
