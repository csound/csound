<CsoundSynthesizer>

<CsOptions>

; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:

</CsOptions>
<CsInstruments>

; By Stefano Cucchi 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs =1


instr 1
i1 = exp(2) 
; BASE = e (2,71828 18284 59045 23536...)
; EXPONENT = 2
print i1
endin



</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
