<CsoundSynthesizer>
<CsOptions>
-d
</CsOptions>
<CsInstruments>
sr=44100
ksmps=10
nchnls=1

instr 1

k1 invalue "amp"
k1 tonek k1, 10
k2 invalue "freq"
asig oscili k1,k2,1;
outvalue "freq", k2
     out asig
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1
i1 0 60

</CsScore>

</CsoundSynthesizer>
