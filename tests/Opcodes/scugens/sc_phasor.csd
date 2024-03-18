<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

instr 1
	krate linseg 1, p3, 40
	ktrig metro krate
	kx sc_phasor ktrig, krate/kr, 0, 1
	asine oscili 0.2, kx*500+500
	outch 1, asine
endin
	
instr 2
	krate linseg 1, p3, 40
	atrig = mpulse(1, 1/krate)
	ax sc_phasor atrig, krate/sr, 0, 1
	asine oscili 0.2, ax*500+500
	outch 2, asine
endin

</CsInstruments>
<CsScore>
i1 0 20
i2 0 20
</CsScore>
</CsoundSynthesizer>
