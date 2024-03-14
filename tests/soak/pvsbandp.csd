<CsoundSynthesizer>
 <CsOptions>
 ; Select audio/midi flags here according to platform
 ; Audio out   Audio in
 -odac    ;;;RT audio I/O
 ; For Non-realtime ouput leave only the line below:
 ; -o pvsbandp.wav -W ;;; for file output any platform
 </CsOptions>
 <CsInstruments>

 sr = 44100
 ksmps = 32
 nchnls = 2
 0dbfs = 1

 ;; example written by joachim heintz 2009

 instr 1
 Sfile = "fox.wav"
 klowcut = 100
 klowfull = p4
 khighfull = p5
 khighcut = 2000
 ain    soundin	Sfile
 fftin  pvsanal	ain, 1024, 256, 1024, 1; fft-analysis of the audio-signal
 fftbp  pvsbandp	fftin, klowcut, klowfull, khighfull, khighcut ; band pass
 abp    pvsynth	fftbp; resynthesis
 	outs    abp, abp
 endin


 </CsInstruments>
 <CsScore>
 ;         lowfull highfull
 i 1 0 3     200     1900
 i 1 + 3     100     1500
 e
 </CsScore>
 </CsoundSynthesizer>
