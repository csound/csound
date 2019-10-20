<CsoundSynthesizer>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1

kcps = sr/ftlen(1)
asig oscil .8, kcps, 1
   outs asig, asig
   
endin
</CsInstruments>
<CsScore>
f 1 0 131072 49 "beats.mp3" 0 1	;read an audio file (using GEN49).

i 1 0 2
e
</CsScore>
</CsoundSynthesizer>
