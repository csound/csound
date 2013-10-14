<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ftgenonce.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments> 

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 
; Use ftgenonce instead of ftgen, ftgentmp, or f statement
iHz	= p4
isine	ftgenonce 0, 0, 1024, 10, 1 
aoscili	pluck .7, iHz, 100, isine, 1 
aadsr 	adsr 0.015, 0.07, 0.6, 0.3
asig 	= aoscili * aadsr
        outs asig, asig
endin 

</CsInstruments> 
<CsScore> 

i 1 0 1 220 
i 1 2 1 261 
e
</CsScore> 
</CsoundSynthesizer> 