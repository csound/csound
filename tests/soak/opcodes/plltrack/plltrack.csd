<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kd = p4
a1 diskin2 "fox.wav", 1,0,1
apitch, aloc plltrack a1, kd
krms rms a1
krms port krms, 0.01
asig buzz krms, apitch, 10, 1
     outs asig, asig		;mix in some dry signal as well

endin
</CsInstruments>
<CsScore>
f1 0 65536 10 1	;sine wave

i 1 0 6 0.1
i 1 7 6 0.3	;more feedback

e
</CsScore>
</CsoundSynthesizer>
