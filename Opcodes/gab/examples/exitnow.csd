<CsoundSynthesizer>
<CsOptions>
-odac1 -B441 -b441
</CsOptions>
<CsInstruments>

sr     =        44100
kr     =        100
ksmps  =        441
nchnls =        2

       instr    1
print 55665
	endin

	instr	2
exitnow
	endin

</CsInstruments>
<CsScore>

i 1 0 20
i 2 6 1

</CsScore>
</CsoundSynthesizer>
