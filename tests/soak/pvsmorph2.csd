<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 16
nchnls = 1
0dbfs = 1

;; example written by joachim heintz 2009
; this example uses the files "flute-C-octave0.wav" and
; "saxophone-alto-C-octave0.wav" from www.archive.org/details/OpenPathMusic44V2

giSine		ftgen		0, 0, 4096, 10, 1

instr 1
iampint1	=		p4; value for interpolating the amplitudes at the beginning ...
iampint2	=		p5; ... and at the end
ifrqint1	=		p6; value for unterpolating the frequencies at the beginning ...
ifrqint2	=		p7; ... and at the end
kampint	linseg		iampint1, p3, iampint2
kfrqint	linseg		ifrqint1, p3, ifrqint2
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile1		=		"flute-C-octave0.wav"
Sfile2		=		"saxophone-alto-C-octave0.wav"
ain1		soundin	Sfile1
ain2		soundin	Sfile2
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape
fmorph		pvsmorph	fftin1, fftin2, kampint, kfrqint
aout		pvsynth	fmorph
		out		aout * .5
endin

instr 2; moving randomly in certain borders between two spectra
iampintmin	=		p4; minimum value for amplitudes
iampintmax	=		p5; maximum value for amplitudes
ifrqintmin	=		p6; minimum value for frequencies
ifrqintmax	=		p7; maximum value for frequencies
imovefreq	=		p8; frequency for generating new random values
kampint	randomi	iampintmin, iampintmax, imovefreq
kfrqint	randomi	ifrqintmin, ifrqintmax, imovefreq
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile1		=		"flute-C-octave0.wav"
Sfile2		=		"saxophone-alto-C-octave0.wav"
ain1		soundin	Sfile1
ain2		soundin	Sfile2
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape
fmorph		pvsmorph	fftin1, fftin2, kampint, kfrqint
aout		pvsynth	fmorph
		out		aout * .5
endin

</CsInstruments>
<CsScore>
i 1 0 3 0 0 1 1; amplitudes from flute, frequencies from saxophone
i 1 3 3 1 1 0 0; amplitudes from saxophone, frequencies from flute
i 1 6 3 0 1 0 1; amplitudes and frequencies moving from flute to saxophone 
i 1 9 3 1 0 1 0; amplitudes and frequencies moving from saxophone to flute
i 2 12 3 .2 .8 .2 .8 5; amps and freqs moving randomly between the two spectra
e
</CsScore>
</CsoundSynthesizer>

