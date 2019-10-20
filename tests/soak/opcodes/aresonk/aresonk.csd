<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

gisin ftgen 0, 0, 2^10, 10, 1

instr 1

ksig	randomh	400, 1800, 150
aout	poscil	.2, 1000+ksig, gisin
	outs	aout, aout
endin

instr 2

ksig	randomh	400, 1800, 150
kbw	line	1, p3, 600	; vary bandwith
ksig	aresonk	ksig, 800, kbw
aout	poscil	.2, 1000+ksig, gisin
	outs	aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0 5
i 2 5.5 5
e
</CsScore>
</CsoundSynthesizer>
