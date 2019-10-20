<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

instr 1

kfreq = 220
kc1 = p4
kc2 = p5
kvrate = 6

kvdpth line 0, p3, p6
asig   fmb3 .4, kfreq, kc1, kc2, kvdpth, kvrate
       outs asig, asig

endin
</CsInstruments>
<CsScore>
;sine wave.
f 1 0 32768 10 1

i 1 0 2  5  5 0.1
i 1 3 2 .5 .5 0.01
e
</CsScore>
</CsoundSynthesizer>
