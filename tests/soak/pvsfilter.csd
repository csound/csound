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

giSine		ftgen		0, 0, 4096, 10, 1
giBell		ftgen		0, 0, 4096, 9, .56, 1, 0, .57, .67, 0, .92, 1.8, 0, .93, 1.8, 0, 1.19, 2.67, 0, 1.7, 1.67, 0, 2, 1.46, 0, 2.74, 1.33, 0, 3, 1.33, 0, 3.76, 1, 0, 4.07, 1.33, 0; bell-like (after Risset)

instr 1
ipermut	=		p4; 1 = change order of soundfiles 
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile1		=		"fox.wav"
ain1		soundin	Sfile1
kfreq		randomi	200, 300, 3
ain2		oscili		.2, kfreq, giBell
;ain2		oscili		.2, kfreq, giSine; try also this 
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file 1
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file 2
if ipermut == 1 then
fcross		pvsfilter	fftin2, fftin1, 1
else
fcross		pvsfilter	fftin1, fftin2, 1
endif
aout		pvsynth	fcross
		out		aout * 20
endin

</CsInstruments>
<CsScore>
i 1 0 2.757 0; frequencies from fox.wav, amplitudes multiplied by amplitudes of giBell
i 1 3 2.757 1; frequencies from giBell, amplitudes multiplied by amplitudes of fox.wav
e
</CsScore>
</CsoundSynthesizer>

