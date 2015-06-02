;Audio File Test
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

0dbfs = 1
nchnls = 1

instr 1

	a1 diskin2 "input.wav", 1
	out a1 
endin

</CsInstruments>
<CsScore>

i1 0 4

</CsScore>
</CsoundSynthesizer>
