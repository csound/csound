<CsoundSynthesizer>
<CsOptions>
-d -odac -b1024 -B4096
</CsOptions>
<CsInstruments>
sr=44100
ksmps=100
nchnls=1

instr 1

k1 linseg 100, p3/4, 2000, p3/4, 1000, p3/4, 2000, p3/4, 4000
a1 oscili 15000,k1,1

f1 pvsanal a1,1024,256,1024,1
pvsout f1, 0
out a1

endin

</CsInstruments>
<CsScore>
f1 0 1024 10 1 0.5 0.33 0.25
i1 0 10
e
</CsScore>
</CsoundSynthesizer>

