<CsoundSynthesizer>
<CsOptions>
; Audio out   Audio in
-odac           -iadc     -M0    ;;;RT audio I/O and RT midi
</CsOptions>

<CsInstruments>

sr = 44100
ksmps = 100
nchnls = 2

giengine	fluidEngine
; soundfont path to manual/examples
isfnum	fluidLoad "07AcousticGuitar.sf2", giengine, 1
		fluidProgramSelect giengine, 1, isfnum, 0, 0

instr 1
	mididefault		60, p3
	midinoteonkey	p4, p5
ikey	init	p4
ivel	init	p5

	fluidNote	giengine, 1, ikey, ivel
endin

instr 99
imvol	init	120000
asigl, asigr	fluidOut	giengine
	outs	asigl * imvol, asigr * imvol
endin

</CsInstruments>

<CsScore>

i 1 0 3600 60 100
i 99 0 3600
e

</CsScore>

</CsoundSynthesizer>

