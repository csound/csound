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
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile		=		"fox.wav"
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of the audio-signal
fftblur	pvscale	fftin, p4, p5, p6; scale
aout		pvsynth	fftblur; resynthesis
		out		aout
endin

</CsInstruments>
<CsScore>
i 1 0 3 1 0 1; original sound
i 1 3 3 1.5 0 2; fifth higher without ...
i 1 6 3 1.5 1 2; ... and with different ...
i 1 9 3 1.5 2 5; ... kinds of formant preservation
e
</CsScore>
</CsoundSynthesizer>

