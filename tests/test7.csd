<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2

giamp = 10000

	instr 1	;untitled

;iamp = 10000
ifreq = p4

kenv	linseg 0, p3 * .5, 1, p3 * .5, 0
kenv 	= kenv * giamp
;kenv = 1
aout	vco2 kenv, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2 440
e

</CsScore>

</CsoundSynthesizer>
