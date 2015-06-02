<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsarp2.wav -W ;;; for file output any platform
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
Sfile1		=		"fox.wav"
ain1		soundin	Sfile1
fftin		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape
  ;make 3 independently moving accentuations in the spectrum
kbin1		linseg		0.05, p3/2, .05, p3/2, .05
farp1		pvsarp		fftin, kbin1, .9, 10
kbin2		linseg		0.075, p3/2, .1, p3/2, .075
farp2		pvsarp		fftin, kbin2, .9, 10
kbin3		linseg		0.02, p3/2, .03, p3/2, .04
farp3		pvsarp		fftin, kbin3, .9, 10
  ;resynthesize and add them
aout1		pvsynth	farp1
aout2		pvsynth	farp2
aout3		pvsynth	farp3
aout		=		aout1*.3 + aout2*.3 + aout3*.3
		out		aout
endin

</CsInstruments>
<CsScore>
i 1 0 3 
e
</CsScore>
</CsoundSynthesizer>
