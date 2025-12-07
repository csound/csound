<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
 asigs[] init 2
 asigs[0] = oscili:a(0.1, 1000)
 asigs[0] *= linsegr:a(0, 0.5, 1, 0.5, 1)
 outch 1, asigs[0]
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

