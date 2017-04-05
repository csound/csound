<CsoundSynthesizer>
<CsInstruments>
sr=44100
ksmps=1
nchnls=2

giamp = 10000

		opcode TestUDO, i, i[]
ipch[]	xin
ipch[0] 	= ipch[0] * 2
		xout ipch[0]
		endop

	instr 1	;untitled
iamp = giamp
ipch[] init 1
ipch[0] = p4
kenv	linseg 0, p3 * (.25 + .25), 1, p3 * .5 * 1, 0
kenv 	= kenv * iamp
ifreq	TestUDO ipch
aout	vco2 kenv, ifreq
	outs aout, aout
	endin
</CsInstruments>
<CsScore>
i1	0.0	1 440
e
</CsScore>
</CsoundSynthesizer>
