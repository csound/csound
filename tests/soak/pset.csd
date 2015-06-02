<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pset.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2 

instr 1 ;this shows an example with non-midi use

pset 1, 0, 1, 220, 0.5 
asig poscil p5, p4, 1 
     outs asig, asig
 
endin 
</CsInstruments> 
<CsScore> 
f 1 0 1024 10 1	;sine wave

i 1 0 1 
i 1 1 1 440 
i 1 2 1 440 0.1 
e
</CsScore> 
</CsoundSynthesizer> 