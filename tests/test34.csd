<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

instr 1
i0 divz 1,2,3
ii = (p3<10 ? 1 : 2)
print ii
endin

</CsInstruments>
<CsScore>
f 1 0 1024 10 1
i 1 0 20
e

</CsScore>
</CsoundSynthesizer>
