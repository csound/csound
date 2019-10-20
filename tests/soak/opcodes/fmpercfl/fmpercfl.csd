<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

instr 1

kfreq = 220
kc1 = 5
kvdepth = .01
kvrate = 6

kc2  line 5, p3, p4
asig fmpercfl .5, kfreq, kc1, kc2, kvdepth, kvrate
     outs asig, asig
endin


</CsInstruments>
<CsScore>
; sine wave.
f 1 0 32768 10 1

i 1 0 4 5
i 1 5 8 .1

e
</CsScore>
</CsoundSynthesizer>
