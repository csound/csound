;Widget Test
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

0dbfs = 1
nchnls = 1

chn_k "slider1", 1

instr 1

	kinput chnget "slider1"
	a1 vco2 0.1, 440 + kinput 
	out a1 
endin

</CsInstruments>
<CsScore>

i1 0 40

</CsScore>
</CsoundSynthesizer>
