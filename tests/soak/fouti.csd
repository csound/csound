<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fouti.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

gihand fiopen "test.txt", 0 

instr 1

ires  random  0, 10 
      fouti gihand, 0, 1, ires 
      ficlose gihand

endin 
</CsInstruments> 
<CsScore> 

i 1 0 1 

e
</CsScore> 
</CsoundSynthesizer> 
