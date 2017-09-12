<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2

giamp = 10000

	instr 1	;untitled

iamp = giamp


kfreq init p4

kenv	linseg 0, p3 * (.25 + .25), 1, p3 * .5 * 1, 0
kenv 	= kenv * iamp

if (1 == 0) then
	kenv = kenv * .5	
	kfreq = kfreq + .0001
else
    kfreq = kfreq - .0001
endif

aout	vco2 kenv, kfreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2 440
e

</CsScore>

</CsoundSynthesizer>
