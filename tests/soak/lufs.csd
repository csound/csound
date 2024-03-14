<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr=48000
ksmps=64
0dbfs=1.0
nchnls=2

instr 1
ktrig init 0
iamp = ampdbfs(-18.0)
a1 poscil iamp,1000,1
kM,kI,kS lufs ktrig,a1,a1
printks "M: %f, I: %f, S: %f LUFS\n", 0.3, k1, k2, k3
endin

</CsInstruments>
<CsScore>
f1 0 8192 10 1
i1 0 20
</CsScore>
</CsoundSynthesizer>
