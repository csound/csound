<CsoundSynthesizer>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

asig diskin2 "fox.wav", 1, 0, 1
 
kfr = p4
ifdbgain = 0.90

astr streson asig, kfr, ifdbgain
asig clip astr, 0, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 1 20
i 1 + . >
i 1 + . >
i 1 + . >
i 1 + . >
i 1 + . >
i 1 + . 1000
e
</CsScore>
</CsoundSynthesizer>
