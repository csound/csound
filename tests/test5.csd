<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2

giamp = 10000

	instr 1	;untitled

;iamp = 10000
ifreq = 440

kenv	linseg 0, 1, giamp, 1, 0
aout	vco2 kenv, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2	
e

</CsScore>

</CsoundSynthesizer>
