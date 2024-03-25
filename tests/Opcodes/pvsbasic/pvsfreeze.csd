<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac  ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o pvsfreeze.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

;; example written by joachim heintz 2009

seed		0

instr 1

ifftsize        = 1024
ioverlap	= ifftsize / 4
iwinsize	= ifftsize
iwinshape	= 1     ; von-Hann window
Sfile1		= "fox.wav"
ain		soundin	Sfile1
kfreq		randomh	.7, 1.1, 3; probability of freezing freqs: 1/4
kamp		randomh	.7, 1.1, 3; idem for amplitudes
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of file
freeze		pvsfreeze	fftin, kamp, kfreq; freeze amps or freqs independently
aout		pvsynth	freeze; resynthesize
		outs aout, aout
endin

</CsInstruments>
<CsScore>
r 10
i 1 0 2.757
e
</CsScore>
</CsoundSynthesizer>
