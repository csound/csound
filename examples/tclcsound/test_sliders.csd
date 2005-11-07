<CsoundSynthesizer>
<CsOptions>
-d  -B2048 -b2048
</CsOptions>
<CsInstruments>
sr=44100
ksmps=10
nchnls=1

gifn ftgen 1,0, 16384, 10, 1, .5, .33, .25, .2, .16, .14, .12, .1, .9, .8, .7, .6, .5

instr 1

k1 invalue "amp"
k1 tonek k1, 10
k2 invalue "freq"
k3 invalue "filfreq"
k4 invalue "filq"
asig oscili k1,k2,1
afil resonz asig, k3, k3/k4, 1
outvalue "freq", k2
     out afil
endin

</CsInstruments>
<CsScore>
i1 0 600

</CsScore>

</CsoundSynthesizer>
