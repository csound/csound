<CsoundSynthesizer>

<CsInstruments>
sr=48000
ksmps=1
nchnls=2

giamp = 10000

		opcode testUDO, i, i
ipch	xin
print ipch
ipch 	= ipch * 2
		xout ipch
		endop

	instr itest	;untitled

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

i"itest"	0.0	1 440
i"itest"  + . 550
i"itest"  + . 660
i"itest"  + . 770
i"itest"  + . 880

e

</CsScore>

</CsoundSynthesizer>
