<CsoundSynthesizer>

<CsInstruments>
sr=44100
ksmps=1
nchnls=2

giamp = 10000

		opcode testUDO, i, i
ipch	xin
print ipch
ipch 	= ipch * 2
		xout ipch
		endop

	instr 1	;untitled

iamp = giamp

ipch = p4

kenv	linseg 0, p3 * (.25 + .25), 1, p3 * .5 * 1, 0
kenv 	= kenv * iamp

ifreq	testUDO ipch

aout	vco2 kenv, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	1 440
i1  + . 550
i1  + . 660
i1  + . 770
i1  + . 880

e

</CsScore>

</CsoundSynthesizer>
