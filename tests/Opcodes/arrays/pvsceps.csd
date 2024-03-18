 <CsoundSynthesizer>
 <CsOptions>
 ; Select audio/midi flags here according to platform
 ; Audio out   Audio in
 -odac       ;;;RT audio out
 ; For Non-realtime ouput leave only the line below:
 ; -o pvsceps.wav -W ;;; for file output any platform
 </CsOptions>
 <CsInstruments>

 sr = 44100
 ksmps = 32
 nchnls = 2
 0dbfs  = 1

 instr 1

 a1      diskin "fox.wav",1,0,1
 k1      randh  80, 2.5
 a2      vco2  8, 220+k1
 fsig    pvsanal a1,1024,256,1024,1
 fsig2   pvsanal a2,1024,256,1024,1
 keps[]  pvsceps fsig, p4
 kenv[]  cepsinv keps
 fenv    tab2pvs r2c(kenv)
 fvoc    pvsfilter fsig2, fenv, 1
 asig    pvsynth fvoc
         outs asig, asig
 endin

 </CsInstruments>
 <CsScore>
 i1 0 30 30  ; p4 -= the number of retained coefficients
 i1 31 10 5
 e
 </CsScore>
 </CsoundSynthesizer>
