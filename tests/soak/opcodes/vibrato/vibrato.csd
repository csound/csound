<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kaverageamp     init .5
kaveragefreq    init 5
krandamountamp  line p4, p3, p5			;increase random amplitude of vibrato
krandamountfreq init .3
kampminrate init 3
kampmaxrate init 5
kcpsminrate init 3
kcpsmaxrate init 5
kvib vibrato kaverageamp, kaveragefreq, krandamountamp, krandamountfreq, kampminrate, kampmaxrate, kcpsminrate, kcpsmaxrate, 1
asig poscil .8, 220+kvib, 1			;add vibrato
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1       ;sine wave

i 1 0 15 .01 20

e
</CsScore>
</CsoundSynthesizer>

