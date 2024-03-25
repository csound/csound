<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fini.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

gihand fiopen "test.txt", 1

instr 1

 ihz  init 0
      fini gihand, 0, 1, ihz 
 ar   oscil 0.5, ihz
      outs  ar, ar
endin
</CsInstruments> 
<CsScore> 

i 1 0 1 

e
</CsScore> 
</CsoundSynthesizer> 
