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
ishift		=		p4; shift amount in Hz
ilowest	=		p5; lowest frequency to be shifted
ikeepform	=		p6; 0=no formant keeping, 1=keep by amps, 2=keep by spectral envelope
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile		=		"fox.wav"
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file
fshift		pvshift  	fftin, ishift, ilowest, ikeepform; shift frequencies
aout		pvsynth	fshift; resynthesize
		out		aout
endin

</CsInstruments>
<CsScore>
i 1 0 2.757 0 0 0; no shift at all
i 1 3 2.757 100 0 0; shift all frequencies by 100 Hz
i 1 6 2.757 200 0 0; by 200 Hz
i 1 9 2.757 200 0 1; keep formants by method 1
i 1 12 2.757 200 0 2; by method 2
i 1 15 2.757 200 1000 0; shift by 200 Hz but just above 1000 Hz
i 1 18 2.757 1000 500 0; shift by 1000 Hz above 500 Hz
i 1 21 2.757 1000 300 0; above 300 Hz
e
</CsScore>
</CsoundSynthesizer>

