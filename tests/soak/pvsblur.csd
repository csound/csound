 <CsoundSynthesizer>
 <CsOptions>
 ; Select audio/midi flags here according to platform
 ; Audio out   Audio in
 -odac       ;;;RT audio out
 ; For Non-realtime ouput leave only the line below:
 ; -o pvsblur.wav -W ;;; for file output any platform
 </CsOptions>
 <CsInstruments>

 sr = 44100
 ksmps = 32
 nchnls = 2
 0dbfs  = 1

 ;; example written by joachim heintz 2009

 instr 1
 ifftsize	= 1024
 ioverlap	= ifftsize / 4
 iwinsize	= ifftsize
 iwinshape	= 1               ; von-Hann window
 Sfile	= "fox.wav"
 ain		soundin	Sfile
 fftin	pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of the audio-signal
 fftblur	pvsblur	fftin, p4, 1; blur
 aout		pvsynth	fftblur; resynthesis
 		outs	aout, aout
 endin

 </CsInstruments>
 <CsScore>
 i 1 0 3    0
 i 1 3 3   .1
 i 1 6 3.5 .5
 e
 </CsScore>
 </CsoundSynthesizer>
