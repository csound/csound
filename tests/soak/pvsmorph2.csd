<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsmorph-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;; example written by joachim heintz 2009, additions by Menno Knevel 2021
; this example uses the files "flute.aiff" and "saxophone-alto-C-octave0.wav"

instr 1

iampint1	=	p4                                  ; value for interpolating the amplitudes at the beginning ...
iampint2	=	p5                                  ; ... and at the end
ifrqint1	=	p6                                  ; value for unterpolating the frequencies at the beginning ...
ifrqint2	=	p7                                  ; ... and at the end
kampint	linseg	iampint1, p3, iampint2
kfrqint	linseg	ifrqint1, p3, ifrqint2
ifftsize	=	1024
ioverlap	=	ifftsize / 4
iwinsize	=	ifftsize
iwinshape	=	1                                   ; von-Hann window
Sfile1		=	"flute.aiff"
Sfile2		=	"saxophone-alto-C-octave0.wav"
ain1		soundin	Sfile1
ain2		soundin	Sfile2
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape
fmorph		pvsmorph fftin1, fftin2, kampint, kfrqint
aout		pvsynth	fmorph
outs		aout , aout
endin

instr 2 ; moving randomly in certain borders between two spectra

iampintmin	=	p4                                  ; minimum value for amplitudes
iampintmax	=	p5                                  ; maximum value for amplitudes
ifrqintmin	=	p6                                  ; minimum value for frequencies
ifrqintmax	=	p7                                  ; maximum value for frequencies
imovefreq	=	p8                                  ; frequency for generating new random values
kampint	randomi	iampintmin, iampintmax, imovefreq
kfrqint	randomi	ifrqintmin, ifrqintmax, imovefreq
ifftsize	=	1024
ioverlap	=	ifftsize / 4
iwinsize	=	ifftsize
iwinshape	=	1                                   ; von-Hann window
Sfile1		=	"flute.aiff"
Sfile2		=	"saxophone-alto-C-octave0.wav"
ain1		soundin	Sfile1
ain2		soundin	Sfile2
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape
fmorph		pvsmorph fftin1, fftin2, kampint, kfrqint
aout		pvsynth	fmorph
outs		aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0  3  0  0   1  1         ; amplitudes from flute, frequencies from saxophone
i 1 3  3  1  1   0  0         ; amplitudes from saxophone, frequencies from flute
i 1 6  3  0  1   0  1         ; amplitudes and frequencies moving from flute to saxophone 
i 1 9  3  1  0   1  0         ; amplitudes and frequencies moving from saxophone to flute

i 2 13 3 .2  .8 .2 .8  10      ; amps and freqs moving randomly between the two spectra
e
</CsScore>
</CsoundSynthesizer>
