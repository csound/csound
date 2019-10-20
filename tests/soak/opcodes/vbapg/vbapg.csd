<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4	;quad
0dbfs  = 1

vbaplsinit 2.01, 4, 0, 90, 180, 270

instr 1

asig diskin2 "beats.wav", 1, 0, 1		;loop beats.wav
kaz  line 0, p3, p4				;come from right rear speaker &	
k1,k2,k3,k4 vbapg  180, 100, kaz, 1             ;change spread of soundsource
     printks "spread of source = %d\n", 1, kaz	;print spread value
     outq asig*k1,asig*k2,asig*k3,asig*k4

endin 
</CsInstruments>
<CsScore>

i 1 0 12 100

e
</CsScore>
</CsoundSynthesizer>
