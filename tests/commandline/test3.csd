<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=1
nchnls=2


	instr 1	;untitled

iamp,ifreq = 10000,440

aout1,aout2 = vco2(iamp, ifreq), vco2(iamp,ifreq)

	outs aout1, aout2
	endin


</CsInstruments>

<CsScore>

i1	0.0	2	
e

</CsScore>

</CsoundSynthesizer>
