<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0  ;;;realtime audio out and midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o foutir.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

gihand fiopen "foutir.sco", 0 

instr 1 ; play virtual midi keyboard

inot  notnum  ;just for priting on screen
icps  cpsmidi    
iamp  ampmidi 1

      foutir  gihand, 0, 1, icps, iamp 
      prints  "WRITING:\n"
      prints  "note = %f,velocity = %f\n", icps, iamp  ;prints them
      ficlose gihand
asig  pluck   iamp, icps, 1000, 0, 1
      outs    asig, asig

endin 
</CsInstruments> 
<CsScore> 

f 0 10

e
</CsScore> 
</CsoundSynthesizer> 
