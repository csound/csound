<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o abs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; test instrument for pdclip opcode
instr 3

	idur		= p3
	iamp		= p4
	ifreq		= p5
	ifn			= p6
	
	kenv		linseg		0, .05, 1.0, idur - .1, 1.0, .05, 0
	aosc		oscil		1.0, ifreq, ifn
	
	kmod		expseg		0.00001, idur, 1.0
	aout		pdclip		aosc, kmod, 0.0, 1.0
	
				out			kenv*aout*iamp		
endin

</CsInstruments>
<CsScore>
f1 0 16385 10 1
f2 0 16385 10 1 .5 .3333 .25 .5

; pdclipped sine wave
i3 0 3 15000 440 1
i3 + 3 15000 330 1
i3 + 3 15000 220 1
s

; pdclipped composite wave
i3 0 3 15000 440 2
i3 + 3 15000 330 2
i3 + 3 15000 220 2
e
</CsScore>
</CsoundSynthesizer>
