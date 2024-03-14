<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

gisine ftgen 0,0,2^12,10,1

instr 1

kdclk   linseg  0, 0.1, 1, p3-0.02, 1, 0.1, 0
kb linseg 1,p3/2,1.7,p3/2,1
krot linseg 0,p3,1

asig sterrain 0.5, 220, 0.5, 0.5, 0.5, 0.5, krot, gisine, gisine, 2,2,0.5,2,2,0.7,kb,2
asig dcblock asig
asig = asig * kdclk
aL,aR reverbsc asig, asig , 0.6, 12000, 44100, 0.5, 1
outs aL, aR

endin

</CsInstruments>

<CsScore>

i 1 0 40

e
</CsScore>
</CsoundSynthesizer>

