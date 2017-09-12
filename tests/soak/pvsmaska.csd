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

; function table for defining amplitude peaks (from the example of Richard Dobson)
giTab		ftgen		0, 0, 513, 8, 0, 2, 1, 3, 0, 4, 1, 6, 0, 10, 1, 12, 0, 16, 1, 32, 0, 1, 0, 436, 0

instr 1
imod		=		p4; degree of midification (0-1)
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
Sfile		=		"fox.wav"
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file
fmask		pvsmaska  	fftin, giTab, imod
aout		pvsynth	fmask; resynthesize
		out		aout
endin

</CsInstruments>
<CsScore>
i 1 0 2.757 0 
i 1 3 2.757 1
e
</CsScore>
</CsoundSynthesizer>

