<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too

</CsOptions> 
<CsInstruments> 

sr = 44100
ksmps = 32
nchnls = 2

instr 1 

inum = p4 
iceil = ceil(inum) 
print iceil 

endin 

</CsInstruments> 
<CsScore> 

i 1 0 0 1 
i . . . 0.999999 
i . . . 0.000001 
i . . . 0 
i . . . -0.0000001 
i . . . -0.9999999 
i . . . -1 
e 
</CsScore> 
</CsoundSynthesizer> 
