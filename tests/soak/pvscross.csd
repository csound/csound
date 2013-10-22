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

instr 1
ipermut	=		p4; 1 = change order of soundfiles 
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile1		=		"fox.wav"
Sfile2		=		"wave.wav"
ain1		soundin	Sfile1
ain2		soundin	Sfile2
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file 1
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file 2
ktrans		linseg		0, p3, 1; linear transition 
if ipermut == 1 then
fcross		pvscross	fftin2, fftin1, ktrans, 1-ktrans
else
fcross		pvscross	fftin1, fftin2, ktrans, 1-ktrans
endif
aout		pvsynth	fcross
		out		aout
endin

</CsInstruments>
<CsScore>
i 1 0 2.757 0; frequencies from fox. wav; amplitudes moving from wave to fox
i 1 3 2.757 1; frequencies from wave.wav, amplitudes moving from fox to wave
e
</CsScore>
</CsoundSynthesizer>

