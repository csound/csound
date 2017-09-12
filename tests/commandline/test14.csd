<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2

	instr 1	;untitled

ifreq = p4
iamp = ampdb(p5)

ihalf init 1
ihalf2 init 1

kenv	linseg 0, ihalf, 1, p3 * .5, 0
kenv 	= kenv * iamp


aout	vco2 kenv, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>
i1	0.0	2 440 80
e

</CsScore>

</CsoundSynthesizer>