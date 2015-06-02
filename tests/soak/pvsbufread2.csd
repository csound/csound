<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
ksmps  = 64
0dbfs  = 1
nchnls = 2

instr 1
kcnt     init         0
ifftsize =            2048
ihop     =            ifftsize/4

a1       diskin2      "beats.wav", 1, 0, 1
a1       =            a1*0.5
fsig1    pvsanal      a1, ifftsize, ihop, ifftsize, 1
ih, kt   pvsbuffer    fsig1, 10

fsig2    pvsbufread2  kt, ih, 1, 1
fsig3    pvsbufread2  kt, ih, 2, 2

a2       pvsynth      fsig3
a3       pvsynth      fsig2

         outs         a2, a3
endin
</CsInstruments>

<CsScore>
f1 0 2048 -7 0 128 1.1 128 0.5  256 1.8 512 1.1 1024 0.1
f2 0 2048 -7 1 128 0.2 128 0.05 256 0.5 512 0.9 1024 0.1

i1 0 60
</CsScore>
</CsoundSynthesizer>
