<CsoundSynthesizer>

<CsInstruments>
	sr = 96000
	kr = 960
	ksmps = 100
	nchnls = 2

	instr	1
a1	oscil	p4, p5, 1
	outs	a1,a1
	endin

</CsInstruments>

<CsScore>
f1 0 8192 10 1
i1 0 1 20000 440
e
</CsScore>

</CsoundSynthesizer>
