<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2


	instr 1	;untitled

iamp,ifreq = 10000,440

aout	vco2 iamp, ifreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2	
e

</CsScore>

</CsoundSynthesizer>
