<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fink.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

gihand fiopen "test.txt", 1

instr 1

 khz  init 0
      fink gihand, 0, 1, khz 
 ar   oscil 0.5, khz
      outs  ar, ar
endin
</CsInstruments> 
<CsScore> 

i 1 0 1 

e
</CsScore> 
</CsoundSynthesizer> 
