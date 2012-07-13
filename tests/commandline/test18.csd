<CsoundSynthesizer>

<CsInstruments>
sr=44100
ksmps=1
nchnls=2

giamp = 10000

	instr 1	;untitled

iamp = giamp


kfreq init p4

kenv	linseg 0, p3 * (.25 + .25), 1, p3 * .5 * 1, 0
kenv 	= kenv * iamp

kadd init .001
if (kfreq <= 420) then
	kadd = .001
elseif (kfreq >= 460) then
        kadd =  -.001
endif
kfreq = kfreq + kadd
aout	vco2 kenv, kfreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2 440
e

</CsScore>

</CsoundSynthesizer>
