<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2

giamp = 10000

	instr 1	;untitled

iamp = giamp

goto skipIamp

iamp = 10

skipIamp:

ifreq = p4

kenv	linseg 0, p3 * (.25 + .25), 1, p3 * .5 * 1, 0
kenv 	= kenv * iamp
;kenv = 1

if (1 == 1) kgoto testLabel

kenv = 0

testLabel:

aout	vco2 kenv, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2 440
e

</CsScore>

</CsoundSynthesizer>
