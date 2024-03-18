<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
-iadc    ;;;uncomment -iadc if realtime audio input is needed too
</CsOptions> 
<CsInstruments> 

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

S1 getcfg 1	; -+max_str_len 
S2 getcfg 2	; -i 
S3 getcfg 3	; -o 
S4 getcfg 4	; RTaudio 
S5 getcfg 5	; -t 
S6 getcfg 6	; os system host 
S7 getcfg 7	; callback 

prints "------------------------------" 
prints "\nMax string len : " 
prints	S1 
prints "\nInput file name (-i) : " 
prints	S2 
prints "\nOutput file name (-o) : " 
prints	S3 
prints "\nRTaudio (-odac) : " 
prints	S4 
prints "\nBeat mode (-t)? : " 
prints	S5 
prints "\nHost Op. Sys. : " 
prints	S6 
prints "\nCallback ? : " 
prints	S7 
prints "\n" 
prints "------------------------------\n" 

endin 

</CsInstruments> 
<CsScore> 
i 1 0 0 
e 
</CsScore> 
</CsoundSynthesizer> 