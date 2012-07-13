<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2


	instr 1	;untitled

iamp = 10000
ifreq = 440

aout	vco2 iamp, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2	
e

</CsScore>

</CsoundSynthesizer>
