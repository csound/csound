<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsbandr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 16
nchnls = 1
0dbfs = 1

;; example written by joachim heintz 2009

instr 1
Sfile		=		"fox.wav"
klowcut = 100
klowfull = 200
khighfull = 1900
khighcut = 2000
ain		soundin	Sfile
fftin		pvsanal	ain, 1024, 256, 1024, 1; fft-analysis of the audio-signal
fftbp		pvsbandr	fftin, klowcut, klowfull, khighfull, khighcut ; band reject
abp		pvsynth	fftbp; resynthesis
		out		abp
endin


</CsInstruments>
<CsScore>
i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
