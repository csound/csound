<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outleta.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1 

connect	 "1", "Outl", "reverby", "InL" 
connect	 "1", "Outr", "reverby", "InR" 

alwayson "reverby", 1

instr 1

aIn diskin2 "fox.wav", 1 
    outleta "Outl", aIn 
    outleta "Outr", aIn 

endin 

instr reverby

aInL   inleta "InL" 
aInR   inleta "InR" 

al, ar reverbsc	aInL, aInR, 0.7, 21000 
ifxlev = 0.5 
al     = (aInL*ifxlev)+(al*(1-ifxlev)) 
ar     = (aInR*ifxlev)+(ar*(1-ifxlev)) 
       outs al, ar 
                                
endin 
</CsInstruments>
<CsScore>

i 1 0 3 
e4
</CsScore>
</CsoundSynthesizer>
